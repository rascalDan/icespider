#include "routeCompiler.h"
#include "routes.h"
#include <Ice/Optional.h>
#include <IceUtil/Handle.h>
#include <Slice/CPlusPlusUtil.h>
#include <Slice/Preprocessor.h>
#include <algorithm>
#include <boost/algorithm/string/find_iterator.hpp>
#include <boost/algorithm/string/join.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/format.hpp>
#include <compileTimeFormatter.h>
#include <cstdlib>
#include <filesystem>
#include <fprintbf.h>
#include <http.h>
#include <initializer_list>
#include <list>
#include <memory>
#include <pathparts.h>
#include <scopeExit.h>
#include <slicer/modelPartsTypes.h>
#include <slicer/serializer.h>
#include <slicer/slicer.h>
#include <stdexcept>
#include <string_view>
#include <utility>

namespace IceSpider::Compile {
	using namespace AdHoc::literals;

	RouteCompiler::RouteCompiler()
	{
		searchPath.push_back(std::filesystem::current_path());
	}

	RouteConfigurationPtr
	RouteCompiler::loadConfiguration(const std::filesystem::path & input) const
	{
		auto deserializer = Slicer::DeserializerPtr(
				Slicer::FileDeserializerFactory::createNew(input.extension().string(), input));
		return Slicer::DeserializeAnyWith<RouteConfigurationPtr>(deserializer);
	}

	Ice::StringSeq
	operator+(Ice::StringSeq ss, const std::string & s)
	{
		ss.push_back(s);
		return ss;
	}

	Slice::OperationPtr
	// NOLINTNEXTLINE(misc-no-recursion)
	RouteCompiler::findOperation(const std::string & on, const Slice::ContainerPtr & c, const Ice::StringSeq & ns)
	{
		for (const auto & cls : c->classes()) {
			auto fqcn = ns + cls->name();
			for (const auto & op : cls->allOperations()) {
				if (boost::algorithm::join(fqcn + op->name(), ".") == on) {
					return op;
				}
			}
		}
		for (const auto & m : c->modules()) {
			if (auto op = findOperation(on, m, ns + m->name())) {
				return op;
			}
		}
		return nullptr;
	}

	Slice::OperationPtr
	// NOLINTNEXTLINE(misc-no-recursion)
	RouteCompiler::findOperation(const std::string & on, const Units & us)
	{
		for (const auto & u : us) {
			if (auto op = findOperation(on, u.second)) {
				return op;
			}
		}
		throw std::runtime_error("Find operation " + on + " failed.");
	}

	std::optional<RouteCompiler::Type>
	// NOLINTNEXTLINE(misc-no-recursion)
	RouteCompiler::findType(const std::string & tn, const Slice::ContainerPtr & c, const Ice::StringSeq & ns)
	{
		for (const auto & strct : c->structs()) {
			auto fqon = boost::algorithm::join(ns + strct->name(), ".");
			if (fqon == tn) {
				return {{Slice::typeToString(strct), {}, strct->dataMembers()}};
			}
			auto t = findType(tn, strct, ns + strct->name());
		}
		for (const auto & cls : c->classes()) {
			auto fqon = boost::algorithm::join(ns + cls->name(), ".");
			if (fqon == tn) {
				return {{Slice::typeToString(cls->declaration()), cls->scoped(), cls->dataMembers()}};
			}
			if (auto t = findType(tn, cls, ns + cls->name())) {
				return t;
			}
		}
		for (const auto & m : c->modules()) {
			if (auto t = findType(tn, m, ns + m->name())) {
				return t;
			}
		}
		return {};
	}

	RouteCompiler::Type
	RouteCompiler::findType(const std::string & tn, const Units & us)
	{
		for (const auto & u : us) {
			if (auto t = findType(tn, u.second)) {
				return *t;
			}
		}
		throw std::runtime_error("Find type " + tn + " failed.");
	}

	RouteCompiler::ParameterMap
	RouteCompiler::findParameters(const RoutePtr & r, const Units & us)
	{
		RouteCompiler::ParameterMap pm;
		for (const auto & o : r->operations) {
			auto op = findOperation(o.second->operation, us);
			if (!op) {
				throw std::runtime_error("Find operator failed for " + r->path);
			}
			for (const auto & p : op->parameters()) {
				auto po = o.second->paramOverrides.find(p->name());
				if (po != o.second->paramOverrides.end()) {
					pm[po->second] = p;
				}
				else {
					pm[p->name()] = p;
				}
			}
		}
		return pm;
	}

	void
	RouteCompiler::applyDefaults(const RouteConfigurationPtr & c, const Units & u) const
	{
		for (const auto & r : c->routes) {
			if (r.second->operation) {
				r.second->operations[std::string()] = std::make_shared<Operation>(*r.second->operation, StringMap());
			}
			auto ps = findParameters(r.second, u);
			for (const auto & p : ps) {
				auto defined = r.second->params.find(p.first);
				if (defined != r.second->params.end()) {
					if (!defined->second->key && defined->second->source != ParameterSource::Body) {
						defined->second->key = defined->first;
					}
				}
				else {
					defined = r.second->params
									  .insert({p.first,
											  std::make_shared<Parameter>(ParameterSource::URL, p.first, false,
													  Ice::optional<std::string>(), Ice::optional<std::string>(),
													  false)})
									  .first;
				}
				auto d = defined->second;
				if (d->source == ParameterSource::URL) {
					Path path(r.second->path);
					d->hasUserSource = std::find_if(path.parts.begin(), path.parts.end(), [d](const auto & pp) {
						if (auto par = dynamic_cast<PathParameter *>(pp.get())) {
							return par->name == *d->key;
						}
						return false;
					}) != path.parts.end();
				}
			}
		}
	}

	void
	RouteCompiler::compile(const std::filesystem::path & input, const std::filesystem::path & output) const
	{
		auto configuration = loadConfiguration(input);
		auto units = loadUnits(configuration);
		auto outputh = std::filesystem::path(output).replace_extension(".h");
		applyDefaults(configuration, units);

		AdHoc::ScopeExit uDestroy([&units]() {
			for (auto & u : units) {
				u.second->destroy();
			}
		});

		using FilePtr = std::unique_ptr<FILE, decltype(&fclose)>;
		const auto out = FilePtr {fopen(output.c_str(), "w"), &fclose};
		const auto outh = FilePtr {fopen(outputh.c_str(), "w"), &fclose};
		if (!out || !outh) {
			throw std::runtime_error("Failed to open output files");
		}
		AdHoc::ScopeExit outClose(nullptr, nullptr, [&output, &outputh]() {
			std::filesystem::remove(output);
			std::filesystem::remove(outputh);
		});
		processConfiguration(out.get(), outh.get(), output.stem().string(), configuration, units);
	}

	RouteCompiler::Units
	RouteCompiler::loadUnits(const RouteConfigurationPtr & c) const
	{
		RouteCompiler::Units units;
		AdHoc::ScopeExit uDestroy;
		for (const auto & slice : c->slices) {
			std::filesystem::path realSlice;
			for (const auto & p : searchPath) {
				if (std::filesystem::exists(p / slice)) {
					realSlice = p / slice;
					break;
				}
			}
			if (realSlice.empty()) {
				throw std::runtime_error("Could not find slice file");
			}
			std::vector<std::string> cppArgs;
			for (const auto & p : searchPath) {
				cppArgs.emplace_back("-I");
				cppArgs.emplace_back(p.string());
			}
			Slice::PreprocessorPtr icecpp = Slice::Preprocessor::create("IceSpider", realSlice.string(), cppArgs);
			FILE * cppHandle = icecpp->preprocess(false);

			if (!cppHandle) {
				throw std::runtime_error("Preprocess failed");
			}

			Slice::UnitPtr u = Slice::Unit::createUnit(false, false, false, false);
			uDestroy.onFailure.emplace_back([u]() {
				u->destroy();
			});

			int parseStatus = u->parse(realSlice.string(), cppHandle, false);

			if (!icecpp->close()) {
				throw std::runtime_error("Preprocess close failed");
			}

			if (parseStatus == EXIT_FAILURE) {
				throw std::runtime_error("Unit parse failed");
			}

			units[slice] = u;
		}
		return units;
	}

	template<typename... X>
	void
	fprintbf(unsigned int indent, FILE * output, const X &... x)
	{
		for (; indent > 0; --indent) {
			fputc('\t', output);
		}
		// NOLINTNEXTLINE(hicpp-no-array-decay)
		fprintbf(output, x...);
	}

	template<typename Enum>
	std::string
	getEnumString(Enum & e)
	{
		return Slicer::ModelPartForEnum<Enum>::lookup(e);
	}

	static std::string
	outputSerializerClass(const IceSpider::OutputSerializers::value_type & os)
	{
		return boost::algorithm::replace_all_copy(os.second->serializer, ".", "::");
	}

	AdHocFormatter(MimePair, R"C({ "%?", "%?" })C");

	static std::string
	outputSerializerMime(const IceSpider::OutputSerializers::value_type & os)
	{
		auto slash = os.first.find('/');
		return MimePair::get(os.first.substr(0, slash), os.first.substr(slash + 1));
	}

	void
	RouteCompiler::registerOutputSerializers(FILE * output, const RoutePtr & r) const
	{
		for (const auto & os : r->outputSerializers) {
			fprintbf(4, output, "addRouteSerializer(%s,\n", outputSerializerMime(os));
			fprintbf(6, output, "std::make_shared<%s::IceSpiderFactory>(", outputSerializerClass(os));
			for (auto p = os.second->params.begin(); p != os.second->params.end(); ++p) {
				if (p != os.second->params.begin()) {
					fputs(", ", output);
				}
				fputs(p->c_str(), output);
			}
			fputs("));\n", output);
		}
	}

	void
	RouteCompiler::processConfiguration(FILE * output, FILE * outputh, const std::string & name,
			const RouteConfigurationPtr & c, const Units & units) const
	{
		fputs("// This source files was generated by IceSpider.\n", output);
		fprintbf(output, "// Configuration name: %s\n\n", c->name);

		fputs("#pragma once\n", outputh);
		fputs("// This source files was generated by IceSpider.\n", outputh);
		fprintbf(outputh, "// Configuration name: %s\n\n", c->name);

		fputs("// Configuration headers.\n", output);
		fprintbf(output, "#include \"%s.h\"\n", name);

		fputs("\n// Interface headers.\n", output);
		fputs("\n// Interface headers.\n", outputh);
		for (const auto & s : c->slices) {
			std::filesystem::path slicePath(s);
			slicePath.replace_extension(".h");
			fprintbf(output, "#include <%s>\n", slicePath.string());
			fprintbf(outputh, "#include <%s>\n", slicePath.string());
		}

		fputs("\n// Standard headers.\n", outputh);
		fputs("namespace IceSpider {\n", outputh);
		fputs("\tclass Core; // IWYU pragma: keep\n", outputh);
		fputs("\tclass IHttpRequest; // IWYU pragma: keep\n", outputh);
		fputs("}\n", outputh);

		fputs("// Standard headers.\n", output);
		for (const auto header : {
					 "core.h",
					 "irouteHandler.h",
					 "Ice/Config.h",
					 "Ice/Exception.h",
					 "Ice/Proxy.h",
					 "future",
					 "ios",
					 "memory",
					 "string",
					 "string_view",
					 "factory.h",
					 "http.h",
					 "ihttpRequest.h",
					 "util.h",
			 }) {
			fprintbf(output, "#include <%s>\n", header);
		}

		if (!c->headers.empty()) {
			fputs("\n// Extra headers.\n", output);
			fputs("\n// Extra headers.\n", outputh);
			for (const auto & h : c->headers) {
				fprintbf(output, "#include <%s> // IWYU pragma: keep\n", h);
				fprintbf(outputh, "#include <%s> // IWYU pragma: keep\n", h);
			}
		}

		processBases(output, outputh, c, units);
		processRoutes(output, c, units);

		fputs("\n// End generated code.\n", output);
	}

	void
	RouteCompiler::processBases(FILE * output, FILE * outputh, const RouteConfigurationPtr & c, const Units & u) const
	{
		fputs("\n", outputh);
		fprintbf(outputh, "namespace %s {\n", c->name);
		fprintbf(1, outputh, "// Base classes.\n\n");
		fputs("\n", output);
		fprintbf(output, "namespace %s {\n", c->name);
		fprintbf(1, output, "// Base classes.\n\n");
		for (const auto & r : c->routeBases) {
			processBase(output, outputh, r, u);
		}
		fprintbf(output, "} // namespace %s\n\n", c->name);
		fprintbf(outputh, "} // namespace %s\n\n", c->name);
	}

	void
	RouteCompiler::processBase(FILE * output, FILE * outputh, const RouteBases::value_type & b, const Units &) const
	{
		fprintbf(1, outputh, "class %s {\n", b.first);
		fprintbf(2, outputh, "protected:\n");
		fprintbf(3, outputh, "explicit %s(const IceSpider::Core * core);\n\n", b.first);
		for (const auto & f : b.second->functions) {
			fprintbf(3, outputh, "%s;\n", f);
		}
		fprintbf(1, output, "%s::%s(const IceSpider::Core * core)", b.first, b.first);
		if (!b.second->proxies.empty()) {
			fputs(" :", output);
		}
		fputs("\n", output);
		unsigned int pn = 0;
		for (const auto & p : b.second->proxies) {
			fprintbf(3, outputh, "const %sPrxPtr prx%u;\n", boost::algorithm::replace_all_copy(p, ".", "::"), pn);
			fprintbf(3, output, "prx%u(core->getProxy<%s>())", pn, boost::algorithm::replace_all_copy(p, ".", "::"));
			if (++pn < b.second->proxies.size()) {
				fputs(",", output);
			}
			fputs("\n", output);
		}
		fprintbf(1, outputh, "}; // %s\n", b.first);
		fprintbf(1, output, "{ }\n");
	}

	void
	RouteCompiler::processRoutes(FILE * output, const RouteConfigurationPtr & c, const Units & u) const
	{
		fputs("\n", output);
		fprintbf(output, "namespace %s {\n", c->name);
		fprintbf(1, output, "// Implementation classes.\n\n");
		for (const auto & r : c->routes) {
			processRoute(output, r, u);
		}
		fprintbf(output, "} // namespace %s\n\n", c->name);
		fputs("// Register route handlers.\n", output);
		for (const auto & r : c->routes) {
			fprintbf(output, "FACTORY(%s::%s, IceSpider::RouteHandlerFactory);\n", c->name, r.first);
		}
	}

	void
	RouteCompiler::processRoute(FILE * output, const Routes::value_type & r, const Units & units) const
	{
		std::string methodName = getEnumString(r.second->method);

		fprintbf(1, output, "// Route name: %s\n", r.first);
		fprintbf(1, output, "//       path: %s\n", r.second->path);
		fprintbf(1, output, "class %s : public IceSpider::IRouteHandler", r.first);
		for (const auto & b : r.second->bases) {
			fputs(",\n", output);
			fprintbf(3, output, "public %s", b);
		}
		fputs(" {\n", output);
		fprintbf(2, output, "public:\n");
		fprintbf(3, output, "explicit %s(const IceSpider::Core * core) :\n", r.first);
		fprintbf(4, output, "IceSpider::IRouteHandler(IceSpider::HttpMethod::%s, \"%s\")", methodName, r.second->path);
		for (const auto & b : r.second->bases) {
			fputs(",\n", output);
			fprintbf(4, output, "%s(core)", b);
		}
		auto proxies = initializeProxies(output, r.second);
		fputs("\n", output);
		fprintbf(3, output, "{\n");
		registerOutputSerializers(output, r.second);
		fprintbf(3, output, "}\n\n");
		fprintbf(3, output, "void execute(IceSpider::IHttpRequest * request) const override\n");
		fprintbf(3, output, "{\n");
		auto ps = findParameters(r.second, units);
		bool doneBody = false;
		for (const auto & p : r.second->params) {
			if (p.second->hasUserSource) {
				auto ip = ps.find(p.first)->second;
				const auto paramType = Slice::typeToString(ip->type(), false, "", ip->getMetaData());
				// This shouldn't be needed... the warning is ignored elsewhere to no effect
				if (p.second->source == ParameterSource::Body) {
					if (p.second->key) {
						if (!doneBody) {
							if (p.second->type) {
								fprintbf(4, output, "const auto _pbody(request->getBody<%s>());\n", *p.second->type);
							}
							else {
								fprintbf(4, output, "const auto _pbody(request->getBody<IceSpider::StringMap>());\n");
							}
							doneBody = true;
						}
						if (p.second->type) {
							fprintbf(4, output, "const auto _p_%s(_pbody->%s", p.first, p.first);
						}
						else {
							fprintbf(4, output, "const auto _p_%s(request->getBodyParam<%s>(_pbody, _pn_%s)", p.first,
									paramType, p.first);
						}
					}
					else {
						fprintbf(4, output, "const auto _p_%s(request->getBody<%s>()", p.first, paramType);
					}
				}
				else {
					fprintbf(4, output, "const auto _p_%s(request->get%sParam<%s>(_p%c_%s)", p.first,
							getEnumString(p.second->source), paramType,
							p.second->source == ParameterSource::URL ? 'i' : 'n', p.first);
				}
				if (!p.second->isOptional && p.second->source != ParameterSource::URL) {
					fprintbf(0, output, " /\n");
					if (p.second->defaultExpr) {
						fprintbf(5, output, " [this]() { return _pd_%s; }", p.first);
					}
					else {
						fprintbf(5, output, " [this]() { return requiredParameterNotFound<%s>(\"%s\", _pn_%s); }",
								paramType, getEnumString(p.second->source), p.first);
					}
				}
				fprintbf(0, output, ");\n");
			}
		}
		if (r.second->operation) {
			addSingleOperation(output, r.second, findOperation(*r.second->operation, units));
		}
		else {
			addMashupOperations(output, r.second, proxies, units);
		}
		fprintbf(3, output, "}\n\n");
		fprintbf(2, output, "private:\n");
		declareProxies(output, proxies);
		for (const auto & p : r.second->params) {
			if (p.second->hasUserSource) {
				if (p.second->source == ParameterSource::URL) {
					Path path(r.second->path);
					long idx = -1;
					for (const auto & pp : path.parts) {
						if (auto par = dynamic_cast<PathParameter *>(pp.get())) {
							if (par->name == *p.second->key) {
								idx = &pp - &path.parts.front();
							}
						}
					}
					fprintbf(3, output, "static constexpr unsigned int _pi_%s{%ld};\n", p.first, idx);
				}
				else {
					fprintbf(3, output, "static constexpr std::string_view _pn_%s{", p.first);
					if (p.second->key) {
						fprintbf(output, "\"%s\"", *p.second->key);
					}
					fputs("};\n", output);
				}
			}
			if (p.second->defaultExpr) {
				auto ip = ps.find(p.first)->second;
				fprintbf(3, output, "static const %s _pd_%s{%s};\n", Slice::typeToString(ip->type()), p.first,
						*p.second->defaultExpr);
			}
		}
		fprintbf(1, output, "};\n\n");
	}

	RouteCompiler::Proxies
	RouteCompiler::initializeProxies(FILE * output, const RoutePtr & r) const
	{
		Proxies proxies;
		int n = 0;
		for (const auto & o : r->operations) {
			auto proxyName = o.second->operation.substr(0, o.second->operation.find_last_of('.'));
			if (proxies.find(proxyName) == proxies.end()) {
				proxies[proxyName] = n;
				fputs(",\n", output);
				fprintbf(4, output, "prx%d(core->getProxy<%s>())", n,
						boost::algorithm::replace_all_copy(proxyName, ".", "::"));
				n += 1;
			}
		}
		return proxies;
	}

	void
	RouteCompiler::declareProxies(FILE * output, const Proxies & proxies) const
	{
		for (const auto & p : proxies) {
			fprintbf(3, output, "const %sPrxPtr prx%d;\n", boost::algorithm::replace_all_copy(p.first, ".", "::"),
					p.second);
		}
	}

	void
	RouteCompiler::addSingleOperation(FILE * output, const RoutePtr & r, const Slice::OperationPtr & o) const
	{
		auto operation = r->operation->substr(r->operation->find_last_of('.') + 1);
		if (o->returnType()) {
			fprintbf(4, output, "auto _responseModel = prx0->%s(", operation);
		}
		else {
			fprintbf(4, output, "prx0->%s(", operation);
		}
		for (const auto & p : o->parameters()) {
			auto rp = *r->params.find(p->name());
			if (rp.second->hasUserSource) {
				fprintbf(output, "_p_%s, ", p->name());
			}
			else {
				fprintbf(output, "_pd_%s, ", p->name());
			}
		}
		fprintbf(output, "request->getContext());\n");
		for (const auto & mutator : r->mutators) {
			fprintbf(4, output, "%s(request, _responseModel);\n", mutator);
		}
		if (o->returnType()) {
			fprintbf(4, output, "request->response(this, _responseModel);\n");
		}
		else {
			fprintbf(4, output, "request->response(200, \"OK\");\n");
		}
	}

	void
	RouteCompiler::addMashupOperations(
			FILE * output, const RoutePtr & r, const Proxies & proxies, const Units & us) const
	{
		for (const auto & o : r->operations) {
			auto proxyName = o.second->operation.substr(0, o.second->operation.find_last_of('.'));
			auto operation = o.second->operation.substr(o.second->operation.find_last_of('.') + 1);
			fprintbf(4, output, "auto _ar_%s = prx%s->%sAsync(", o.first, proxies.find(proxyName)->second, operation);
			auto so = findOperation(o.second->operation, us);
			for (const auto & p : so->parameters()) {
				auto po = o.second->paramOverrides.find(p->name());
				auto rp = *r->params.find(po != o.second->paramOverrides.end() ? po->second : p->name());
				if (rp.second->hasUserSource) {
					fprintbf(output, "_p_%s, ", p->name());
				}
				else {
					fprintbf(output, "_pd_%s, ", p->name());
				}
			}
			fprintbf(output, "request->getContext());\n");
		}
		auto t = findType(r->type, us);
		fprintbf(4, output, "%s _responseModel", t.type);
		if (t.scoped) {
			fprintbf(4, output, " = std::make_shared<%s>()", *t.scoped);
		}
		fprintbf(4, output, ";\n");
		for (const auto & mi : t.members) {
			bool isOp = false;
			fprintbf(4, output, "_responseModel%s%s = ", t.scoped ? "->" : ".", mi->name());
			for (const auto & o : r->operations) {
				auto proxyName = o.second->operation.substr(0, o.second->operation.find_last_of('.'));
				auto operation = o.second->operation.substr(o.second->operation.find_last_of('.') + 1);
				if (mi->name() == o.first) {
					fprintbf(output, "_ar_%s.get()", o.first);
					isOp = true;
					break;
				}
			}
			if (!isOp) {
				fprintbf(output, "_p_%s", mi->name());
			}
			fputs(";\n", output);
		}
		for (const auto & mutator : r->mutators) {
			fprintbf(4, output, "%s(request, _responseModel);\n", mutator);
		}
		fprintbf(4, output, "request->response(this, _responseModel);\n");
	}
}
