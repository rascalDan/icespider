#ifndef ICESPIDER_IROUTEHANDLER_H
#define ICESPIDER_IROUTEHANDLER_H

#include "ihttpRequest.h"
#include "util.h"
#include <pathparts.h>
#include <routes.h>
#include <plugins.h>
#include <visibility.h>
#include <boost/lexical_cast.hpp>

namespace IceSpider {
	class DLL_PUBLIC IRouteHandler : public AdHoc::AbstractPluginImplementation, public Path {
		public:
			IRouteHandler(HttpMethod, const std::string & path);
			virtual ~IRouteHandler();

			virtual void execute(IHttpRequest * request) const = 0;
			virtual ContentTypeSerializer getSerializer(const AcceptPtr &, std::ostream &) const;
			virtual ContentTypeSerializer defaultSerializer(std::ostream &) const;

			const HttpMethod method;

		protected:
			typedef Slicer::StreamSerializerFactory * StreamSerializerFactoryPtr;
			typedef std::map<MimeType, StreamSerializerFactoryPtr> RouteSerializers;
			RouteSerializers routeSerializers;

			template <typename T, typename K>
			inline T requiredParameterNotFound(const char *, const K & key) const
			{
				throw std::runtime_error("Required parameter not found: " +
						boost::lexical_cast<std::string>(key));
			}

			Ice::ObjectPrx getProxy(IHttpRequest *, const char *) const;

			template<typename Interface>
			typename Interface::ProxyType getProxy(IHttpRequest * request) const
			{
				return Interface::ProxyType::uncheckedCast(getProxy(request, typeid(Interface).name()));
			}

			void addRouteSerializer(const MimeType &, StreamSerializerFactoryPtr);
			void removeRouteSerializer(const MimeType &);
	};
	typedef AdHoc::PluginOf<IRouteHandler> RouteHandlers;
}

#endif


