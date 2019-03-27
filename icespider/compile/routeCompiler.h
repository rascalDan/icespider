#ifndef ICESPIDER_COMPILE_ROURTECOMPILER_H
#define ICESPIDER_COMPILE_ROURTECOMPILER_H

#include <filesystem>
#include <visibility.h>
#include <vector>
#include <routes.h>
#include <Slice/Parser.h>
#include <Ice/BuiltinSequences.h>

namespace IceSpider {
	namespace Compile {
		class DLL_PUBLIC RouteCompiler {
			public:
				typedef std::map<std::string, Slice::UnitPtr> Units;

				RouteCompiler();

				RouteConfigurationPtr loadConfiguration(const std::filesystem::path &) const;
				Units loadUnits(const RouteConfigurationPtr &) const;

				void applyDefaults(const RouteConfigurationPtr &, const Units & u) const;
				void compile(const std::filesystem::path & input, const std::filesystem::path & output) const;

				std::vector<std::filesystem::path> searchPath;

			private:
				typedef std::map<std::string, int> Proxies;

#pragma GCC visibility push(hidden)
				void processConfiguration(FILE * output, FILE * outputh, const std::string & name, const RouteConfigurationPtr &, const Units &) const;
				void processBases(FILE * output, FILE * outputh, const RouteConfigurationPtr &, const Units &) const;
				void processBase(FILE * output, FILE * outputh, const RouteBases::value_type &, const Units &) const;
				void processRoutes(FILE * output, const RouteConfigurationPtr &, const Units &) const;
				void processRoute(FILE * output, const Routes::value_type &, const Units &) const;
				void registerOutputSerializers(FILE * output, const RoutePtr &) const;
				Proxies initializeProxies(FILE * output, const RoutePtr &) const;
				void declareProxies(FILE * output, const Proxies &) const;
				void addSingleOperation(FILE * output, const RoutePtr &, const Slice::OperationPtr &) const;
				void addMashupOperations(FILE * output, const RoutePtr &, const Proxies &, const Units &) const;
				using ParameterMap = std::map<std::string, Slice::ParamDeclPtr>;
				static ParameterMap findParameters(const RoutePtr &, const Units &);
				static Slice::OperationPtr findOperation(const std::string &, const Units &);
				static Slice::OperationPtr findOperation(const std::string &, const Slice::ContainerPtr &, const Ice::StringSeq & = Ice::StringSeq());
				using Type = std::pair<Slice::StructPtr, Slice::ClassDeclPtr>;
				static Type findType(const std::string &, const Units &);
				static Type findType(const std::string &, const Slice::ContainerPtr &, const Ice::StringSeq & = Ice::StringSeq());
#pragma GCC visibility pop
		};
	}
}

#endif

