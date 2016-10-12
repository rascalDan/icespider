#include "ihttpRequest.h"
#include "irouteHandler.h"
#include "util.h"
#include "exceptions.h"
#include "xwwwFormUrlEncoded.h"
#include <boost/lexical_cast.hpp>
#include <time.h>

namespace IceSpider {
	IHttpRequest::IHttpRequest(const Core * c) :
		core(c)
	{
	}

	Ice::Context
	IHttpRequest::getContext() const
	{
		return Ice::Context();
	}

	Slicer::DeserializerPtr
	IHttpRequest::getDeserializer() const
	{
		try {
			return Slicer::StreamDeserializerFactory::createNew(
				getEnv("CONTENT_TYPE") / []() -> std::string {
					throw Http400_BadRequest();
				}, getInputStream());
		}
		catch (const AdHoc::NoSuchPluginException &) {
			throw Http415_UnsupportedMediaType();
		}
	}

	ContentTypeSerializer
	IHttpRequest::getSerializer(const IRouteHandler * handler) const
	{
		auto acceptHdr = getHeaderParam("Accept");
		if (acceptHdr) {
			auto accept = acceptHdr->c_str();
			std::vector<AcceptPtr> accepts;
			accepts.reserve(5);
			char grp[BUFSIZ], type[BUFSIZ];
			float pri = 0.0f;
			int chars, v;
			while ((v = sscanf(accept, " %[^ /] / %[^ ;,] %n , %n", grp, type, &chars, &chars)) == 2) {
				accept += chars;
				chars = 0;
				auto a = new Accept();
				if ((v = sscanf(accept, " ; q = %f %n , %n", &pri, &chars, &chars)) == 1) {
					a->q = pri;
				}
				if (strcmp(grp, "*")) {
					a->group = grp;
				}
				if (strcmp(type, "*")) {
					a->type = type;
				}
				accept += chars;
				accepts.push_back(a);
			}
			std::stable_sort(accepts.begin(), accepts.end(), [](const auto & a, const auto & b) { return a->q > b->q; });
			auto & strm = getOutputStream();
			for(auto & a : accepts) {
				ContentTypeSerializer serializer = handler->getSerializer(a, strm);
				if (serializer.second) {
					return serializer;
				}
			}
			throw Http406_NotAcceptable();
		}
		else {
			return handler->defaultSerializer(getOutputStream());
		}
	}

	const std::string &
	IHttpRequest::getURLParam(unsigned int idx) const
	{
		auto & url = getRequestPath();
		if (idx >= url.size()) {
			throw std::runtime_error("Bad Url parameter index");
		}
		return url[idx];
	}

	template <typename T, typename Y>
	inline T wrapLexicalCast(const Y & y)
	{
		try {
			return boost::lexical_cast<T>(y);
		}
		catch (const boost::bad_lexical_cast &) {
			throw Http400_BadRequest();
		}
	}

	// Set-Cookie: value[; expires=date][; domain=domain][; path=path][; secure]
	// Sat, 02 May 2009 23:38:25 GMT
	void IHttpRequest::setCookie(const std::string & name, const std::string & value,
			const IceUtil::Optional<std::string> & d, const IceUtil::Optional<std::string> & p, bool s,
			IceUtil::Optional<time_t> e)
	{
		auto & o = getOutputStream();
		o << "Set-Cookie: " << XWwwFormUrlEncoded::urlencode(name) <<
			'=' << XWwwFormUrlEncoded::urlencode(value);
		if (e) {
			char buf[45];
			struct tm tm;
			memset(&tm, 0, sizeof(tm));
			gmtime_r(&*e, &tm);
			auto l = strftime(buf, sizeof(buf), "; expires=%a, %d %b %Y %T %Z", &tm);
			o.write(buf, l);
		}
		if (d) o << "; domain=" << *d;
		if (p) o << "; path=" << *p;
		if (s) o << "; secure";
		o << "\r\n";
	}

	template <typename T>
	inline IceUtil::Optional<T> optionalLexicalCast(const IceUtil::Optional<std::string> & p)
	{
		if (p) return wrapLexicalCast<T>(*p);
		return IceUtil::Optional<T>();
	}

	void IHttpRequest::response(short statusCode, const std::string & statusMsg) const
	{
		getOutputStream()
			<< "Status: " << statusCode << " " << statusMsg << "\r\n"
			<< "\r\n";
	}


#define getParams(T) \
	template<> void IHttpRequest::setCookie<T>(const std::string & n, const T & v, \
					const IceUtil::Optional<std::string> & d, const IceUtil::Optional<std::string> & p, \
					bool s, IceUtil::Optional<time_t> e) { \
		setCookie(n, boost::lexical_cast<std::string>(v), d, p, s, e); } \
	template<> T IHttpRequest::getURLParam<T>(unsigned int idx) const { \
		return wrapLexicalCast<T>(getURLParam(idx)); } \
	template<> IceUtil::Optional<T> IHttpRequest::getQueryStringParam<T>(const std::string & key) const { \
		return optionalLexicalCast<T>(getQueryStringParam(key)); } \
	template<> IceUtil::Optional<T> IHttpRequest::getCookieParam<T>(const std::string & key) const { \
		return optionalLexicalCast<T>(getCookieParam(key)); } \
	template<> IceUtil::Optional<T> IHttpRequest::getHeaderParam<T>(const std::string & key) const { \
		return optionalLexicalCast<T>(getHeaderParam(key)); }
	template<> std::string IHttpRequest::getURLParam<std::string>(unsigned int idx) const {
		return getURLParam(idx); }
	template<> IceUtil::Optional<std::string> IHttpRequest::getQueryStringParam<std::string>(const std::string & key) const { \
		return getQueryStringParam(key); }
	template<> IceUtil::Optional<std::string> IHttpRequest::getCookieParam<std::string>(const std::string & key) const { \
		return getCookieParam(key); }
	template<> IceUtil::Optional<std::string> IHttpRequest::getHeaderParam<std::string>(const std::string & key) const { \
		return getHeaderParam(key); }

	getParams(bool);
	getParams(Ice::Short);
	getParams(Ice::Int);
	getParams(Ice::Long);
	getParams(Ice::Float);
	getParams(Ice::Double);
}

