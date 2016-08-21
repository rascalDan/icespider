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
				void processConfiguration(FILE * output, RouteConfigurationPtr, const Units &) const;
				void registerOutputSerializers(FILE * output, RoutePtr) const;
				void releaseOutputSerializers(FILE * output, RoutePtr) const;
				static Slice::OperationPtr findOperation(RoutePtr, const Units &);
				static Slice::OperationPtr findOperation(RoutePtr, const Slice::ContainerPtr &, const Ice::StringSeq & = Ice::StringSeq());
		};
	}
}

#endif

