#ifndef ICESPIDER_IROUTEHANDLER_H
#define ICESPIDER_IROUTEHANDLER_H

#include "ihttpRequest.h"
#include <routes.h>
#include <plugins.h>
#include <visibility.h>

namespace IceSpider {
	class DLL_PUBLIC IRouteHandler : public AdHoc::AbstractPluginImplementation {
		public:
			IRouteHandler(UserIceSpider::HttpMethod, const std::string & path);
			virtual void execute(IHttpRequest * request) const = 0;

			const UserIceSpider::HttpMethod method;
			const std::string path;

		protected:
			Ice::ObjectPrx getProxy(const char *) const;

			template<typename Interface>
			typename Interface::ProxyType getProxy() const
			{
				return Interface::ProxyType::uncheckedCast(getProxy(typeid(Interface).name()));
			}
	};
	typedef AdHoc::PluginOf<IRouteHandler> RouteHandlers;
}

#endif


