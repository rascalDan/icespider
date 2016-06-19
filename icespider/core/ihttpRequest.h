#ifndef ICESPIDER_IHTTPREQUEST_H
#define ICESPIDER_IHTTPREQUEST_H

#include <IceUtil/Exception.h>
#include <IceUtil/Optional.h>
#include <Ice/Current.h>
#include <visibility.h>
#include <routes.h>

namespace IceSpider {
	class Core;

	class DLL_PUBLIC IHttpRequest {
		public:
			IHttpRequest(const Core *);

			Ice::Context getContext() const;
			virtual std::string getRequestPath() const = 0;
			virtual UserIceSpider::HttpMethod getRequestMethod() const = 0;

			template<typename T>
			T getURLParam(const std::string & key) const { (void)key; return T(); }
			template<typename T>
			T getBodyParam(const std::string & key) const { (void)key; return T(); }
			template<typename T>
			T getQueryStringParam(const std::string & key) const { (void)key; return T(); }
			template<typename T>
			T getHeaderParam(const std::string & key) const { (void)key; return T(); }
			template<typename T>
			void response(const T &) const { }

			const Core * core;
	};
}

#endif

