#include "core.h"
#include "exceptions.h"
#include <Ice/Initialize.h>
#include <Ice/ObjectAdapter.h>
#include <compileTimeFormatter.h>
#include <cxxabi.h>
#include <factory.impl.h>
#include <filesystem>

INSTANTIATEFACTORY(IceSpider::Plugin, Ice::CommunicatorPtr, Ice::PropertiesPtr);
INSTANTIATEPLUGINOF(IceSpider::ErrorHandler);

namespace IceSpider {
	const std::filesystem::path Core::defaultConfig("config/ice.properties");

	Core::Core(const Ice::StringSeq & args)
	{
		Ice::InitializationData id;
		id.properties = Ice::createProperties();
		id.properties->parseCommandLineOptions("", args);
		auto config = id.properties->getPropertyWithDefault("IceSpider.Config", defaultConfig.string());
		if (std::filesystem::exists(config)) {
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
				pluginAdapter->add(p, Ice::stringToIdentity(pf->name));
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
				pluginAdapter->remove(Ice::stringToIdentity(pf->name));
			}
			pluginAdapter->deactivate();
			pluginAdapter->destroy();
		}

		if (communicator) {
			communicator->destroy();
		}
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
			handleError(request, e);
		}
		catch (...) {
			request->response(500, "Unknown internal server error");
			request->dump(std::cerr);
		}
	}

	void
	Core::handleError(IHttpRequest * request, const std::exception & exception) const
	{
		auto errorHandlers = AdHoc::PluginManager::getDefault()->getAll<ErrorHandler>();
		for (const auto & eh : errorHandlers) {
			try {
				switch (eh->implementation()->handleError(request, exception)) {
					case ErrorHandlerResult_Handled:
						return;
					case ErrorHandlerResult_Unhandled:
						continue;
					case ErrorHandlerResult_Modified:
						process(request, nullptr);
						return;
				}
			}
			catch (const HttpException & he) {
				request->response(he.code, he.message);
				return;
			}
			catch (...) {
				std::cerr << "Error handler failed" << std::endl;
			}
		}
		defaultErrorReport(request, exception);
	}

	auto
	demangle(const char * const name)
	{
		return std::unique_ptr<char, decltype(&std::free)>(
				__cxxabiv1::__cxa_demangle(name, nullptr, nullptr, nullptr), std::free);
	}

	AdHocFormatter(LogExp, "Exception type: %?\nDetail: %?\n");
	void
	Core::defaultErrorReport(IHttpRequest * request, const std::exception & exception) const
	{
		auto buf = demangle(typeid(exception).name());
		request->setHeader(H::CONTENT_TYPE, MIME::TEXT_PLAIN);
		request->response(500, buf.get());
		LogExp::write(request->getOutputStream(), buf.get(), exception.what());
		request->dump(std::cerr);
		LogExp::write(std::cerr, buf.get(), exception.what());
	}

	Ice::ObjectPrxPtr
	Core::getProxy(const std::string_view type) const
	{
		return communicator->propertyToProxy(std::string {type});
	}

	static bool
	operator/=(const PathElements & pathparts, const IRouteHandler * r)
	{
		auto rpi = r->parts.begin();
		for (auto ppi = pathparts.begin(); ppi != pathparts.end(); ++ppi, ++rpi) {
			if (!(*rpi)->matches(*ppi)) {
				return false;
			}
		}
		return true;
	}

	CoreWithDefaultRouter::CoreWithDefaultRouter(const Ice::StringSeq & opts) : Core(opts)
	{
		for (const auto & r : allRoutes) {
			if (routes.size() <= r->pathElementCount()) {
				routes.resize(r->pathElementCount() + 1);
			}
			auto & lroutes = routes[r->pathElementCount()];
			lroutes.push_back(r);
		}
	}

	const IceSpider::IRouteHandler *
	CoreWithDefaultRouter::findRoute(const IceSpider::IHttpRequest * request) const
	{
		const auto & pathparts = request->getRequestPath();
		const auto method = request->getRequestMethod();
		if (pathparts.size() >= routes.size()) {
			throw Http404_NotFound();
		}
		const auto & routeSet = routes[pathparts.size()];
		bool match = false;
		for (const auto & r : routeSet) {
			if (pathparts /= r.get()) {
				if (r->method == method) {
					return r.get();
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
