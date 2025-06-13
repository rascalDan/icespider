#include "irouteHandler.h"
#include "exceptions.h"
#include "routeOptions.h"
#include <factory.impl.h>
#include <formatters.h>
#include <optional>
#include <pathparts.h>
#include <string>
#include <utility>

INSTANTIATEFACTORY(IceSpider::IRouteHandler, const IceSpider::Core *);

namespace IceSpider {
	static const std::string APPLICATION = "application";
	static const std::string JSON = "json";
	static const std::string APPLICATION_JSON = MimeTypeFmt::get(APPLICATION, JSON);

	const RouteOptions IRouteHandler::DEFAULT_ROUTE_OPTIONS {};

	IRouteHandler::IRouteHandler(HttpMethod method, const std::string_view path) :
		IRouteHandler(method, path, DEFAULT_ROUTE_OPTIONS)
	{
	}

	IRouteHandler::IRouteHandler(HttpMethod method, const std::string_view path, const RouteOptions & routeOpts) :
		Path(path), method(method)
	{
		if (routeOpts.addDefaultSerializers) {
			auto globalSerializers = AdHoc::PluginManager::getDefault()->getAll<Slicer::StreamSerializerFactory>();
			for (const auto & serializer : globalSerializers) {
				auto slash = serializer->name.find('/');
				routeSerializers.insert(
						{{.group = serializer->name.substr(0, slash), .type = serializer->name.substr(slash + 1)},
								serializer->implementation()});
			}
		}
	}

	ContentTypeSerializer
	IRouteHandler::getSerializer(const Accept & accept, std::ostream & strm) const
	{
		for (const auto & serializer : routeSerializers) {
			if ((!accept.group || serializer.first.group == accept.group)
					&& (!accept.type || serializer.first.type == accept.type)) {
				return {serializer.first, serializer.second->create(strm)};
			}
		}
		return {};
	}

	ContentTypeSerializer
	IRouteHandler::defaultSerializer(std::ostream & strm) const
	{
		return {{.group = APPLICATION, .type = JSON},
				Slicer::StreamSerializerFactory::createNew(APPLICATION_JSON, strm)};
	}

	void
	IRouteHandler::requiredParameterNotFound(const char *, const std::string_view)
	{
		throw Http400BadRequest();
	}

	void
	IRouteHandler::addRouteSerializer(const MimeType & contentType, const StreamSerializerFactoryPtr & ssfp)
	{
		routeSerializers.erase(contentType);
		routeSerializers.emplace(contentType, ssfp);
	}
}
