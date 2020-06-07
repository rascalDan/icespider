#ifndef ICESPIDER_COMPILE_ROURTECOMPILER_H
#define ICESPIDER_COMPILE_ROURTECOMPILER_H

#include <Ice/BuiltinSequences.h>
#include <Slice/Parser.h>
#include <filesystem>
#include <routes.h>
#include <vector>
#include <visibility.h>

namespace IceSpider {
	namespace Compile {
		class DLL_PUBLIC RouteCompiler {
		public:
			using Units = std::map<std::string, Slice::UnitPtr>;

			RouteCompiler();

			[[nodiscard]] RouteConfigurationPtr loadConfiguration(const std::filesystem::path &) const;
			[[nodiscard]] Units loadUnits(const RouteConfigurationPtr &) const;

			void applyDefaults(const RouteConfigurationPtr &, const Units & u) const;
			void compile(const std::filesystem::path & input, const std::filesystem::path & output) const;

			std::vector<std::filesystem::path> searchPath;

		private:
			using Proxies = std::map<std::string, int>;

#pragma GCC visibility push(hidden)
			void processConfiguration(FILE * output, FILE * outputh, const std::string & name,
					const RouteConfigurationPtr &, const Units &) const;
			void processBases(FILE * output, FILE * outputh, const RouteConfigurationPtr &, const Units &) const;
			void processBase(FILE * output, FILE * outputh, const RouteBases::value_type &, const Units &) const;
			void processRoutes(FILE * output, const RouteConfigurationPtr &, const Units &) const;
			void processRoute(FILE * output, const Routes::value_type &, const Units &) const;
			void registerOutputSerializers(FILE * output, const RoutePtr &) const;
			[[nodiscard]] Proxies initializeProxies(FILE * output, const RoutePtr &) const;
			void declareProxies(FILE * output, const Proxies &) const;
			void addSingleOperation(FILE * output, const RoutePtr &, const Slice::OperationPtr &) const;
			void addMashupOperations(FILE * output, const RoutePtr &, const Proxies &, const Units &) const;
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
}

#endif
