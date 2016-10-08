#include "core.h"
#include "exceptions.h"
#include <Ice/Initialize.h>
#include <Ice/ObjectAdapter.h>
#include <boost/filesystem/convenience.hpp>
#include <factory.impl.h>

INSTANTIATEFACTORY(IceSpider::Plugin, Ice::CommunicatorPtr, Ice::PropertiesPtr);

namespace IceSpider {
	const boost::filesystem::path Core::defaultConfig("config/ice.properties");

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

	Core::Core(const Ice::StringSeq & args)
	{
		Ice::InitializationData id;
		id.properties = Ice::createProperties();
		id.properties->parseCommandLineOptions("", args);
		auto config = id.properties->getPropertyWithDefault("IceSpider.Config", defaultConfig.string());
		if (boost::filesystem::exists(config)) {
			id.properties->load(config);
		}
		communicator = Ice::initialize(id);

		// Initialize routes
		for (const auto & rp : AdHoc::PluginManager::getDefault()->getAll<RouteHandlerFactory>()) {
			auto r = rp->implementation()->create(this);
			if (routes.size() <= r->pathElementCount()) {
				routes.resize(r->pathElementCount() + 1);
			}
			auto & lroutes = routes[r->pathElementCount()];
			lroutes.push_back(r);
		}
		for (auto & l : routes) {
			std::sort(l.begin(), l.end(), [](const auto & a, const auto & b) {
				return a->path < b->path;
			});
		}
		// Load plugins
		auto plugins = AdHoc::PluginManager::getDefault()->getAll<PluginFactory>();
		if (!plugins.empty()) {
			pluginAdapter = communicator->createObjectAdapterWithEndpoints("plugins", "default");
			for (const auto & pf : plugins) {
				auto p = pf->implementation()->create(communicator, communicator->getProperties());
				pluginAdapter->add(p, communicator->stringToIdentity(pf->name));
			}
			pluginAdapter->activate();
		}
	}

	Core::~Core()
	{
		// Unload plugins
		auto plugins = AdHoc::PluginManager::getDefault()->getAll<PluginFactory>();
		if (!plugins.empty()) {
			for (const auto & pf : plugins) {
				pluginAdapter->remove(communicator->stringToIdentity(pf->name));
			}
			pluginAdapter->deactivate();
			pluginAdapter->destroy();
		}
		// Initialize routes
		for (auto l : routes) {
			for (auto r : l) {
				delete r;
			}
		}

		if (communicator) communicator->destroy();
	}

	void
	Core::process(IHttpRequest * request) const
	{
		try {
			findRoute(request)->execute(request);
		}
		catch (const HttpException & he) {
			request->response(he.code, he.message);
		}
		catch (const std::exception & e) {
			request->response(500, e.what());
		}
		catch (...) {
			request->response(500, "Unknown internal server error");
		}
	}

	const IRouteHandler *
	Core::findRoute(const IHttpRequest * request) const
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

	Ice::ObjectPrx
	Core::getProxy(const char * type) const
	{
		char * buf = __cxxabiv1::__cxa_demangle(type, NULL, NULL, NULL);
		char * c = buf;
		int off = 0;
		while (*c) {
			if (*(c + 1) == ':' && *c == ':') {
				*c = '.';
				off += 1;
			}
			else if (off) {
				*c = *(c + off);
			}
			c += 1;
		}
		auto i = communicator->propertyToProxy(buf);
		free(buf);
		return i;
	}
}

