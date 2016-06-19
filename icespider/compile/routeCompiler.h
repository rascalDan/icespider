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

				UserIceSpider::RouteConfigurationPtr loadConfiguration(const boost::filesystem::path &) const;
				Units loadUnits(UserIceSpider::RouteConfigurationPtr) const;

				void applyDefaults(UserIceSpider::RouteConfigurationPtr, const Units & u) const;
				void compile(const boost::filesystem::path & input, const boost::filesystem::path & output) const;

				std::vector<boost::filesystem::path> searchPath;

			private:
				void processConfiguration(FILE * output, UserIceSpider::RouteConfigurationPtr, const Units &) const;
				static Slice::OperationPtr findOperation(UserIceSpider::RoutePtr, const Units &);
				static Slice::OperationPtr findOperation(UserIceSpider::RoutePtr, const Slice::ContainerPtr &, const Ice::StringSeq & = Ice::StringSeq());
		};
	}
}

#endif

