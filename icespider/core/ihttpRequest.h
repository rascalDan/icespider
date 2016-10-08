#ifndef ICESPIDER_IHTTPREQUEST_H
#define ICESPIDER_IHTTPREQUEST_H

#include <IceUtil/Exception.h>
#include <IceUtil/Optional.h>
#include <Ice/Current.h>
#include <visibility.h>
#include <routes.h>
#include <slicer/slicer.h>
#include <IceUtil/Optional.h>
#include <boost/lexical_cast.hpp>

namespace IceSpider {
	class Core;
	class IRouteHandler;

	typedef std::vector<std::string> PathElements;
	typedef IceUtil::Optional<std::string> OptionalString;
	typedef std::pair<MimeType, Slicer::SerializerPtr> ContentTypeSerializer;

	class DLL_PUBLIC IHttpRequest {
		public:
			IHttpRequest(const Core *);

			Ice::Context getContext() const;
			virtual const PathElements & getRequestPath() const = 0;
			virtual HttpMethod getRequestMethod() const = 0;

			const std::string & getURLParam(unsigned int) const;
			virtual OptionalString getQueryStringParam(const std::string &) const = 0;
			virtual OptionalString getHeaderParam(const std::string &) const = 0;
			virtual OptionalString getCookieParam(const std::string &) const = 0;
			virtual OptionalString getEnv(const std::string &) const = 0;
			virtual Slicer::DeserializerPtr getDeserializer() const;
			virtual ContentTypeSerializer getSerializer(const IRouteHandler *) const;
			virtual std::istream & getInputStream() const = 0;
			virtual std::ostream & getOutputStream() const = 0;

			template<typename T>
			T getURLParam(unsigned int) const;
			template<typename T>
			IceUtil::Optional<T> getBody() const
			{
				return Slicer::DeserializeAnyWith<T>(getDeserializer());
			}
			template<typename T>
			IceUtil::Optional<T> getBodyParam(const IceUtil::Optional<IceSpider::StringMap> & map, const std::string & key) const
			{
				if (!map) {
					return IceUtil::Optional<T>();
				}
				auto i = map->find(key);
				if (i == map->end()) {
					return IceUtil::Optional<T>();
				}
				else {
					return boost::lexical_cast<T>(i->second);
				}
			}
			template<typename T>
			IceUtil::Optional<T> getQueryStringParam(const std::string & key) const;
			template<typename T>
			IceUtil::Optional<T> getHeaderParam(const std::string & key) const;
			template<typename T>
			IceUtil::Optional<T> getCookieParam(const std::string & key) const;
			void response(short, const std::string &) const;
			template<typename T>
			void response(const IRouteHandler * route, const T & t) const
			{
				auto s = getSerializer(route);
				getOutputStream() << "Content-Type: " << s.first.group << "/" << s.first.type << "\r\n";
				response(200, "OK");
				Slicer::SerializeAnyWith<T>(t, s.second);
			}

			const Core * core;
	};
}

#endif

