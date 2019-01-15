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
	typedef std::optional<std::string_view> OptionalString;
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
			std::optional<T> getBody() const
			{
				return Slicer::DeserializeAnyWith<T>(getDeserializer());
			}
			template<typename T>
			std::optional<T> getBodyParam(const std::optional<IceSpider::StringMap> & map, const std::string_view & key) const
			{
				if (!map) {
					return {};
				}
				auto i = map->find(key);
				if (i == map->end()) {
					return {};
				}
				else {
					return boost::lexical_cast<T>(i->second);
				}
			}
			void responseRedirect(const std::string_view & url, const OptionalString & = {}) const;
			void setCookie(const std::string_view &, const std::string_view &,
					const OptionalString & = {}, const OptionalString & = {},
					bool = false, std::optional<time_t> = {});
			template<typename T>
			void setCookie(const std::string_view &, const T &, const OptionalString & = {},
					const OptionalString & = {}, bool = false, std::optional<time_t> = {});
			template<typename T>
			std::optional<T> getQueryStringParam(const std::string_view & key) const;
			template<typename T>
			std::optional<T> getHeaderParam(const std::string_view & key) const;
			template<typename T>
			std::optional<T> getCookieParam(const std::string_view & key) const;
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

