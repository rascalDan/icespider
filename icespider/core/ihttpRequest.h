#ifndef ICESPIDER_IHTTPREQUEST_H
#define ICESPIDER_IHTTPREQUEST_H

#include <IceUtil/Exception.h>
#include <IceUtil/Optional.h>
#include <Ice/Current.h>
#include <visibility.h>

namespace IceSpider {
	class DLL_PUBLIC IHttpRequest {
		public:
			Ice::Context getContext() const;
			template<typename T>
			T getURLParam(const std::string & key) const { (void)key; return T(); }
			template<typename T>
			T getBodyParam(const std::string & key) const { (void)key; return T(); }
			template<typename T>
			T getQueryStringParam(const std::string & key) const { (void)key; return T(); }
			template<typename T>
			T getHeaderParam(const std::string & key) const { (void)key; return T(); }
	};
}

#endif

