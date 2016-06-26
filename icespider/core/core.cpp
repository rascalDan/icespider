#include "core.h"
#include <Ice/Initialize.h>

namespace IceSpider {
	Core::Core(int argc, char ** argv)
	{
		// Big enough to map all the request methods (an empty of zero lenght routes as default)
		routes.resize(HttpMethod::OPTIONS + 1, {{ }});
		// Initialize routes
		for (const auto & rp : AdHoc::PluginManager::getDefault()->getAll<IRouteHandler>()) {
			auto r = rp->implementation();
			auto & mroutes = routes[r->method];
			if (mroutes.size() <= r->pathElementCount()) {
				mroutes.resize(r->pathElementCount() + 1);
			}
			mroutes[r->pathElementCount()].push_back(r);
		}

		Ice::InitializationData id;
		id.properties = Ice::createProperties(argc, argv);
		communicator = Ice::initialize(id);
	}

	Core::~Core()
	{
		if (communicator) communicator->destroy();
	}

	void
	Core::process(IHttpRequest * request) const
	{
		auto routeHandler = findRoute(request);
		if (routeHandler) {
			routeHandler->execute(request);
		}
		else {
			request->response(404, "Not found");
		}
	}

	const IRouteHandler *
	Core::findRoute(const IHttpRequest * request) const
	{
		const auto & pathparts = request->getRequestPath();
		const auto & mroutes = routes[request->getRequestMethod()];
		if (pathparts.size() >= mroutes.size()) {
			// Not found error
			return NULL;
		}
		const auto & routeSet = mroutes[pathparts.size()];
		auto ri = std::find_if(routeSet.begin(), routeSet.end(), [&pathparts](const auto & r) {
			auto rpi = r->parts.begin();
			for (auto ppi = pathparts.begin(); ppi != pathparts.end(); ++ppi, ++rpi) {
				if (!(*rpi)->matches(*ppi)) return false;
			}
			return true;
		});
		if (ri == routeSet.end()) {
			// Not found error
			return NULL;
		}
		return (*ri);
	}

	Ice::ObjectPrx
	Core::getProxy(const char * type) const
	{
		return communicator->propertyToProxy(type);
	}
}

