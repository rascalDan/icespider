#ifndef ICESPIDER_IHTTPREQUEST_H
#define ICESPIDER_IHTTPREQUEST_H

#include <IceUtil/Exception.h>
#include <IceUtil/Optional.h>
#include <Ice/Current.h>
#include <visibility.h>
#include <http.h>
#include <slicer/slicer.h>
#include <Ice/Optional.h>
#include <boost/lexical_cast.hpp>

namespace IceSpider {
	class Core;
	class IRouteHandler;

	typedef std::vector<std::string> PathElements;
	typedef Ice::optional<std::string> OptionalString;
	typedef std::pair<MimeType, Slicer::SerializerPtr> ContentTypeSerializer;

	class DLL_PUBLIC IHttpRequest {
		public:
			IHttpRequest(const Core *);

			Ice::Context getContext() const;
			virtual const PathElements & getRequestPath() const = 0;
			virtual PathElements & getRequestPath() = 0;
			virtual HttpMethod getRequestMethod() const = 0;

			const std::string & getURLParam(unsigned int) const;
			virtual OptionalString getQueryStringParam(const std::string_view &) const = 0;
			virtual OptionalString getHeaderParam(const std::string_view &) const = 0;
			virtual OptionalString getCookieParam(const std::string_view &) const = 0;
			virtual OptionalString getEnv(const std::string_view &) const = 0;
			static Accepted parseAccept(const std::string_view &);
			virtual Slicer::DeserializerPtr getDeserializer() const;
			virtual ContentTypeSerializer getSerializer(const IRouteHandler *) const;
			virtual std::istream & getInputStream() const = 0;
			virtual std::ostream & getOutputStream() const = 0;
			virtual void setHeader(const std::string_view &, const std::string_view &) const = 0;

			virtual std::ostream & dump(std::ostream & s) const = 0;

			template<typename T>
			T getURLParam(unsigned int) const;
			template<typename T>
			Ice::optional<T> getBody() const
			{
				return Slicer::DeserializeAnyWith<T>(getDeserializer());
			}
			template<typename T>
			Ice::optional<T> getBodyParam(const Ice::optional<IceSpider::StringMap> & map, const std::string_view & key) const
			{
				if (!map) {
					return Ice::optional<T>();
				}
				auto i = map->find(key);
				if (i == map->end()) {
					return Ice::optional<T>();
				}
				else {
					return boost::lexical_cast<T>(i->second);
				}
			}
			void responseRedirect(const std::string_view & url, const Ice::optional<std::string> & = IceUtil::None) const;
			void setCookie(const std::string_view &, const std::string_view &,
					const Ice::optional<std::string> & = IceUtil::None, const Ice::optional<std::string> & = IceUtil::None,
					bool = false, Ice::optional<time_t> = IceUtil::None);
			template<typename T>
			void setCookie(const std::string_view &, const T &, const Ice::optional<std::string> & = IceUtil::None,
					const Ice::optional<std::string> & = IceUtil::None, bool = false, Ice::optional<time_t> = IceUtil::None);
			template<typename T>
			Ice::optional<T> getQueryStringParam(const std::string_view & key) const;
			template<typename T>
			Ice::optional<T> getHeaderParam(const std::string_view & key) const;
			template<typename T>
			Ice::optional<T> getCookieParam(const std::string_view & key) const;
			virtual void response(short, const std::string_view &) const = 0;
			template<typename T>
			void response(const IRouteHandler * route, const T & t) const
			{
				modelPartResponse(route, Slicer::ModelPart::CreateRootFor(t));
			}
			void modelPartResponse(const IRouteHandler * route, const Slicer::ModelPartForRootPtr &) const;

			const Core * core;
	};
}

#endif

