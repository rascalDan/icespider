#include "core.h"
#include "exceptions.h"
#include <Ice/Initialize.h>
#include <Ice/ObjectAdapter.h>
#include <boost/filesystem/convenience.hpp>
#include <factory.impl.h>

INSTANTIATEFACTORY(IceSpider::Plugin, Ice::CommunicatorPtr, Ice::PropertiesPtr);

namespace IceSpider {
	const boost::filesystem::path Core::defaultConfig("config/ice.properties");

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
			allRoutes.push_back(rp->implementation()->create(this));
		}
		std::sort(allRoutes.begin(), allRoutes.end(), [](const auto & a, const auto & b) {
			return a->path < b->path;
		});
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
		// Terminate routes
		for (auto r : allRoutes) {
			delete r;
		}

		if (communicator) communicator->destroy();
	}

	void
	Core::process(IHttpRequest * request, const IRouteHandler * route) const
	{
		try {
			(route ? route : findRoute(request))->execute(request);
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

	const IceSpider::IRouteHandler *
	Core::findRoute(const IceSpider::IHttpRequest *) const
	{
		throw Http404_NotFound();
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

