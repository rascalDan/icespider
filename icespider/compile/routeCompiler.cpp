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
		class StringValue : public Slicer::ValueTarget {
			public:
				StringValue(std::string & t) : target(t) { }
				void get(const bool &) const override { }
				void get(const Ice::Byte &) const override { }
				void get(const Ice::Short &) const override { }
				void get(const Ice::Int &) const override { }
				void get(const Ice::Long &) const override { }
				void get(const Ice::Float &) const override { }
				void get(const Ice::Double &) const override { }
				void get(const std::string & v) const override { target = v; }
				std::string & target;
		};

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
		RouteCompiler::findOperation(RoutePtr r, const Slice::ContainerPtr & c, const Ice::StringSeq & ns)
		{
			for (const auto & cls : c->classes()) {
				auto fqcn = ns + cls->name();
				for (const auto & op : cls->allOperations()) {
					auto fqon = boost::algorithm::join(fqcn + op->name(), ".");
					if (fqon == r->operation) {
						return op;
					}
				}
			}
			for (const auto & m : c->modules()) {
				auto op = findOperation(r, m, ns + m->name());
				if (op) return op;
			}
			return NULL;
		}

		Slice::OperationPtr
		RouteCompiler::findOperation(RoutePtr r, const Units & us)
		{
			for (const auto & u : us) {
				auto op = findOperation(r, u.second);
				if (op) return op;
			}
			throw std::runtime_error("Find operator failed for " + r->name);
		}

		void
		RouteCompiler::applyDefaults(RouteConfigurationPtr c, const Units & u) const
		{
			for (const auto & r : c->routes) {
				auto o = findOperation(r, u);
				for (const auto & p : o->parameters()) {
					auto defined = std::find_if(r->params.begin(), r->params.end(), [p](const auto & rp) {
						return p->name() == rp->name;
					});
					if (defined != r->params.end()) {
						auto d = *defined;
						if (!d->key) d->key = d->name;
					}
					else {
						r->params.push_back(new Parameter(p->name(), ParameterSource::URL, p->name(), false, IceUtil::Optional<std::string>(), false));
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
			std::string rtn;
			Slicer::ModelPart::CreateFor<Enum>(e)->GetValue(new StringValue(rtn));
			return rtn;
		}

		void
		RouteCompiler::processConfiguration(FILE * output, RouteConfigurationPtr c, const Units & units) const
		{
			fprintf(output, "// This source files was generated by IceSpider.\n");
			fprintbf(output, "// Configuration name: %s\n\n", c->name);

			fprintf(output, "// Standard headers.\n");
			fprintf(output, "#include <irouteHandler.h>\n");

			fprintf(output, "\n// Interface headers.\n");
			for (const auto & s : c->slices) {
				boost::filesystem::path slicePath(s);
				slicePath.replace_extension(".h");
				fprintbf(output, "#include <%s>\n", slicePath.string());
			}

			fprintf(output, "\n");
			fprintbf(output, "namespace %s {\n", c->name);
			fprintbf(1, output, "// Implementation classes.\n\n");
			for (const auto & r : c->routes) {
				auto proxyName = r->operation.substr(0, r->operation.find_last_of('.'));
				auto operation = r->operation.substr(r->operation.find_last_of('.') + 1);
				boost::algorithm::replace_all(proxyName, ".", "::");
				std::string methodName = getEnumString(r->method);

				fprintbf(1, output, "// Route name: %s\n", r->name);
				fprintbf(1, output, "//       path: %s\n", r->path);
				fprintbf(1, output, "class %s : public IceSpider::IRouteHandler {\n", r->name);
				fprintbf(2, output, "public:\n");
				fprintbf(3, output, "%s() :\n", r->name);
				fprintbf(4, output, "IceSpider::IRouteHandler(IceSpider::HttpMethod::%s, \"%s\")", methodName, r->path);
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
				fprintbf(3, output, "}\n\n");
				fprintbf(3, output, "void execute(IceSpider::IHttpRequest * request) const\n");
				fprintbf(3, output, "{\n");
				auto o = findOperation(r, units);
				for (const auto & p : r->params) {
					if (p->hasUserSource) {
						auto ip = *std::find_if(o->parameters().begin(), o->parameters().end(), [p](const auto & ip) { return ip->name() == p->name; });
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
				fprintbf(4, output, "auto prx = getProxy<%s>(request);\n", proxyName);
				if (o->returnsData()) {
					fprintbf(4, output, "request->response(this, prx->%s(", operation);
				}
				else {
					fprintbf(4, output, "prx->%s(", operation);
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
				fprintbf(3, output, "}\n\n");
				fprintbf(2, output, "private:\n");
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
						auto ip = *std::find_if(o->parameters().begin(), o->parameters().end(), [p](const auto & ip) { return ip->name() == p->name; });
						fprintbf(3, output, "const %s _pd_%s;\n",
								Slice::typeToString(ip->type()), p->name);

					}
				}
				fprintbf(1, output, "};\n\n");
			}
			fprintbf(output, "} // namespace %s\n\n", c->name);
			fprintf(output, "// Register route handlers.\n");
			for (const auto & r : c->routes) {
				fprintbf(output, "PLUGIN(%s::%s, IceSpider::IRouteHandler);\n", c->name, r->name);
			}
			fprintf(output, "\n// End generated code.\n");
		}
	}
}

