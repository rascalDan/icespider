#include "cgiCore.h"

namespace IceSpider {
	static
	bool
	operator/=(const PathElements & pathparts, const IRouteHandler * r)
	{
		auto rpi = r->parts.begin();
		for (auto ppi = pathparts.begin(); ppi != pathparts.end(); ++ppi, ++rpi) {
			if (!(*rpi)->matches(*ppi)) return false;
		}
		return true;
	}

	CgiCore::CgiCore(const Ice::StringSeq & opts) :
		Core(opts)
	{
		for (const auto & r : allRoutes) {
			if (routes.size() <= r->pathElementCount()) {
				routes.resize(r->pathElementCount() + 1);
			}
			auto & lroutes = routes[r->pathElementCount()];
			lroutes.push_back(r);
		}
	}

	const IRouteHandler *
	CgiCore::findRoute(const IHttpRequest * request) const
	{
		const auto & pathparts = request->getRequestPath();
		const auto method = request->getRequestMethod();
		if (pathparts.size() >= routes.size()) {
			throw Http404_NotFound();
		}
		const auto & routeSet = routes[pathparts.size()];
		bool match = false;
		for (const auto & r : routeSet) {
			if (pathparts /= r) {
				if (r->method == method) {
					return r;
				}
				match = true;
			}
		}
		if (!match) {
			throw Http404_NotFound();
		}
		throw Http405_MethodNotAllowed();
	}
}

