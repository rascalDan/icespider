#ifndef ICESPIDER_IROUTEHANDLER_H
#define ICESPIDER_IROUTEHANDLER_H

#include "ihttpRequest.h"
#include "util.h"
#include "exceptions.h"
#include <pathparts.h>
#include <factory.h>
#include <visibility.h>
#include "routeOptions.h"

namespace IceSpider {
	class Core;

	class DLL_PUBLIC IRouteHandler : public Path {
		public:
			const static RouteOptions defaultRouteOptions;

			IRouteHandler(HttpMethod, const std::string_view & path);
			IRouteHandler(HttpMethod, const std::string_view & path, const RouteOptions &);
			virtual ~IRouteHandler() = default;

			virtual void execute(IHttpRequest * request) const = 0;
			virtual ContentTypeSerializer getSerializer(const AcceptPtr &, std::ostream &) const;
			virtual ContentTypeSerializer defaultSerializer(std::ostream &) const;

			const HttpMethod method;

		protected:
			typedef std::shared_ptr<Slicer::StreamSerializerFactory> StreamSerializerFactoryPtr;
			typedef std::map<MimeType, StreamSerializerFactoryPtr> RouteSerializers;
			RouteSerializers routeSerializers;

			void requiredParameterNotFound(const char *, const std::string_view & key) const;

			template <typename T, typename K>
			inline T requiredParameterNotFound(const char * s, const K & key) const
			{
				requiredParameterNotFound(s, key);
				// LCOV_EXCL_START unreachable, requiredParameterNotFound always throws
				return T();
				// LCOV_EXCL_STOP
			}

			void addRouteSerializer(const MimeType &, const StreamSerializerFactoryPtr &);
	};
	typedef std::shared_ptr<IRouteHandler> IRouteHandlerPtr;
	typedef std::shared_ptr<const IRouteHandler> IRouteHandlerCPtr;
	typedef AdHoc::Factory<IRouteHandler, const Core *> RouteHandlerFactory;
}

#if __cplusplus < 201709
namespace std {
	template <typename T> using remove_cvref = typename std::remove_cv<typename std::remove_reference<T>::type>;
}
#endif

#endif


