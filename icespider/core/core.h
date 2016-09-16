#ifndef ICESPIDER_CORE_CORE_H
#define ICESPIDER_CORE_CORE_H

#include <visibility.h>
#include <vector>
#include "irouteHandler.h"
#include <Ice/Communicator.h>
#include <boost/filesystem/path.hpp>

namespace IceSpider {
	class DLL_PUBLIC Core {
		public:
			typedef std::vector<const IRouteHandler *> LengthRoutes;
			typedef std::vector<LengthRoutes> Routes;

			Core(int = 0, char ** = NULL);
			~Core();

			void process(IHttpRequest *) const;
			const IRouteHandler * findRoute(const IHttpRequest *) const;

			Ice::ObjectPrx getProxy(const char * type) const;

			template<typename Interface>
			typename Interface::ProxyType getProxy() const
			{
				return Interface::ProxyType::uncheckedCast(getProxy(typeid(Interface).name()));
			}

			Routes routes;
			Ice::CommunicatorPtr communicator;

			static const boost::filesystem::path defaultConfig;
	};
}

#endif

