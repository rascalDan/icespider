#include "irouteHandler.h"
#include "core.h"
#include <factory.impl.h>
#include <formatters.h>

INSTANTIATEFACTORY(IceSpider::IRouteHandler, const IceSpider::Core *);

namespace IceSpider {
	static const std::string APPLICATION = "application";
	static const std::string JSON = "json";
	static const std::string APPLICATION_JSON = MimeTypeFmt::get(APPLICATION, JSON);

	const RouteOptions IRouteHandler::defaultRouteOptions {
	};
	IRouteHandler::IRouteHandler(HttpMethod m, const std::string_view & p) :
		IRouteHandler(m, p, defaultRouteOptions)
	{
	}

	IRouteHandler::IRouteHandler(HttpMethod m, const std::string_view & p, const RouteOptions & ro) :
		Path(p),
		method(m)
	{
		if (ro.addDefaultSerializers) {
			auto globalSerializers = AdHoc::PluginManager::getDefault()->getAll<Slicer::StreamSerializerFactory>();
			for (const auto & gs : globalSerializers) {
				auto slash = gs->name.find('/');
				routeSerializers.insert({ { gs->name.substr(0, slash), gs->name.substr(slash + 1) }, gs->implementation() });
			}
		}
	}

	IRouteHandler::~IRouteHandler()
	{
	}

	ContentTypeSerializer
	IRouteHandler::getSerializer(const AcceptPtr & a, std::ostream & strm) const
	{
		for (const auto & rs : routeSerializers) {
			if ((!a->group || rs.first.group == a->group) && (!a->type || rs.first.type == a->type)) {
				return { rs.first, rs.second->create(strm) };
			}
		}
		return ContentTypeSerializer();
	}

	ContentTypeSerializer
	IRouteHandler::defaultSerializer(std::ostream & strm) const
	{
		return {
			{ APPLICATION, JSON },
			Slicer::StreamSerializerFactory::createNew(APPLICATION_JSON, strm)
		};
	}

	void
	IRouteHandler::requiredParameterNotFound(const char *, const std::string_view &) const
	{
		throw Http400_BadRequest();
	}

	void
	IRouteHandler::addRouteSerializer(const MimeType & ct, StreamSerializerFactoryPtr ssfp)
	{
		routeSerializers.erase(ct);
		routeSerializers.insert({ ct, ssfp });
	}
}

