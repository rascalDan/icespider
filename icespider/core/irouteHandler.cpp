#include "irouteHandler.h"
#include "core.h"
#include <factory.impl.h>

INSTANTIATEVOIDFACTORY(IceSpider::IRouteHandler);

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

	Ice::ObjectPrx
	IRouteHandler::getProxy(IHttpRequest * request, const char * type) const
	{
		return request->core->getProxy(type);
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
	IRouteHandler::addRouteSerializer(const MimeType & ct, StreamSerializerFactoryPtr ssfp)
	{
		routeSerializers.insert({ ct, ssfp });
	}

	void
	IRouteHandler::removeRouteSerializer(const MimeType & ct)
	{
		auto i = routeSerializers.find(ct);
		if (i != routeSerializers.end()) {
			delete i->second;
			routeSerializers.erase(i);
		}
	}
}

