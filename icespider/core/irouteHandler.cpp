#include "irouteHandler.h"
#include "core.h"
#include <factory.impl.h>

INSTANTIATEFACTORY(IceSpider::IRouteHandler, const IceSpider::Core *);

namespace IceSpider {
	IRouteHandler::IRouteHandler(HttpMethod m, const std::string & p) :
		Path(p),
		method(m)
	{
		auto globalSerializers = AdHoc::PluginManager::getDefault()->getAll<Slicer::StreamSerializerFactory>();
		for (const auto & gs : globalSerializers) {
			auto slash = gs->name.find('/');
			routeSerializers.insert({ { gs->name.substr(0, slash), gs->name.substr(slash + 1) }, gs->implementation() });
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
			{ "application", "json" },
			Slicer::StreamSerializerFactory::createNew("application/json", strm)
		};
	}

	void
	IRouteHandler::requiredParameterNotFound(const char *, const std::string &) const
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

