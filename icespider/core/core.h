#ifndef ICESPIDER_CORE_CORE_H
#define ICESPIDER_CORE_CORE_H

#include <visibility.h>
#include <vector>
#include "irouteHandler.h"
#include <Ice/Communicator.h>

namespace IceSpider {
	class DLL_PUBLIC Core {
		public:
			typedef std::vector<const IRouteHandler *> Routes;
			typedef std::vector<Routes> LengthRoutes;
			typedef std::vector<LengthRoutes> MethodRoutes;

			Core();
			~Core();

			void process(IHttpRequest *) const;
			const IRouteHandler * findRoute(const IHttpRequest *) const;

			Ice::ObjectPrx getProxy(const char * type) const;

			MethodRoutes routes;
			Ice::CommunicatorPtr communicator;
	};
}

#endif

