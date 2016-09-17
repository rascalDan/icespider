#ifndef ICESPIDER_CORE_CORE_H
#define ICESPIDER_CORE_CORE_H

#include <visibility.h>
#include <vector>
#include "irouteHandler.h"
#include <Ice/Communicator.h>
#include <boost/filesystem/path.hpp>

#define DeclareHttpEx(Name) \
	class Name : public ::IceSpider::HttpException { \
		public: \
			Name(); \
			static const int code; \
			static const std::string message; \
	}
#define DefineHttpEx(Name, Code, Message) \
	Name::Name() : ::IceSpider::HttpException(code, message) { } \
	const int Name::code(Code); \
	const std::string Name::message(Message);

namespace IceSpider {
	DeclareHttpEx(Http400_BadRequest);
	DeclareHttpEx(Http404_NotFound);
	DeclareHttpEx(Http405_MethodNotAllowed);
	DeclareHttpEx(Http406_NotAcceptable);
	DeclareHttpEx(Http415_UnsupportedMediaType);

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

