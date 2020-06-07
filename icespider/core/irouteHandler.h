#ifndef ICESPIDER_IROUTEHANDLER_H
#define ICESPIDER_IROUTEHANDLER_H

#include "exceptions.h"
#include "ihttpRequest.h"
#include "routeOptions.h"
#include "util.h"
#include <c++11Helpers.h>
#include <factory.h>
#include <pathparts.h>
#include <visibility.h>

namespace IceSpider {
	class Core;

	class DLL_PUBLIC IRouteHandler : public Path {
	public:
		const static RouteOptions defaultRouteOptions;

		IRouteHandler(HttpMethod, const std::string_view & path);
		IRouteHandler(HttpMethod, const std::string_view & path, const RouteOptions &);
		SPECIAL_MEMBERS_MOVE_RO(IRouteHandler);
		virtual ~IRouteHandler() = default;

		virtual void execute(IHttpRequest * request) const = 0;
		virtual ContentTypeSerializer getSerializer(const AcceptPtr &, std::ostream &) const;
		virtual ContentTypeSerializer defaultSerializer(std::ostream &) const;

		const HttpMethod method;

	protected:
		using StreamSerializerFactoryPtr = std::shared_ptr<Slicer::StreamSerializerFactory>;
		using RouteSerializers = std::map<MimeType, StreamSerializerFactoryPtr>;
		RouteSerializers routeSerializers;

		void requiredParameterNotFound(const char *, const std::string_view & key) const;

		template<typename T, typename K>
		inline T
		requiredParameterNotFound(const char * s, const K & key) const
		{
			requiredParameterNotFound(s, key);
			// LCOV_EXCL_START unreachable, requiredParameterNotFound always throws
			return T();
			// LCOV_EXCL_STOP
		}

		void addRouteSerializer(const MimeType &, const StreamSerializerFactoryPtr &);
	};
	using IRouteHandlerPtr = std::shared_ptr<IRouteHandler>;
	using IRouteHandlerCPtr = std::shared_ptr<const IRouteHandler>;
	using RouteHandlerFactory = AdHoc::Factory<IRouteHandler, const Core *>;
}

#if __cplusplus < 201709
namespace std {
	template<typename T> using remove_cvref = typename std::remove_cv<typename std::remove_reference<T>::type>;
}
#endif

#endif
