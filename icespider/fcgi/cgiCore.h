#ifndef ICESPIDER_CGI_CGICORE_H
#define ICESPIDER_CGI_CGICORE_H

#include <core.h>

namespace IceSpider {
	class CgiCore : public Core {
		public:
			typedef std::vector<const IRouteHandler *> LengthRoutes;
			typedef std::vector<LengthRoutes> Routes;

			CgiCore(const Ice::StringSeq & = {});

			const IRouteHandler * findRoute(const IHttpRequest *) const override;

			Routes routes;
	};
}

#endif

