#include "core.h"
#include "exceptions.h"
#include "ihttpRequest.h"
#include <Ice/Initialize.h>
#include <Ice/ObjectAdapter.h>
#include <Ice/PropertiesF.h>
#include <algorithm>
#include <compileTimeFormatter.h>
#include <cstdlib>
#include <cxxabi.h>
#include <factory.impl.h>
#include <filesystem>
#include <http.h>
#include <iostream>
#include <pathparts.h>
#include <set>
#include <string>
#include <typeinfo>

INSTANTIATEFACTORY(IceSpider::Plugin, Ice::CommunicatorPtr, Ice::PropertiesPtr);
INSTANTIATEPLUGINOF(IceSpider::ErrorHandler);

namespace IceSpider {
	const std::filesystem::path Core::DEFAULT_CONFIG("config/ice.properties");

	Core::Core(const Ice::StringSeq & args)
	{
		Ice::InitializationData initData;
		initData.properties = Ice::createProperties();
		initData.properties->parseCommandLineOptions("", args);
		auto config = initData.properties->getPropertyWithDefault("IceSpider.Config", DEFAULT_CONFIG.string());
		if (std::filesystem::exists(config)) {
			initData.properties->load(config);
		}
		communicator = Ice::initialize(initData);

		// Initialize routes
		for (const auto & routeHandleFactory : AdHoc::PluginManager::getDefault()->getAll<RouteHandlerFactory>()) {
			allRoutes.push_back(routeHandleFactory->implementation()->create(this));
		}
		std::ranges::sort(allRoutes, {}, &IRouteHandler::path);
		// Load plugins
		auto plugins = AdHoc::PluginManager::getDefault()->getAll<PluginFactory>();
		if (!plugins.empty()) {
			pluginAdapter = communicator->createObjectAdapterWithEndpoints("plugins", "default");
			for (const auto & plugin : plugins) {
				pluginAdapter->add(plugin->implementation()->create(communicator, communicator->getProperties()),
						Ice::stringToIdentity(plugin->name));
			}
			pluginAdapter->activate();
		}
	}

	Core::~Core()
	{
		// Unload plugins
		auto plugins = AdHoc::PluginManager::getDefault()->getAll<PluginFactory>();
		if (!plugins.empty()) {
			for (const auto & plugin : plugins) {
				pluginAdapter->remove(Ice::stringToIdentity(plugin->name));
			}
			pluginAdapter->deactivate();
			pluginAdapter->destroy();
		}

		if (communicator) {
			communicator->destroy();
		}
	}

	void
	// NOLINTNEXTLINE(misc-no-recursion)
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
			request->response(Http500InternalServerError::CODE, Http500InternalServerError::MESSAGE);
			request->dump(std::cerr);
		}
	}

	void
	// NOLINTNEXTLINE(misc-no-recursion)
	Core::handleError(IHttpRequest * request, const std::exception & exception) const
	{
		auto errorHandlers = AdHoc::PluginManager::getDefault()->getAll<ErrorHandler>();
		for (const auto & errorHandler : errorHandlers) {
			try {
				switch (errorHandler->implementation()->handleError(request, exception)) {
					case ErrorHandlerResult::Handled:
						return;
					case ErrorHandlerResult::Unhandled:
						continue;
					case ErrorHandlerResult::Modified:
						process(request, nullptr);
						return;
				}
			}
			catch (const HttpException & he) {
				request->response(he.code, he.message);
				return;
			}
			catch (...) {
				std::cerr << "Error handler failed\n";
			}
		}
		defaultErrorReport(request, exception);
	}

	namespace {
		auto
		demangle(const char * const name)
		{
			return std::unique_ptr<char, decltype(&std::free)>(
					__cxxabiv1::__cxa_demangle(name, nullptr, nullptr, nullptr), std::free);
		}
	}

	AdHocFormatter(LogExp, "Exception type: %?\nDetail: %?\n");

	void
	Core::defaultErrorReport(IHttpRequest * request, const std::exception & exception)
	{
		auto buf = demangle(typeid(exception).name());
		request->setHeader(H::CONTENT_TYPE, MIME::TEXT_PLAIN);
		request->response(Http500InternalServerError::CODE, buf.get());
		LogExp::write(request->getOutputStream(), buf.get(), exception.what());
		request->dump(std::cerr);
		LogExp::write(std::cerr, buf.get(), exception.what());
	}

	Ice::ObjectPrxPtr
	Core::getProxy(const std::string_view type) const
	{
		return communicator->propertyToProxy(std::string {type});
	}

	CoreWithDefaultRouter::CoreWithDefaultRouter(const Ice::StringSeq & opts) : Core(opts)
	{
		for (const auto & route : allRoutes) {
			if (routes.size() <= route->pathElementCount()) {
				routes.resize(route->pathElementCount() + 1);
			}
			auto & lroutes = routes[route->pathElementCount()];
			lroutes.push_back(route);
		}
	}

	const IceSpider::IRouteHandler *
	CoreWithDefaultRouter::findRoute(const IceSpider::IHttpRequest * request) const
	{
		const auto & pathparts = request->getRequestPath();
		const auto method = request->getRequestMethod();
		if (pathparts.size() >= routes.size()) {
			throw Http404NotFound();
		}
		const auto & routeSet = routes[pathparts.size()];
		bool match = false;
		for (const auto & route : routeSet) {
			if (route->matches(pathparts)) {
				if (route->method == method) {
					return route.get();
				}
				match = true;
			}
		}
		if (!match) {
			throw Http404NotFound();
		}
		throw Http405MethodNotAllowed();
	}

}
