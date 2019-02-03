#ifndef ICESPIDER_CORE_CORE_H
#define ICESPIDER_CORE_CORE_H

#include <visibility.h>
#include <vector>
#include "irouteHandler.h"
#include <Ice/Communicator.h>
#include <filesystem>
#include <plugins.h>

namespace IceSpider {
	class DLL_PUBLIC Core {
		public:
			typedef std::vector<IRouteHandlerCPtr> AllRoutes;

			Core(const Ice::StringSeq & = {});
			virtual ~Core();

			virtual const IRouteHandler * findRoute(const IHttpRequest *) const = 0;
			void process(IHttpRequest *, const IRouteHandler * = nullptr) const;
			void handleError(IHttpRequest *, const std::exception &) const;

			Ice::ObjectPrxPtr getProxy(const char * type) const;

			template<typename Interface>
			auto getProxy() const
			{
				return Ice::uncheckedCast<typename Interface::ProxyType>(getProxy(typeid(Interface).name()));
			}

			AllRoutes allRoutes;
			Ice::CommunicatorPtr communicator;
			Ice::ObjectAdapterPtr pluginAdapter;

			static const std::filesystem::path defaultConfig;

		private:
			void defaultErrorReport(IHttpRequest * request, const std::exception & ex) const;
	};

	class DLL_PUBLIC CoreWithDefaultRouter : public Core {
		public:
			typedef std::vector<IRouteHandlerCPtr> LengthRoutes;
			typedef std::vector<LengthRoutes> Routes;

			CoreWithDefaultRouter(const Ice::StringSeq & = {});

			const IRouteHandler * findRoute(const IHttpRequest *) const override;

			Routes routes;
	};

	class DLL_PUBLIC Plugin : public virtual Ice::Object {
	};
	typedef AdHoc::Factory<Plugin, Ice::CommunicatorPtr, Ice::PropertiesPtr> PluginFactory;

	enum ErrorHandlerResult {
		ErrorHandlerResult_Unhandled,
		ErrorHandlerResult_Handled,
		ErrorHandlerResult_Modified,
	};
	class DLL_PUBLIC ErrorHandler : public AdHoc::AbstractPluginImplementation {
		public:
			virtual ErrorHandlerResult handleError(IHttpRequest * IHttpRequest, const std::exception &) const = 0;
	};
	typedef AdHoc::PluginOf<ErrorHandler> ErrorHandlerPlugin;
}

#endif

