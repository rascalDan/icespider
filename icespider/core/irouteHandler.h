#pragma once

#include "http.h"
#include "ihttpRequest.h"
#include "slicer/serializer.h"
#include <c++11Helpers.h>
#include <factory.h> // IWYU pragma: keep
#include <iosfwd>
#include <map>
#include <memory>
#include <pathparts.h>
#include <string_view>
#include <visibility.h>

// IWYU pragma: no_include "factory.impl.h"
// IWYU pragma: no_include <Ice/Comparable.h>
// IWYU pragma: no_include <string>

namespace IceSpider {
	class Core;
	class RouteOptions;

	class DLL_PUBLIC IRouteHandler : public Path {
	public:
		const static RouteOptions defaultRouteOptions;

		IRouteHandler(HttpMethod, const std::string_view & path);
		IRouteHandler(HttpMethod, const std::string_view & path, const RouteOptions &);
		SPECIAL_MEMBERS_MOVE_RO(IRouteHandler);
		virtual ~IRouteHandler() = default;

		virtual void execute(IHttpRequest * request) const = 0;
		virtual ContentTypeSerializer getSerializer(const Accept &, std::ostream &) const;
		virtual ContentTypeSerializer defaultSerializer(std::ostream &) const;

		const HttpMethod method;

	protected:
		using StreamSerializerFactoryPtr = std::shared_ptr<Slicer::StreamSerializerFactory>;
		using RouteSerializers = std::map<MimeType, StreamSerializerFactoryPtr>;
		RouteSerializers routeSerializers;

		[[noreturn]] void requiredParameterNotFound(const char *, const std::string_view & key) const;

		template<typename T, typename K>
		inline T
		requiredParameterNotFound(const char * s, const K & key) const
		{
			requiredParameterNotFound(s, key);
		}

		void addRouteSerializer(const MimeType &, const StreamSerializerFactoryPtr &);
	};

	using IRouteHandlerPtr = std::shared_ptr<IRouteHandler>;
	using IRouteHandlerCPtr = std::shared_ptr<const IRouteHandler>;
	using RouteHandlerFactory = AdHoc::Factory<IRouteHandler, const Core *>;
}
