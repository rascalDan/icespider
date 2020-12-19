#ifndef ICESPIDER_CORE_CORE_H
#define ICESPIDER_CORE_CORE_H

#include "irouteHandler.h"
#include "util.h"
#include <Ice/Communicator.h>
#include <c++11Helpers.h>
#include <filesystem>
#include <plugins.h>
#include <vector>
#include <visibility.h>

namespace IceSpider {
	class DLL_PUBLIC Core {
	public:
		using AllRoutes = std::vector<IRouteHandlerCPtr>;

		explicit Core(const Ice::StringSeq & = {});
		SPECIAL_MEMBERS_MOVE_RO(Core);
		virtual ~Core();

		virtual const IRouteHandler * findRoute(const IHttpRequest *) const = 0;
		void process(IHttpRequest *, const IRouteHandler * = nullptr) const;
		void handleError(IHttpRequest *, const std::exception &) const;

		[[nodiscard]] Ice::ObjectPrxPtr getProxy(std::string_view type) const;

		template<typename Interface>
		[[nodiscard]] auto
		getProxy() const
		{
			return Ice::uncheckedCast<typename Interface::ProxyType>(getProxy(TypeName<Interface>::str()));
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
		using LengthRoutes = std::vector<IRouteHandlerCPtr>;
		using Routes = std::vector<LengthRoutes>;

		explicit CoreWithDefaultRouter(const Ice::StringSeq & = {});

		const IRouteHandler * findRoute(const IHttpRequest *) const override;

		Routes routes;
	};

	class DLL_PUBLIC Plugin : public virtual Ice::Object {
	};
	using PluginFactory = AdHoc::Factory<Plugin, Ice::CommunicatorPtr, Ice::PropertiesPtr>;

	enum ErrorHandlerResult {
		ErrorHandlerResult_Unhandled,
		ErrorHandlerResult_Handled,
		ErrorHandlerResult_Modified,
	};
	class DLL_PUBLIC ErrorHandler : public AdHoc::AbstractPluginImplementation {
	public:
		virtual ErrorHandlerResult handleError(IHttpRequest * IHttpRequest, const std::exception &) const = 0;
	};
	using ErrorHandlerPlugin = AdHoc::PluginOf<ErrorHandler>;
}

#endif
