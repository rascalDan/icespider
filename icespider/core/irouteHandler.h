#ifndef ICESPIDER_IROUTEHANDLER_H
#define ICESPIDER_IROUTEHANDLER_H

#include "ihttpRequest.h"
#include "util.h"
#include <pathparts.h>
#include <routes.h>
#include <plugins.h>
#include <visibility.h>

namespace IceSpider {
	class DLL_PUBLIC IRouteHandler : public AdHoc::AbstractPluginImplementation, public Path {
		public:
			IRouteHandler(UserIceSpider::HttpMethod, const std::string & path);
			virtual void execute(IHttpRequest * request) const = 0;

			const UserIceSpider::HttpMethod method;

		protected:
			template <typename T>
			inline T requiredParameterNotFound(const char *, const std::string & key) const
			{
				throw std::runtime_error("Required parameter not found: " + key);
			}

			Ice::ObjectPrx getProxy(IHttpRequest *, const char *) const;

			template<typename Interface>
			typename Interface::ProxyType getProxy(IHttpRequest * request) const
			{
				return Interface::ProxyType::uncheckedCast(getProxy(request, typeid(Interface).name()));
			}
	};
	typedef AdHoc::PluginOf<IRouteHandler> RouteHandlers;
}

#endif


