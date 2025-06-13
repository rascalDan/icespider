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
	namespace {
		Ice::StringSeq
		operator+(Ice::StringSeq target, std::string value)
		{
			target.emplace_back(std::move(value));
			return target;
		}
	}

	using namespace AdHoc::literals;

	RouteCompiler::RouteCompiler()
	{
		searchPath.push_back(std::filesystem::current_path());
	}

	RouteConfigurationPtr
	RouteCompiler::loadConfiguration(const std::filesystem::path & input)
	{
		auto deserializer = Slicer::DeserializerPtr(
				Slicer::FileDeserializerFactory::createNew(input.extension().string(), input));
		return Slicer::DeserializeAnyWith<RouteConfigurationPtr>(deserializer);
	}

	Slice::OperationPtr
	// NOLINTNEXTLINE(misc-no-recursion)
	RouteCompiler::findOperation(
			const std::string & operationName, const Slice::ContainerPtr & container, const Ice::StringSeq & nameSpace)
	{
		for (const auto & cls : container->classes()) {
			auto fqcn = nameSpace + cls->name();
			for (const auto & operation : cls->allOperations()) {
				if (boost::algorithm::join(fqcn + operation->name(), ".") == operationName) {
					return operation;
				}
			}
		}
		for (const auto & module : container->modules()) {
			if (auto operation = findOperation(operationName, module, nameSpace + module->name())) {
				return operation;
			}
		}
		return nullptr;
	}

	Slice::OperationPtr
	// NOLINTNEXTLINE(misc-no-recursion)
	RouteCompiler::findOperation(const std::string & operationName, const Units & units)
	{
		for (const auto & unit : units) {
			if (auto operation = findOperation(operationName, unit.second)) {
				return operation;
			}
		}
		throw std::runtime_error("Find operation " + operationName + " failed.");
	}

	std::optional<RouteCompiler::Type>
	// NOLINTNEXTLINE(misc-no-recursion)
	RouteCompiler::findType(
			const std::string & typeName, const Slice::ContainerPtr & container, const Ice::StringSeq & nameSpace)
	{
		for (const auto & strct : container->structs()) {
			auto fqon = boost::algorithm::join(nameSpace + strct->name(), ".");
			if (fqon == typeName) {
				return {{.type = Slice::typeToString(strct), .scoped = {}, .members = strct->dataMembers()}};
			}
			// auto t = findType(typeName, strct, nameSpace + strct->name());
		}
		for (const auto & cls : container->classes()) {
			auto fqon = boost::algorithm::join(nameSpace + cls->name(), ".");
			if (fqon == typeName) {
				return {{.type = Slice::typeToString(cls->declaration()),
						.scoped = cls->scoped(),
						.members = cls->dataMembers()}};
			}
			if (auto type = findType(typeName, cls, nameSpace + cls->name())) {
				return type;
			}
		}
		for (const auto & module : container->modules()) {
			if (auto type = findType(typeName, module, nameSpace + module->name())) {
				return type;
			}
		}
		return {};
	}

	RouteCompiler::Type
	RouteCompiler::findType(const std::string & typeName, const Units & units)
	{
		for (const auto & unit : units) {
			if (auto type = findType(typeName, unit.second)) {
				return *type;
			}
		}
		throw std::runtime_error("Find type " + typeName + " failed.");
	}

	RouteCompiler::ParameterMap
	RouteCompiler::findParameters(const RoutePtr & route, const Units & units)
	{
		RouteCompiler::ParameterMap parameters;
		for (const auto & operationName : route->operations) {
			auto operation = findOperation(operationName.second->operation, units);
			if (!operation) {
				throw std::runtime_error("Find operation failed for " + route->path);
			}
			for (const auto & parameter : operation->parameters()) {
				auto parameterOverride = operationName.second->paramOverrides.find(parameter->name());
				if (parameterOverride != operationName.second->paramOverrides.end()) {
					parameters[parameterOverride->second] = parameter;
				}
				else {
					parameters[parameter->name()] = parameter;
				}
			}
		}
		return parameters;
	}

	void
	RouteCompiler::applyDefaults(const RouteConfigurationPtr & routeConfig, const Units & units)
	{
		for (const auto & route : routeConfig->routes) {
			if (route.second->operation) {
				route.second->operations[std::string()]
						= std::make_shared<Operation>(*route.second->operation, StringMap());
			}
			const auto parameters = findParameters(route.second, units);
			for (const auto & parameter : parameters) {
				auto defined = route.second->params.find(parameter.first);
				if (defined != route.second->params.end()) {
					if (!defined->second->key && defined->second->source != ParameterSource::Body) {
						defined->second->key = defined->first;
					}
				}
				else {
					defined = route.second->params
									  .insert({parameter.first,
											  std::make_shared<Parameter>(ParameterSource::URL, parameter.first, false,
													  Ice::optional<std::string>(), Ice::optional<std::string>(),
													  false)})
									  .first;
				}
				auto definition = defined->second;
				if (definition->source == ParameterSource::URL) {
					Path path(route.second->path);
					definition->hasUserSource
							= std::find_if(path.parts.begin(), path.parts.end(),
									  [definition](const auto & paramPtr) {
										  if (auto pathParam = dynamic_cast<PathParameter *>(paramPtr.get())) {
											  return pathParam->name == *definition->key;
										  }
										  return false;
									  })
							!= path.parts.end();
				}
			}
		}
	}

	template<auto Deleter>
	using DeleteWith = decltype([](auto ptr) {
		Deleter(ptr);
	});

	void
	RouteCompiler::compile(const std::filesystem::path & input, const std::filesystem::path & output) const
	{
		auto configuration = loadConfiguration(input);
		auto units = loadUnits(configuration);
		auto outputh = std::filesystem::path(output).replace_extension(".h");
		applyDefaults(configuration, units);

		AdHoc::ScopeExit uDestroy([&units]() {
			for (auto & unit : units) {
				unit.second->destroy();
			}
		});

		using FilePtr = std::unique_ptr<FILE, DeleteWith<fclose>>;
		const FilePtr out {fopen(output.c_str(), "w")};
		const FilePtr outh {fopen(outputh.c_str(), "w")};
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
	RouteCompiler::loadUnits(const RouteConfigurationPtr & routeConfig) const
	{
		RouteCompiler::Units units;
		AdHoc::ScopeExit uDestroy;
		for (const auto & slice : routeConfig->slices) {
			std::filesystem::path realSlice;
			for (const auto & path : searchPath) {
				if (std::filesystem::exists(path / slice)) {
					realSlice = path / slice;
					break;
				}
			}
			if (realSlice.empty()) {
				throw std::runtime_error("Could not find slice file");
			}
			std::vector<std::string> cppArgs;
			for (const auto & path : searchPath) {
				cppArgs.emplace_back("-I");
				cppArgs.emplace_back(path.string());
			}
			Slice::PreprocessorPtr icecpp = Slice::Preprocessor::create("IceSpider", realSlice.string(), cppArgs);
			FILE * cppHandle = icecpp->preprocess(false);

			if (!cppHandle) {
				throw std::runtime_error("Preprocess failed");
			}

			Slice::UnitPtr unit = Slice::Unit::createUnit(false, false, false, false);
			uDestroy.onFailure.emplace_back([unit]() {
				unit->destroy();
			});

			int parseStatus = unit->parse(realSlice.string(), cppHandle, false);

			if (!icecpp->close()) {
				throw std::runtime_error("Preprocess close failed");
			}

			if (parseStatus == EXIT_FAILURE) {
				throw std::runtime_error("Unit parse failed");
			}

			units[slice] = unit;
		}
		return units;
	}

	namespace {
		template<typename... Params>
		void
		fprintbf(unsigned int indent, FILE * output, Params &&... params)
		{
			for (; indent > 0; --indent) {
				fputc('\t', output);
			}
			// NOLINTNEXTLINE(hicpp-no-array-decay)
			fprintbf(output, std::forward<Params>(params)...);
		}

		template<typename Enum>
		std::string
		getEnumString(Enum & enumValue)
		{
			return Slicer::ModelPartForEnum<Enum>::lookup(enumValue);
		}

		std::string
		outputSerializerClass(const IceSpider::OutputSerializers::value_type & outputSerializer)
		{
			return boost::algorithm::replace_all_copy(outputSerializer.second->serializer, ".", "::");
		}

		AdHocFormatter(MimePair, R"C({ "%?", "%?" })C");

		std::string
		outputSerializerMime(const IceSpider::OutputSerializers::value_type & outputSerializer)
		{
			auto slash = outputSerializer.first.find('/');
			return MimePair::get(outputSerializer.first.substr(0, slash), outputSerializer.first.substr(slash + 1));
		}

		void
		processParameterSourceBody(
				FILE * output, const Parameters::value_type & param, bool & doneBody, std::string_view paramType)
		{
			if (param.second->key) {
				if (!doneBody) {
					if (param.second->type) {
						fprintbf(4, output, "const auto _pbody(request->getBody<%s>());\n", *param.second->type);
					}
					else {
						fprintbf(4, output, "const auto _pbody(request->getBody<IceSpider::StringMap>());\n");
					}
					doneBody = true;
				}
				if (param.second->type) {
					fprintbf(4, output, "const auto _p_%s(_pbody->%s", param.first, param.first);
				}
				else {
					fprintbf(4, output, "const auto _p_%s(request->getBodyParam<%s>(_pbody, _pn_%s)", param.first,
							paramType, param.first);
				}
			}
			else {
				fprintbf(4, output, "const auto _p_%s(request->getBody<%s>()", param.first, paramType);
			}
		}
	}

	void
	RouteCompiler::registerOutputSerializers(FILE * output, const RoutePtr & route)
	{
		for (const auto & outputSerializer : route->outputSerializers) {
			fprintbf(4, output, "addRouteSerializer(%s,\n", outputSerializerMime(outputSerializer));
			fprintbf(6, output, "std::make_shared<%s::IceSpiderFactory>(", outputSerializerClass(outputSerializer));
			for (auto parameter = outputSerializer.second->params.begin();
					parameter != outputSerializer.second->params.end(); ++parameter) {
				if (parameter != outputSerializer.second->params.begin()) {
					fputs(", ", output);
				}
				fputs(parameter->c_str(), output);
			}
			fputs("));\n", output);
		}
	}

	void
	RouteCompiler::processConfiguration(FILE * output, FILE * outputh, const std::string & name,
			const RouteConfigurationPtr & routeConfig, const Units & units)
	{
		fputs("// This source files was generated by IceSpider.\n", output);
		fprintbf(output, "// Configuration name: %s\n\n", routeConfig->name);

		fputs("#pragma once\n", outputh);
		fputs("// This source files was generated by IceSpider.\n", outputh);
		fprintbf(outputh, "// Configuration name: %s\n\n", routeConfig->name);

		fputs("// Configuration headers.\n", output);
		fprintbf(output, "#include \"%s.h\"\n", name);

		fputs("\n// Interface headers.\n", output);
		fputs("\n// Interface headers.\n", outputh);
		for (const auto & slice : routeConfig->slices) {
			std::filesystem::path slicePath(slice);
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

		if (!routeConfig->headers.empty()) {
			fputs("\n// Extra headers.\n", output);
			fputs("\n// Extra headers.\n", outputh);
			for (const auto & header : routeConfig->headers) {
				fprintbf(output, "#include <%s> // IWYU pragma: keep\n", header);
				fprintbf(outputh, "#include <%s> // IWYU pragma: keep\n", header);
			}
		}

		processBases(output, outputh, routeConfig, units);
		processRoutes(output, routeConfig, units);

		fputs("\n// End generated code.\n", output);
	}

	void
	RouteCompiler::processBases(
			FILE * output, FILE * outputh, const RouteConfigurationPtr & routeConfig, const Units & units)
	{
		fputs("\n", outputh);
		fprintbf(outputh, "namespace %s {\n", routeConfig->name);
		fprintbf(1, outputh, "// Base classes.\n\n");
		fputs("\n", output);
		fprintbf(output, "namespace %s {\n", routeConfig->name);
		fprintbf(1, output, "// Base classes.\n\n");
		for (const auto & route : routeConfig->routeBases) {
			processBase(output, outputh, route, units);
		}
		fprintbf(output, "} // namespace %s\n\n", routeConfig->name);
		fprintbf(outputh, "} // namespace %s\n\n", routeConfig->name);
	}

	void
	RouteCompiler::processBase(FILE * output, FILE * outputh, const RouteBases::value_type & route, const Units &)
	{
		fprintbf(1, outputh, "class %s {\n", route.first);
		fprintbf(2, outputh, "protected:\n");
		fprintbf(3, outputh, "explicit %s(const IceSpider::Core * core);\n\n", route.first);
		for (const auto & function : route.second->functions) {
			fprintbf(3, outputh, "%s;\n", function);
		}
		fprintbf(1, output, "%s::%s(const IceSpider::Core * core)", route.first, route.first);
		if (!route.second->proxies.empty()) {
			fputs(" :", output);
		}
		fputs("\n", output);
		unsigned int proxyNum = 0;
		for (const auto & proxy : route.second->proxies) {
			fprintbf(3, outputh, "const %sPrxPtr prx%u;\n", boost::algorithm::replace_all_copy(proxy, ".", "::"),
					proxyNum);
			fprintbf(3, output, "prx%u(core->getProxy<%s>())", proxyNum,
					boost::algorithm::replace_all_copy(proxy, ".", "::"));
			if (++proxyNum < route.second->proxies.size()) {
				fputs(",", output);
			}
			fputs("\n", output);
		}
		fprintbf(1, outputh, "}; // %s\n", route.first);
		fprintbf(1, output, "{ }\n");
	}

	void
	RouteCompiler::processRoutes(FILE * output, const RouteConfigurationPtr & routeConfig, const Units & units)
	{
		fputs("\n", output);
		fprintbf(output, "namespace %s {\n", routeConfig->name);
		fprintbf(1, output, "// Implementation classes.\n\n");
		for (const auto & route : routeConfig->routes) {
			processRoute(output, route, units);
		}
		fprintbf(output, "} // namespace %s\n\n", routeConfig->name);
		fputs("// Register route handlers.\n", output);
		for (const auto & route : routeConfig->routes) {
			fprintbf(output, "FACTORY(%s::%s, IceSpider::RouteHandlerFactory);\n", routeConfig->name, route.first);
		}
	}

	void
	RouteCompiler::processRoute(FILE * output, const Routes::value_type & route, const Units & units)
	{
		std::string methodName = getEnumString(route.second->method);

		fprintbf(1, output, "// Route name: %s\n", route.first);
		fprintbf(1, output, "//       path: %s\n", route.second->path);
		fprintbf(1, output, "class %s : public IceSpider::IRouteHandler", route.first);
		for (const auto & base : route.second->bases) {
			fputs(",\n", output);
			fprintbf(3, output, "public %s", base);
		}
		fputs(" {\n", output);
		fprintbf(2, output, "public:\n");
		fprintbf(3, output, "explicit %s(const IceSpider::Core * core) :\n", route.first);
		fprintbf(4, output, "IceSpider::IRouteHandler(IceSpider::HttpMethod::%s, \"%s\")", methodName,
				route.second->path);
		for (const auto & base : route.second->bases) {
			fputs(",\n", output);
			fprintbf(4, output, "%s(core)", base);
		}
		auto proxies = initializeProxies(output, route.second);
		fputs("\n", output);
		fprintbf(3, output, "{\n");
		registerOutputSerializers(output, route.second);
		fprintbf(3, output, "}\n\n");
		fprintbf(3, output, "void execute(IceSpider::IHttpRequest * request) const override\n");
		fprintbf(3, output, "{\n");
		auto parameters = findParameters(route.second, units);
		bool doneBody = false;
		for (const auto & param : route.second->params) {
			if (param.second->hasUserSource) {
				auto iceParamDecl = parameters.find(param.first)->second;
				const auto paramType
						= Slice::typeToString(iceParamDecl->type(), false, "", iceParamDecl->getMetaData());
				if (param.second->source == ParameterSource::Body) {
					processParameterSourceBody(output, param, doneBody, paramType);
				}
				else {
					fprintbf(4, output, "const auto _p_%s(request->get%sParam<%s>(_p%c_%s)", param.first,
							getEnumString(param.second->source), paramType,
							param.second->source == ParameterSource::URL ? 'i' : 'n', param.first);
				}
				if (!param.second->isOptional && param.second->source != ParameterSource::URL) {
					fprintbf(0, output, " /\n");
					if (param.second->defaultExpr) {
						fprintbf(5, output, " [this]() { return _pd_%s; }", param.first);
					}
					else {
						fprintbf(5, output, " [this]() { return requiredParameterNotFound<%s>(\"%s\", _pn_%s); }",
								paramType, getEnumString(param.second->source), param.first);
					}
				}
				fprintbf(0, output, ");\n");
			}
		}
		if (route.second->operation) {
			addSingleOperation(output, route.second, findOperation(*route.second->operation, units));
		}
		else {
			addMashupOperations(output, route.second, proxies, units);
		}
		fprintbf(3, output, "}\n\n");
		fprintbf(2, output, "private:\n");
		declareProxies(output, proxies);
		for (const auto & param : route.second->params) {
			if (param.second->hasUserSource) {
				if (param.second->source == ParameterSource::URL) {
					Path path(route.second->path);
					long idx = -1;
					for (const auto & pathPart : path.parts) {
						if (auto par = dynamic_cast<PathParameter *>(pathPart.get())) {
							if (par->name == *param.second->key) {
								idx = &pathPart - &path.parts.front();
							}
						}
					}
					fprintbf(3, output, "static constexpr unsigned int _pi_%s{%ld};\n", param.first, idx);
				}
				else {
					fprintbf(3, output, "static constexpr std::string_view _pn_%s{", param.first);
					if (param.second->key) {
						fprintbf(output, "\"%s\"", *param.second->key);
					}
					fputs("};\n", output);
				}
			}
			if (param.second->defaultExpr) {
				auto iceParamDecl = parameters.find(param.first)->second;
				fprintbf(3, output, "static const %s _pd_%s{%s};\n", Slice::typeToString(iceParamDecl->type()),
						param.first, *param.second->defaultExpr);
			}
		}
		fprintbf(1, output, "};\n\n");
	}

	RouteCompiler::Proxies
	RouteCompiler::initializeProxies(FILE * output, const RoutePtr & route)
	{
		Proxies proxies;
		int proxyNum = 0;
		for (const auto & operation : route->operations) {
			auto proxyName = operation.second->operation.substr(0, operation.second->operation.find_last_of('.'));
			if (!proxies.contains(proxyName)) {
				proxies[proxyName] = proxyNum;
				fputs(",\n", output);
				fprintbf(4, output, "prx%d(core->getProxy<%s>())", proxyNum,
						boost::algorithm::replace_all_copy(proxyName, ".", "::"));
				proxyNum += 1;
			}
		}
		return proxies;
	}

	void
	RouteCompiler::declareProxies(FILE * output, const Proxies & proxies)
	{
		for (const auto & proxy : proxies) {
			fprintbf(3, output, "const %sPrxPtr prx%d;\n", boost::algorithm::replace_all_copy(proxy.first, ".", "::"),
					proxy.second);
		}
	}

	void
	RouteCompiler::addSingleOperation(FILE * output, const RoutePtr & route, const Slice::OperationPtr & operation)
	{
		if (auto operationName = route->operation->substr(route->operation->find_last_of('.') + 1);
				operation->returnType()) {
			fprintbf(4, output, "auto _responseModel = prx0->%s(", operationName);
		}
		else {
			fprintbf(4, output, "prx0->%s(", operationName);
		}
		for (const auto & parameter : operation->parameters()) {
			auto routeParam = *route->params.find(parameter->name());
			if (routeParam.second->hasUserSource) {
				fprintbf(output, "_p_%s, ", parameter->name());
			}
			else {
				fprintbf(output, "_pd_%s, ", parameter->name());
			}
		}
		fprintbf(output, "request->getContext());\n");
		for (const auto & mutator : route->mutators) {
			fprintbf(4, output, "%s(request, _responseModel);\n", mutator);
		}
		if (operation->returnType()) {
			fprintbf(4, output, "request->response(this, _responseModel);\n");
		}
		else {
			fprintbf(4, output, "request->response(200, \"OK\");\n");
		}
	}

	void
	RouteCompiler::addMashupOperations(
			FILE * output, const RoutePtr & route, const Proxies & proxies, const Units & units)
	{
		for (const auto & operation : route->operations) {
			auto proxyName = operation.second->operation.substr(0, operation.second->operation.find_last_of('.'));
			auto operationName = operation.second->operation.substr(operation.second->operation.find_last_of('.') + 1);
			fprintbf(4, output, "auto _ar_%s = prx%s->%sAsync(", operation.first, proxies.find(proxyName)->second,
					operationName);
			for (const auto & parameter : findOperation(operation.second->operation, units)->parameters()) {
				auto parameterOverride = operation.second->paramOverrides.find(parameter->name());
				if (route->params
								.find(parameterOverride != operation.second->paramOverrides.end()
												? parameterOverride->second
												: parameter->name())
								->second->hasUserSource) {
					fprintbf(output, "_p_%s, ", parameter->name());
				}
				else {
					fprintbf(output, "_pd_%s, ", parameter->name());
				}
			}
			fprintbf(output, "request->getContext());\n");
		}
		const auto type = findType(route->type, units);
		fprintbf(4, output, "%s _responseModel", type.type);
		if (type.scoped) {
			fprintbf(4, output, " = std::make_shared<%s>()", *type.scoped);
		}
		fprintbf(4, output, ";\n");
		for (const auto & member : type.members) {
			bool isOp = false;
			fprintbf(4, output, "_responseModel%s%s = ", type.scoped ? "->" : ".", member->name());
			for (const auto & operation : route->operations) {
				if (member->name() == operation.first) {
					fprintbf(output, "_ar_%s.get()", operation.first);
					isOp = true;
					break;
				}
			}
			if (!isOp) {
				fprintbf(output, "_p_%s", member->name());
			}
			fputs(";\n", output);
		}
		for (const auto & mutator : route->mutators) {
			fprintbf(4, output, "%s(request, _responseModel);\n", mutator);
		}
		fprintbf(4, output, "request->response(this, _responseModel);\n");
	}
}
