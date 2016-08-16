#include "irouteHandler.h"
#include "core.h"
#include <plugins.impl.h>

INSTANTIATEPLUGINOF(IceSpider::IRouteHandler);

namespace IceSpider {
	IRouteHandler::IRouteHandler(HttpMethod m, const std::string & p) :
		Path(p),
		method(m)
	{
	}

	void
	IRouteHandler::initialize()
	{
		auto globalSerializers = AdHoc::PluginManager::getDefault()->getAll<Slicer::StreamSerializerFactory>();
		for (const auto & gs : globalSerializers) {
			auto slash = gs->name.find('/');
			routeSerializers.insert({ { gs->name.substr(0, slash), gs->name.substr(slash + 1) }, gs });
		}
	}

	Ice::ObjectPrx
	IRouteHandler::getProxy(IHttpRequest * request, const char * type) const
	{
		return request->core->getProxy(type);
	}

	Slicer::SerializerPtr
	IRouteHandler::getSerializer(const char * grp, const char * type, std::ostream & strm) const
	{
		for (const auto & rs : routeSerializers) {
			if ((!grp || rs.first.first == grp) && (!type || rs.first.second == type)) {
				return rs.second->implementation()->create(strm);
			}
		}
		return nullptr;
	}

	Slicer::SerializerPtr
	IRouteHandler::defaultSerializer(std::ostream & strm) const
	{
		return Slicer::StreamSerializerFactory::createNew(
			"application/json", strm);
	}
}

