#include "core.h"
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <Ice/Initialize.h>

namespace ba = boost::algorithm;

namespace IceSpider {
	Core::Core()
	{
		// Big enough to map all the request methods
		routes.resize(UserIceSpider::HttpMethod::OPTIONS + 1);
		// Initialize routes
		for (const auto & rp : AdHoc::PluginManager::getDefault()->getAll<IRouteHandler>()) {
			auto r = rp->implementation();
			auto & mroutes = routes[r->method];
			if (mroutes.size() <= r->pathElementCount()) {
				mroutes.resize(r->pathElementCount() + 1);
			}
			mroutes[r->pathElementCount()].push_back(r);
		}

		communicator = Ice::initialize({});
	}

	Core::~Core()
	{
		if (communicator) communicator->destroy();
	}

	void
	Core::process(IHttpRequest * request) const
	{
		auto routeHandler = findRoute(request);
		routeHandler->execute(request);
	}

	const IRouteHandler *
	Core::findRoute(const IHttpRequest * request) const
	{
		auto path = request->getRequestPath().substr(1);
		const auto & mroutes = routes[request->getRequestMethod()];
		std::vector<std::string> pathparts;
		if (!path.empty()) {
			ba::split(pathparts, path, ba::is_any_of("/"), ba::token_compress_off);
		}
		if (pathparts.size() > mroutes.size()) {
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

