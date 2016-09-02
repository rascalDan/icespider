#include "routeCompiler.h"
#include <pathparts.h>
#include <slicer/slicer.h>
#include <slicer/modelPartsTypes.h>
#include <Slice/Preprocessor.h>
#include <scopeExit.h>
#include <fprintbf.h>
#include <boost/filesystem/convenience.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/algorithm/string/join.hpp>
#include <Slice/CPlusPlusUtil.h>

namespace IceSpider {
	namespace Compile {
		RouteCompiler::RouteCompiler()
		{
			searchPath.push_back(boost::filesystem::current_path());
		}

		RouteConfigurationPtr
		RouteCompiler::loadConfiguration(const boost::filesystem::path & input) const
		{
			auto deserializer = Slicer::DeserializerPtr(Slicer::FileDeserializerFactory::createNew(input.extension().string(), input));
			return Slicer::DeserializeAnyWith<RouteConfigurationPtr>(deserializer);
		}

		Ice::StringSeq operator+(Ice::StringSeq ss, const std::string & s)
		{
			ss.push_back(s);
			return ss;
		}

		Slice::OperationPtr
		RouteCompiler::findOperation(const std::string & on, const Slice::ContainerPtr & c, const Ice::StringSeq & ns)
		{
			for (const auto & cls : c->classes()) {
				auto fqcn = ns + cls->name();
				for (const auto & op : cls->allOperations()) {
					auto fqon = boost::algorithm::join(fqcn + op->name(), ".");
					if (fqon == on) {
						return op;
					}
				}
			}
			for (const auto & m : c->modules()) {
				auto op = findOperation(on, m, ns + m->name());
				if (op) return op;
			}
			return NULL;
		}

		Slice::OperationPtr
		RouteCompiler::findOperation(const std::string & on, const Units & us)
		{
			for (const auto & u : us) {
				auto op = findOperation(on, u.second);
				if (op) return op;
			}
			throw std::runtime_error("Find operation " + on + " failed.");
		}

		RouteCompiler::Type
		RouteCompiler::findType(const std::string & tn, const Slice::ContainerPtr & c, const Ice::StringSeq & ns)
		{
			for (const auto & strct : c->structs()) {
				auto fqon = boost::algorithm::join(ns + strct->name(), ".");
				if (fqon == tn) return { strct, NULL };
				auto t = findType(tn, strct, ns + strct->name());
			}
			for (const auto & cls : c->classes()) {
				auto fqon = boost::algorithm::join(ns + cls->name(), ".");
				if (fqon == tn) return { NULL, cls->declaration() };
				auto t = findType(tn, cls, ns + cls->name());
				if (t.first || t.second) return t;
			}
			for (const auto & m : c->modules()) {
				auto t = findType(tn, m, ns + m->name());
				if (t.first || t.second) return t;
			}
			return { NULL, NULL };
		}

		RouteCompiler::Type
		RouteCompiler::findType(const std::string & tn, const Units & us)
		{
			for (const auto & u : us) {
				auto t = findType(tn, u.second);
				if (t.first || t.second) return t;
			}
			throw std::runtime_error("Find type " + tn + " failed.");
		}

		RouteCompiler::ParameterMap
		RouteCompiler::findParameters(RoutePtr r, const Units & us)
		{
			RouteCompiler::ParameterMap pm;
			for (const auto & o : r->operations) {
				auto op = findOperation(o.second->operation, us);
				if (!op) {
					throw std::runtime_error("Find operator failed for " + r->name);
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
		RouteCompiler::applyDefaults(RouteConfigurationPtr c, const Units & u) const
		{
			for (const auto & r : c->routes) {
				if (r->operation) {
					r->operations[std::string()] = new Operation(*r->operation, {});
				}
				auto ps = findParameters(r, u);
				for (const auto & p : ps) {
					auto defined = std::find_if(r->params.begin(), r->params.end(), [p](const auto & rp) {
						return p.first == rp->name;
					});
					if (defined != r->params.end()) {
						auto d = *defined;
						if (!d->key) d->key = d->name;
					}
					else {
						r->params.push_back(new Parameter(p.first, ParameterSource::URL, p.first, false, IceUtil::Optional<std::string>(), false));
						defined = --r->params.end();
					}
					auto d = *defined;
					if (d->source == ParameterSource::URL) {
						Path path(r->path);
						d->hasUserSource = std::find_if(path.parts.begin(), path.parts.end(), [d](const auto & pp) {
							if (auto par = dynamic_cast<PathParameter *>(pp.get())) {
								return par->name == d->key;
							}
							return false;
						}) != path.parts.end();
					}
				}
			}
		}

		void
		RouteCompiler::compile(const boost::filesystem::path & input, const boost::filesystem::path & output) const
		{
			auto configuration = loadConfiguration(input);
			auto units = loadUnits(configuration);
			applyDefaults(configuration, units);

			AdHoc::ScopeExit uDestroy([&units]() {
				for (auto & u : units) {
					u.second->destroy();
				}
			});

			FILE * out = fopen(output.c_str(), "w");
			if (!out) {
				throw std::runtime_error("Failed to open output file");
			}
			AdHoc::ScopeExit outClose(
				[out]() { fclose(out); },
				NULL,
				[&output]() { boost::filesystem::remove(output); });
			processConfiguration(out, configuration, units);
		}

		RouteCompiler::Units
		RouteCompiler::loadUnits(RouteConfigurationPtr c) const
		{
			RouteCompiler::Units units;
			AdHoc::ScopeExit uDestroy;
			for (const auto & slice : c->slices) {
				boost::filesystem::path realSlice;
				for (const auto & p : searchPath) {
					if (boost::filesystem::exists(p / slice)) {
						realSlice = p / slice;
						break;
					}
				}
				if (realSlice.empty()) {
					throw std::runtime_error("Could not find slice file");
				}
				std::vector<std::string> cppArgs;
				for (const auto & p : searchPath) {
					cppArgs.push_back("-I");
					cppArgs.push_back(p.string());
				}
				Slice::PreprocessorPtr icecpp = Slice::Preprocessor::create("IceSpider", realSlice.string(), cppArgs);
				FILE * cppHandle = icecpp->preprocess(false);

				if (cppHandle == NULL) {
					throw std::runtime_error("Preprocess failed");
				}

				Slice::UnitPtr u = Slice::Unit::createUnit(false, false, false, false);
				uDestroy.onFailure.push_back([u]() { u->destroy(); });

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

		template<typename ... X>
		void
		fprintbf(unsigned int indent, FILE * output, const X & ... x)
		{
			for (; indent > 0; --indent) {
				fprintf(output, "\t");
			}
			fprintbf(output, x...);
		}

		template<typename Enum>
		std::string getEnumString(Enum & e)
		{
			return Slicer::ModelPartForEnum<Enum>::lookup(e);
		}

		void
		RouteCompiler::registerOutputSerializers(FILE * output, RoutePtr r) const
		{
			for (const auto & os : r->outputSerializers) {
				auto bs = boost::algorithm::replace_all_copy(os->serializer, ".", "::");
				auto slash = os->contentType.find('/');
				fprintbf(4, output, "addRouteSerializer({ \"%s\", \"%s\" }, new %s::IceSpiderFactory(",
						os->contentType.substr(0, slash), os->contentType.substr(slash + 1), bs);
				for (auto p = os->params.begin(); p != os->params.end(); ++p) {
					if (p != os->params.begin()) {
						fprintf(output, ", ");
					}
					fputs(p->c_str(), output);
				}
				fprintf(output, "));\n");
			}
		}

		void
		RouteCompiler::releaseOutputSerializers(FILE * output, RoutePtr r) const
		{
			for (const auto & os : r->outputSerializers) {
				auto slash = os->contentType.find('/');
				fprintbf(4, output, "removeRouteSerializer({ \"%s\", \"%s\" });\n",
						os->contentType.substr(0, slash), os->contentType.substr(slash + 1));
			}
		}

		void
		RouteCompiler::processConfiguration(FILE * output, RouteConfigurationPtr c, const Units & units) const
		{
			fprintf(output, "// This source files was generated by IceSpider.\n");
			fprintbf(output, "// Configuration name: %s\n\n", c->name);

			fprintf(output, "// Standard headers.\n");
			fprintf(output, "#include <irouteHandler.h>\n");
			fprintf(output, "#include <core.h>\n");
			fprintf(output, "#include <slicer/serializer.h>\n");

			fprintf(output, "\n// Interface headers.\n");
			for (const auto & s : c->slices) {
				boost::filesystem::path slicePath(s);
				slicePath.replace_extension(".h");
				fprintbf(output, "#include <%s>\n", slicePath.string());
			}

			if (!c->headers.empty()) {
				fprintf(output, "\n// Extra headers.\n");
				for (const auto & h : c->headers) {
					fprintbf(output, "#include <%s>\n", h);
				}
			}

			fprintf(output, "\n");
			fprintbf(output, "namespace %s {\n", c->name);
			fprintbf(1, output, "// Implementation classes.\n\n");
			for (const auto & r : c->routes) {
				std::string methodName = getEnumString(r->method);

				fprintbf(1, output, "// Route name: %s\n", r->name);
				fprintbf(1, output, "//       path: %s\n", r->path);
				fprintbf(1, output, "class %s : public IceSpider::IRouteHandler {\n", r->name);
				fprintbf(2, output, "public:\n");
				fprintbf(3, output, "%s(const IceSpider::Core * core) :\n", r->name);
				fprintbf(4, output, "IceSpider::IRouteHandler(IceSpider::HttpMethod::%s, \"%s\")", methodName, r->path);
				auto proxies = initializeProxies(output, r);
				for (const auto & p : r->params) {
					if (p->hasUserSource) {
						fprintf(output, ",\n");
						if (p->source == ParameterSource::URL) {
							Path path(r->path);
							unsigned int idx = -1;
							for (const auto & pp : path.parts) {
								if (auto par = dynamic_cast<PathParameter *>(pp.get())) {
									if (par->name == p->key) {
										idx = &pp - &path.parts.front();
									}
								}
							};
							fprintbf(4, output, "_pi_%s(%d)", p->name, idx);
						}
						else {
							fprintbf(4, output, "_pn_%s(\"%s\")", p->name, *p->key);
						}
					}
					if (p->defaultExpr) {
						fprintf(output, ",\n");
						fprintbf(4, output, "_pd_%s(%s)",
								p->name, p->defaultExpr.get());
					}
				}
				fprintf(output, "\n");
				fprintbf(3, output, "{\n");
				registerOutputSerializers(output, r);
				fprintbf(3, output, "}\n\n");
				fprintbf(3, output, "~%s()\n", r->name);
				fprintbf(3, output, "{\n");
				releaseOutputSerializers(output, r);
				fprintbf(3, output, "}\n\n");
				fprintbf(3, output, "void execute(IceSpider::IHttpRequest * request) const\n");
				fprintbf(3, output, "{\n");
				auto ps = findParameters(r, units);
				for (const auto & p : r->params) {
					if (p->hasUserSource) {
						auto ip = ps.find(p->name)->second;
						fprintbf(4, output, "auto _p_%s(request->get%sParam<%s>(_p%c_%s)",
										 p->name, getEnumString(p->source), Slice::typeToString(ip->type()),
										 p->source == ParameterSource::URL ? 'i' : 'n',
										 p->name);
						if (!p->isOptional && p->source != ParameterSource::URL) {
							fprintbf(0, output, " /\n");
							if (p->defaultExpr) {
								fprintbf(5, output, " [this]() { return _pd_%s; }",
												 p->name);
							}
							else {
								fprintbf(5, output, " [this]() { return requiredParameterNotFound<%s>(\"%s\", _pn_%s); }",
												 Slice::typeToString(ip->type()), getEnumString(p->source), p->name);
							}
						}
						fprintbf(0, output, ");\n");
					}
				}
				if (r->operation) {
					addSingleOperation(output, r, findOperation(*r->operation, units));
				}
				else {
					addMashupOperations(output, r, proxies, units);
				}
				fprintbf(3, output, "}\n\n");
				fprintbf(2, output, "private:\n");
				declareProxies(output, proxies);
				for (const auto & p : r->params) {
					if (p->hasUserSource) {
						if (p->source == ParameterSource::URL) {
							fprintbf(3, output, "const unsigned int _pi_%s;\n", p->name);
						}
						else {
							fprintbf(3, output, "const std::string _pn_%s;\n", p->name);
						}
					}
					if (p->defaultExpr) {
						auto ip = ps.find(p->name)->second;
						fprintbf(3, output, "const %s _pd_%s;\n",
								Slice::typeToString(ip->type()), p->name);

					}
				}
				fprintbf(1, output, "};\n\n");
			}
			fprintbf(output, "} // namespace %s\n\n", c->name);
			fprintf(output, "// Register route handlers.\n");
			for (const auto & r : c->routes) {
				fprintbf(output, "FACTORY(%s::%s, IceSpider::RouteHandlerFactory);\n", c->name, r->name);
			}
			fprintf(output, "\n// End generated code.\n");
		}

		RouteCompiler::Proxies
		RouteCompiler::initializeProxies(FILE * output, RoutePtr r) const
		{
			Proxies proxies;
			int n = 0;
			for (const auto & o : r->operations) {
				auto proxyName = o.second->operation.substr(0, o.second->operation.find_last_of('.'));
				if (proxies.find(proxyName) == proxies.end()) {
					proxies[proxyName] = n;
					fprintf(output, ",\n");
					fprintbf(4, output, "prx%d(core->getProxy<%s>())", n, boost::algorithm::replace_all_copy(proxyName, ".", "::"));
					n += 1;
				}
			}	
			return proxies;
		}

		void
		RouteCompiler::declareProxies(FILE * output, const Proxies & proxies) const
		{
			for (const auto & p : proxies) {
				fprintbf(3, output, "const %sPrx prx%d;\n", boost::algorithm::replace_all_copy(p.first, ".", "::"), p.second);
			}
		}

		void
		RouteCompiler::addSingleOperation(FILE * output, RoutePtr r, Slice::OperationPtr o) const
		{
			auto operation = r->operation->substr(r->operation->find_last_of('.') + 1);
			if (o->returnsData()) {
				fprintbf(4, output, "request->response(this, prx0->%s(", operation);
			}
			else {
				fprintbf(4, output, "prx0->%s(", operation);
			}
			for (const auto & p : o->parameters()) {
				auto rp = *std::find_if(r->params.begin(), r->params.end(), [p](const auto & rp) {
					return rp->name == p->name();
				});
				if (rp->hasUserSource) {
					fprintbf(output, "_p_%s, ", p->name());
				}
				else {
					fprintbf(output, "_pd_%s, ", p->name());
				}
			}
			fprintbf(output, "request->getContext())");
			if (o->returnsData()) {
				fprintbf(output, ")");
			}
			fprintbf(output, ";\n");
			if (!o->returnsData()) {
				fprintbf(4, output, "request->response(200, \"OK\");\n");
			}
		}

		void
		RouteCompiler::addMashupOperations(FILE * output, RoutePtr r, const Proxies & proxies, const Units & us) const
		{
			for (const auto & o : r->operations) {
				auto proxyName = o.second->operation.substr(0, o.second->operation.find_last_of('.'));
				auto operation = o.second->operation.substr(o.second->operation.find_last_of('.') + 1);
				fprintbf(4, output, "auto _ar_%s = prx%s->begin_%s(", o.first, proxies.find(proxyName)->second, operation);
				auto so = findOperation(o.second->operation, us);
				for (const auto & p : so->parameters()) {
					auto po = o.second->paramOverrides.find(p->name());
					fprintbf(output, "_p_%s, ", (po != o.second->paramOverrides.end() ? po->second : p->name()));
				}
				fprintbf(output, "request->getContext());\n");
			}
			auto t = findType(r->type, us);
			Slice::DataMemberList members;
			if (t.second) {
				fprintbf(4, output, "request->response<%s>(this, new %s(",
						Slice::typeToString(t.second),
						t.second->scoped());
				members = t.second->definition()->dataMembers();
			}
			else {
				fprintbf(4, output, "request->response<%s>(this, {",
						Slice::typeToString(t.first));
				members = t.first->dataMembers();
			}
			for (auto mi = members.begin(); mi != members.end(); mi++) {
				for (const auto & o : r->operations) {
					auto proxyName = o.second->operation.substr(0, o.second->operation.find_last_of('.'));
					auto operation = o.second->operation.substr(o.second->operation.find_last_of('.') + 1);
					if ((*mi)->name() == o.first) {
						if (mi != members.begin()) {
							fprintbf(output, ",");
						}
						fprintbf(output, "\n");
						fprintbf(6, output, "prx%s->end_%s(_ar_%s)", proxies.find(proxyName)->second, operation, o.first);
					}
				}
			}
			if (t.second) {
				fprintf(output, ")");
			}
			else {
				fprintf(output, " }");
			}
			fprintf(output, ");\n");
		}
	}
}

