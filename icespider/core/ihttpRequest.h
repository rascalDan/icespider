#ifndef ICESPIDER_IHTTPREQUEST_H
#define ICESPIDER_IHTTPREQUEST_H

#include <IceUtil/Exception.h>
#include <IceUtil/Optional.h>
#include <Ice/Current.h>
#include <visibility.h>
#include <routes.h>
#include <slicer/slicer.h>
#include <IceUtil/Optional.h>

namespace IceSpider {
	class Core;

	class DLL_PUBLIC IHttpRequest {
		public:
			IHttpRequest(const Core *);

			Ice::Context getContext() const;
			virtual std::string getRequestPath() const = 0;
			virtual UserIceSpider::HttpMethod getRequestMethod() const = 0;

			virtual IceUtil::Optional<std::string> getURLParam(const std::string &) const = 0;
			virtual IceUtil::Optional<std::string> getQueryStringParam(const std::string &) const = 0;
			virtual IceUtil::Optional<std::string> getHeaderParam(const std::string &) const = 0;
			virtual Slicer::DeserializerPtr getDeserializer() const;
			virtual Slicer::SerializerPtr getSerializer() const;
			virtual std::istream & getInputStream() const = 0;
			virtual std::ostream & getOutputStream() const = 0;

			template<typename T>
			IceUtil::Optional<T> getURLParam(const std::string & key) const;
			template<typename T>
			IceUtil::Optional<T> getBodyParam(const std::string &) const
			{
				return Slicer::DeserializeAnyWith<T>(getDeserializer());
			}
			template<typename T>
			IceUtil::Optional<T> getQueryStringParam(const std::string & key) const;
			template<typename T>
			IceUtil::Optional<T> getHeaderParam(const std::string & key) const;
			void response(short, const std::string &) const;
			template<typename T>
			void response(const T & t) const
			{
				auto s = getSerializer();
				response(200, "OK");
				Slicer::SerializeAnyWith<T>(t, s);
			}

			const Core * core;
	};
}

#endif

