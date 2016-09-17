#ifndef ICESPIDER_IROUTEHANDLER_H
#define ICESPIDER_IROUTEHANDLER_H

#include "ihttpRequest.h"
#include "util.h"
#include "exceptions.h"
#include <pathparts.h>
#include <routes.h>
#include <factory.h>
#include <visibility.h>
#include <boost/lexical_cast.hpp>

namespace IceSpider {
	class Core;

	class DLL_PUBLIC IRouteHandler : public Path {
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

			void requiredParameterNotFound(const char *, const std::string & key) const;

			template <typename T, typename K>
			inline T requiredParameterNotFound(const char * s, const K & key) const
			{
				requiredParameterNotFound(s, key);
				// LCOV_EXCL_START unreachable, requiredParameterNotFound always throws
				return T();
				// LCOV_EXCL_STOP
			}

			void addRouteSerializer(const MimeType &, StreamSerializerFactoryPtr);
			void removeRouteSerializer(const MimeType &);
	};
	typedef AdHoc::Factory<IRouteHandler, const Core *> RouteHandlerFactory;
}

#endif


