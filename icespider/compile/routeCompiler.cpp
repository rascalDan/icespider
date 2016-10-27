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
		RouteCompiler::RouteCompiler() :
			allowIcePrefix(false)
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
		RouteCompiler::applyDefaults(RouteConfigurationPtr c, const Units & u) const
		{
			for (const auto & r : c->routes) {
				if (r.second->operation) {
					r.second->operations[std::string()] = new Operation(*r.second->operation, {});
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
						defined = r.second->params.insert({ p.first, new Parameter(ParameterSource::URL, p.first, false, IceUtil::Optional<std::string>(), IceUtil::Optional<std::string>(), false) }).first;
					}
					auto d = defined->second;
					if (d->source == ParameterSource::URL) {
						Path path(r.second->path);
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
			auto outputh = boost::filesystem::change_extension(output, ".h");
			applyDefaults(configuration, units);

			AdHoc::ScopeExit uDestroy([&units]() {
				for (auto & u : units) {
					u.second->destroy();
				}
			});

			FILE * out = fopen(output.c_str(), "w");
			FILE * outh = fopen(outputh.c_str(), "w");
			if (!out || !outh) {
				throw std::runtime_error("Failed to open output files");
			}
			AdHoc::ScopeExit outClose(
				[out,outh]() {
					fclose(out);
					fclose(outh);
				},
				NULL,
				[&output,&outputh]() {
					boost::filesystem::remove(output);
					boost::filesystem::remove(outputh);
				});
			processConfiguration(out, outh, output.stem().string(), configuration, units);
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

				Slice::UnitPtr u = Slice::Unit::createUnit(false, false, allowIcePrefix, false);
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
				auto bs = boost::algorithm::replace_all_copy(os.second->serializer, ".", "::");
				auto slash = os.first.find('/');
				fprintbf(4, output, "addRouteSerializer({ \"%s\", \"%s\" }, new %s::IceSpiderFactory(",
						os.first.substr(0, slash), os.first.substr(slash + 1), bs);
				for (auto p = os.second->params.begin(); p != os.second->params.end(); ++p) {
					if (p != os.second->params.begin()) {
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
				auto slash = os.first.find('/');
				fprintbf(4, output, "removeRouteSerializer({ \"%s\", \"%s\" });\n",
						os.first.substr(0, slash), os.first.substr(slash + 1));
			}
		}

		void
		RouteCompiler::processConfiguration(FILE * output, FILE * outputh, const std::string & name, RouteConfigurationPtr c, const Units & units) const
		{
			fprintf(output, "// This source files was generated by IceSpider.\n");
			fprintbf(output, "// Configuration name: %s\n\n", c->name);

			fprintf(outputh, "// This source files was generated by IceSpider.\n");
			fprintbf(outputh, "// Configuration name: %s\n\n", c->name);

			fprintf(output, "// Configuration headers.\n");
			fprintbf(output, "#include \"%s.h\"\n", name);

			fprintf(outputh, "// Standard headers.\n");
			fprintf(outputh, "#include <irouteHandler.h>\n");
			fprintf(outputh, "#include <core.h>\n");
			fprintf(outputh, "#include <slicer/serializer.h>\n");

			fprintf(outputh, "\n// Interface headers.\n");
			for (const auto & s : c->slices) {
				boost::filesystem::path slicePath(s);
				slicePath.replace_extension(".h");
				fprintbf(outputh, "#include <%s>\n", slicePath.string());
			}

			if (!c->headers.empty()) {
				fprintf(outputh, "\n// Extra headers.\n");
				for (const auto & h : c->headers) {
					fprintbf(outputh, "#include <%s>\n", h);
				}
			}

			processBases(output, outputh, c, units);
			processRoutes(output, c, units);

			fprintf(output, "\n// End generated code.\n");
		}

		void
		RouteCompiler::processBases(FILE * output, FILE * outputh, RouteConfigurationPtr c, const Units & u) const
		{
			fprintf(outputh, "\n");
			fprintbf(outputh, "namespace %s {\n", c->name);
			fprintbf(1, outputh, "// Base classes.\n\n");
			fprintf(output, "\n");
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
			fprintbf(3, outputh, "%s(const IceSpider::Core * core);\n\n", b.first);
			for (const auto & f: b.second->functions) {
				fprintbf(3, outputh, "%s;\n", f);
			}
			fprintbf(1, output, "%s::%s(const IceSpider::Core * core)", b.first, b.first);
			if (!b.second->proxies.empty()) {
				fprintf(output, " :");
			}
			fprintf(output, "\n");
			unsigned int pn = 0;
			for (const auto & p : b.second->proxies) {
				fprintbf(3, outputh, "const %sPrx prx%u;\n",
						boost::algorithm::replace_all_copy(p, ".", "::"), pn);
				fprintbf(3, output, "prx%u(core->getProxy<%s>())",
						pn, boost::algorithm::replace_all_copy(p, ".", "::"));
				if (++pn < b.second->proxies.size()) {
					fprintf(output, ",");
				}
				fprintf(output, "\n");
			}
			fprintbf(1, outputh, "}; // %s\n", b.first);
			fprintbf(1, output, "{ }\n");
		}

		void
		RouteCompiler::processRoutes(FILE * output, RouteConfigurationPtr c, const Units & u) const
		{
			fprintf(output, "\n");
			fprintbf(output, "namespace %s {\n", c->name);
			fprintbf(1, output, "// Implementation classes.\n\n");
			for (const auto & r : c->routes) {
				processRoute(output, r, u);
			}
			fprintbf(output, "} // namespace %s\n\n", c->name);
			fprintf(output, "// Register route handlers.\n");
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
				fprintf(output, ",\n");
				fprintbf(3, output, "public %s", b);
			}
			fprintf(output, " {\n");
			fprintbf(2, output, "public:\n");
			fprintbf(3, output, "%s(const IceSpider::Core * core) :\n", r.first);
			fprintbf(4, output, "IceSpider::IRouteHandler(IceSpider::HttpMethod::%s, \"%s\")", methodName, r.second->path);
			for (const auto & b : r.second->bases) {
				fprintf(output, ",\n");
				fprintbf(4, output, "%s(core)", b);
			}
			auto proxies = initializeProxies(output, r.second);
			for (const auto & p : r.second->params) {
				if (p.second->hasUserSource) {
					if (p.second->source == ParameterSource::URL) {
						fprintf(output, ",\n");
						Path path(r.second->path);
						unsigned int idx = -1;
						for (const auto & pp : path.parts) {
							if (auto par = dynamic_cast<PathParameter *>(pp.get())) {
								if (par->name == p.second->key) {
									idx = &pp - &path.parts.front();
								}
							}
						};
						fprintbf(4, output, "_pi_%s(%d)", p.first, idx);
					}
					else {
						if (p.second->key) {
							fprintf(output, ",\n");
							fprintbf(4, output, "_pn_%s(\"%s\")", p.first, *p.second->key);
						}
					}
				}
				if (p.second->defaultExpr) {
					fprintf(output, ",\n");
					fprintbf(4, output, "_pd_%s(%s)",
							p.first, p.second->defaultExpr.get());
				}
			}
			fprintf(output, "\n");
			fprintbf(3, output, "{\n");
			registerOutputSerializers(output, r.second);
			fprintbf(3, output, "}\n\n");
			fprintbf(3, output, "~%s()\n", r.first);
			fprintbf(3, output, "{\n");
			releaseOutputSerializers(output, r.second);
			fprintbf(3, output, "}\n\n");
			fprintbf(3, output, "void execute(IceSpider::IHttpRequest * request) const\n");
			fprintbf(3, output, "{\n");
			auto ps = findParameters(r.second, units);
			bool doneBody = false;
			for (const auto & p : r.second->params) {
				if (p.second->hasUserSource) {
					auto ip = ps.find(p.first)->second;
					if (p.second->source == ParameterSource::Body) {
						if (p.second->key) {
							if (!doneBody) {
								if (p.second->type) {
									fprintbf(4, output, "auto _pbody(request->getBody<%s>());\n",
											*p.second->type);
								}
								else {
									fprintbf(4, output, "auto _pbody(request->getBody<IceSpider::StringMap>());\n");
								}
								doneBody = true;
							}
							if (p.second->type) {
								fprintbf(4, output, "auto _p_%s(_pbody->%s",
										p.first, p.first);
							}
							else {
								fprintbf(4, output, "auto _p_%s(request->getBodyParam<%s>(_pbody, _pn_%s)",
										p.first, Slice::typeToString(ip->type()),
										p.first);
							}
						}
						else {
							fprintbf(4, output, "auto _p_%s(request->getBody<%s>()",
									p.first, Slice::typeToString(ip->type()));
						}
					}
					else {
						fprintbf(4, output, "auto _p_%s(request->get%sParam<%s>(_p%c_%s)",
								p.first, getEnumString(p.second->source), Slice::typeToString(ip->type()),
								p.second->source == ParameterSource::URL ? 'i' : 'n',
								p.first);
					}
					if (!p.second->isOptional && p.second->source != ParameterSource::URL) {
						fprintbf(0, output, " /\n");
						if (p.second->defaultExpr) {
							fprintbf(5, output, " [this]() { return _pd_%s; }",
									p.first);
						}
						else {
							fprintbf(5, output, " [this]() { return requiredParameterNotFound<%s>(\"%s\", _pn_%s); }",
									Slice::typeToString(ip->type()), getEnumString(p.second->source), p.first);
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
						fprintbf(3, output, "const unsigned int _pi_%s;\n", p.first);
					}
					else {
						fprintbf(3, output, "const std::string _pn_%s;\n", p.first);
					}
				}
				if (p.second->defaultExpr) {
					auto ip = ps.find(p.first)->second;
					fprintbf(3, output, "const %s _pd_%s;\n",
							Slice::typeToString(ip->type()), p.first);

				}
			}
			fprintbf(1, output, "};\n\n");
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
			if (r->mutator) {
				fprintbf(4, output, "%s(request, _responseModel);\n", *r->mutator);
			}
			if (o->returnsData()) {
				fprintbf(4, output, "request->response(this, _responseModel);\n");
			}
			else {
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
			Slice::DataMemberList members;
			if (t.second) {
				fprintbf(4, output, "%s _responseModel = new %s();\n",
						Slice::typeToString(t.second),
						t.second->scoped());
				members = t.second->definition()->dataMembers();
			}
			else {
				fprintbf(4, output, "%s _responseModel;\n",
						Slice::typeToString(t.first));
				members = t.first->dataMembers();
			}
			for (const auto & mi : members) {
				bool isOp = false;
				fprintbf(4, output, "_responseModel%s%s = ",
						t.second ? "->" : ".",
						mi->name());
				for (const auto & o : r->operations) {
					auto proxyName = o.second->operation.substr(0, o.second->operation.find_last_of('.'));
					auto operation = o.second->operation.substr(o.second->operation.find_last_of('.') + 1);
					if (mi->name() == o.first) {
						fprintbf(output, "prx%s->end_%s(_ar_%s)", proxies.find(proxyName)->second, operation, o.first);
						isOp = true;
						break;
					}
				}
				if (!isOp) {
					fprintbf(output, "_p_%s", mi->name());
				}
				fprintf(output, ";\n");
			}
			if (r->mutator) {
				fprintbf(4, output, "%s(request, _responseModel);\n", *r->mutator);
			}
			fprintbf(4, output, "request->response(this, _responseModel);\n");
		}
	}
}

