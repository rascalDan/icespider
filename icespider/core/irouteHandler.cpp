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

	Ice::ObjectPrx
	IRouteHandler::getProxy(IHttpRequest * request, const char * type) const
	{
		return request->core->getProxy(type);
	}
}

