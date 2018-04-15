#ifndef ICESPIDER_COMPILE_ROURTECOMPILER_H
#define ICESPIDER_COMPILE_ROURTECOMPILER_H

#include <boost/filesystem/path.hpp>
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

				RouteConfigurationPtr loadConfiguration(const boost::filesystem::path &) const;
				Units loadUnits(RouteConfigurationPtr) const;

				void applyDefaults(RouteConfigurationPtr, const Units & u) const;
				void compile(const boost::filesystem::path & input, const boost::filesystem::path & output) const;

				std::vector<boost::filesystem::path> searchPath;

			private:
				typedef std::map<std::string, int> Proxies;

#pragma GCC visibility push(hidden)
				void processConfiguration(FILE * output, FILE * outputh, const std::string & name, RouteConfigurationPtr, const Units &) const;
				void processBases(FILE * output, FILE * outputh, RouteConfigurationPtr, const Units &) const;
				void processBase(FILE * output, FILE * outputh, const RouteBases::value_type &, const Units &) const;
				void processRoutes(FILE * output, RouteConfigurationPtr, const Units &) const;
				void processRoute(FILE * output, const Routes::value_type &, const Units &) const;
				void registerOutputSerializers(FILE * output, RoutePtr) const;
				Proxies initializeProxies(FILE * output, RoutePtr) const;
				void declareProxies(FILE * output, const Proxies &) const;
				void addSingleOperation(FILE * output, RoutePtr, Slice::OperationPtr) const;
				void addMashupOperations(FILE * output, RoutePtr, const Proxies &, const Units &) const;
				typedef std::map<std::string, Slice::ParamDeclPtr> ParameterMap;
				static ParameterMap findParameters(RoutePtr, const Units &);
				static Slice::OperationPtr findOperation(const std::string &, const Units &);
				static Slice::OperationPtr findOperation(const std::string &, const Slice::ContainerPtr &, const Ice::StringSeq & = Ice::StringSeq());
				typedef std::pair<Slice::StructPtr, Slice::ClassDeclPtr> Type;
				static Type findType(const std::string &, const Units &);
				static Type findType(const std::string &, const Slice::ContainerPtr &, const Ice::StringSeq & = Ice::StringSeq());
#pragma GCC visibility pop
		};
	}
}

#endif

