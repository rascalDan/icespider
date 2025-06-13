#pragma once

#include "irouteHandler.h"
#include "util.h"
#include <Ice/BuiltinSequences.h>
#include <Ice/Communicator.h>
#include <Ice/Object.h>
#include <Ice/ObjectAdapterF.h>
#include <Ice/Properties.h>
#include <Ice/Proxy.h>
#include <Ice/ProxyF.h>
#include <c++11Helpers.h>
#include <exception>
#include <factory.h> // IWYU pragma: keep
#include <filesystem>
#include <plugins.h> // IWYU pragma: keep
#include <string_view>
#include <vector>
#include <visibility.h>

// IWYU pragma: no_include "factory.impl.h"

namespace IceSpider {
	class IHttpRequest;

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

		static const std::filesystem::path DEFAULT_CONFIG;

	private:
		static void defaultErrorReport(IHttpRequest * request, const std::exception & exception);
	};

	class DLL_PUBLIC CoreWithDefaultRouter : public Core {
	public:
		using LengthRoutes = std::vector<IRouteHandlerCPtr>;
		using Routes = std::vector<LengthRoutes>;

		explicit CoreWithDefaultRouter(const Ice::StringSeq & = {});

		const IRouteHandler * findRoute(const IHttpRequest *) const override;

		Routes routes;
	};

	class DLL_PUBLIC Plugin : public virtual Ice::Object { };

	using PluginFactory = AdHoc::Factory<Plugin, Ice::CommunicatorPtr, Ice::PropertiesPtr>;

	enum class ErrorHandlerResult : uint8_t {
		Unhandled,
		Handled,
		Modified,
	};

	class DLL_PUBLIC ErrorHandler : public AdHoc::AbstractPluginImplementation {
	public:
		virtual ErrorHandlerResult handleError(IHttpRequest * iHttpRequest, const std::exception &) const = 0;
	};

	using ErrorHandlerPlugin = AdHoc::PluginOf<ErrorHandler>;
}
