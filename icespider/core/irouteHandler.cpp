#include "irouteHandler.h"
#include <plugins.impl.h>

INSTANTIATEPLUGINOF(IceSpider::IRouteHandler);

namespace IceSpider {
	IRouteHandler::IRouteHandler(UserIceSpider::HttpMethod m, const std::string & p) :
		method(m),
		path(p)
	{
	}

	Ice::ObjectPrx
	IRouteHandler::getProxy(const char *) const
	{
		return NULL;
	}
}

