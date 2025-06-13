#pragma once

#include <Ice/BuiltinSequences.h>
#include <Slice/Parser.h>
#include <cstdio>
#include <filesystem>
#include <map>
#include <optional>
#include <routes.h>
#include <string>
#include <vector>
#include <visibility.h>

namespace IceSpider::Compile {
	class DLL_PUBLIC RouteCompiler {
	public:
		using Units = std::map<std::string, Slice::UnitPtr>;

		RouteCompiler();

		[[nodiscard]] static RouteConfigurationPtr loadConfiguration(const std::filesystem::path &);
		[[nodiscard]] Units loadUnits(const RouteConfigurationPtr &) const;

		static void applyDefaults(const RouteConfigurationPtr &, const Units & units);
		void compile(const std::filesystem::path & input, const std::filesystem::path & output) const;

		std::vector<std::filesystem::path> searchPath;

	private:
		using Proxies = std::map<std::string, int>;

#pragma GCC visibility push(hidden)
		static void processConfiguration(
				FILE * output, FILE * outputh, const std::string & name, const RouteConfigurationPtr &, const Units &);
		static void processBases(FILE * output, FILE * outputh, const RouteConfigurationPtr &, const Units &);
		static void processBase(FILE * output, FILE * outputh, const RouteBases::value_type &, const Units &);
		static void processRoutes(FILE * output, const RouteConfigurationPtr &, const Units &);
		static void processRoute(FILE * output, const Routes::value_type &, const Units &);
		static void registerOutputSerializers(FILE * output, const RoutePtr &);
		[[nodiscard]] static Proxies initializeProxies(FILE * output, const RoutePtr &);
		static void declareProxies(FILE * output, const Proxies &);
		static void addSingleOperation(FILE * output, const RoutePtr &, const Slice::OperationPtr &);
		static void addMashupOperations(FILE * output, const RoutePtr &, const Proxies &, const Units &);
		using ParameterMap = std::map<std::string, Slice::ParamDeclPtr>;
		static ParameterMap findParameters(const RoutePtr &, const Units &);
		static Slice::OperationPtr findOperation(const std::string &, const Units &);
		static Slice::OperationPtr findOperation(
				const std::string &, const Slice::ContainerPtr &, const Ice::StringSeq & = Ice::StringSeq());

		// using Type = std::pair<Slice::StructPtr, Slice::ClassDeclPtr>;
		struct Type {
			std::string type;
			std::optional<std::string> scoped;
			Slice::DataMemberList members;
		};

		static Type findType(const std::string &, const Units &);
		static std::optional<Type> findType(
				const std::string &, const Slice::ContainerPtr &, const Ice::StringSeq & = Ice::StringSeq());
#pragma GCC visibility pop
	};
}
