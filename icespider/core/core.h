#ifndef ICESPIDER_CORE_CORE_H
#define ICESPIDER_CORE_CORE_H

#include <visibility.h>
#include <vector>
#include "irouteHandler.h"
#include <Ice/Communicator.h>
#include <boost/filesystem/path.hpp>
#include <plugins.h>

namespace IceSpider {
	class DLL_PUBLIC Core {
		public:
			typedef std::vector<const IRouteHandler *> AllRoutes;

			Core(const Ice::StringSeq & = {});
			~Core();

			virtual const IRouteHandler * findRoute(const IHttpRequest *) const;
			void process(IHttpRequest *, const IRouteHandler * = nullptr) const;

			Ice::ObjectPrx getProxy(const char * type) const;

			template<typename Interface>
			typename Interface::ProxyType getProxy() const
			{
				return Interface::ProxyType::uncheckedCast(getProxy(typeid(Interface).name()));
			}

			AllRoutes allRoutes;
			Ice::CommunicatorPtr communicator;
			Ice::ObjectAdapterPtr pluginAdapter;

			static const boost::filesystem::path defaultConfig;
	};
	class DLL_PUBLIC Plugin : public virtual Ice::Object {
	};
	typedef AdHoc::Factory<Plugin, Ice::CommunicatorPtr, Ice::PropertiesPtr> PluginFactory;
}

#endif

