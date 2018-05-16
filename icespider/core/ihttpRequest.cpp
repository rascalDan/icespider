#include "ihttpRequest.h"
#include "irouteHandler.h"
#include "util.h"
#include "exceptions.h"
#include "xwwwFormUrlEncoded.h"
#include <boost/lexical_cast.hpp>
#include <time.h>
#include <stdio.h>
#include <formatters.h>

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
				getEnv(E::CONTENT_TYPE) / []() -> std::string {
					throw Http400_BadRequest();
				}, getInputStream());
		}
		catch (const AdHoc::NoSuchPluginException &) {
			throw Http415_UnsupportedMediaType();
		}
	}

	Accepted
	IHttpRequest::parseAccept(const std::string_view & acceptHdr)
	{
		auto accept = std::unique_ptr<FILE, decltype(&fclose)>(
				fmemopen(const_cast<char *>(acceptHdr.data()), acceptHdr.length(), "r"), &fclose);
		Accepted accepts;
		accepts.reserve(acceptHdr.length() / 15);
		int grpstart, grpend, typestart, typeend;
		auto reset = [&]() {
			grpstart = grpend = typestart = typeend = 0;
			return ftell(accept.get());
		};

		for (auto off = reset();
				fscanf(accept.get(), " %n%*[^ /]%n / %n%*[^ ;,]%n", &grpstart, &grpend, &typestart, &typeend) == 0;
				off = reset()) {
			if (grpend <= grpstart || typestart <= grpend || typeend <= typestart) {
				throw Http400_BadRequest();
			}
			auto a = std::make_shared<Accept>();
			if (auto g = acceptHdr.substr(typestart + off, typeend - typestart); g != "*") {
				a->type.emplace(g);
			}
			if (auto t = acceptHdr.substr(grpstart + off, grpend - grpstart); t != "*") {
				a->group.emplace(t);
			}
			else if (a->type) {
				throw Http400_BadRequest();
			}
			if (fscanf(accept.get(), " ; q = %f ", &a->q) == 1) {
				if (a->q <= 0.0f || a->q > 1.0f) {
					throw Http400_BadRequest();
				}
			}
			accepts.push_back(a);
			if (fscanf(accept.get(), " ,") != 0) {
				break;
			}
		}

		if (accepts.empty()) {
			throw Http400_BadRequest();
		}
		std::stable_sort(accepts.begin(), accepts.end(), [](const auto & a, const auto & b) { return a->q > b->q; });
		return accepts;
	}

	ContentTypeSerializer
	IHttpRequest::getSerializer(const IRouteHandler * handler) const
	{
		auto acceptHdr = getHeaderParam(H::ACCEPT);
		if (acceptHdr) {
			auto accepts = parseAccept(*acceptHdr);
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
	void IHttpRequest::setCookie(const std::string_view & name, const std::string_view & value,
			const Ice::optional<std::string> & d, const Ice::optional<std::string> & p, bool s,
			Ice::optional<time_t> e)
	{
		std::stringstream o;
		o << XWwwFormUrlEncoded::urlencode(name) <<
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
		setHeader(H::SET_COOKIE, o.str());
	}

	template <typename T>
	inline Ice::optional<T> optionalLexicalCast(const Ice::optional<std::string> & p)
	{
		if (p) return wrapLexicalCast<T>(*p);
		return Ice::optional<T>();
	}

	void IHttpRequest::responseRedirect(const std::string_view & url, const Ice::optional<std::string> & statusMsg) const
	{
		setHeader(H::LOCATION, url);
		response(303, (statusMsg ? *statusMsg : S::MOVED));
	}

	void
	IHttpRequest::modelPartResponse(const IRouteHandler * route, const Slicer::ModelPartForRootPtr & mp) const
	{
		auto s = getSerializer(route);
		setHeader(H::CONTENT_TYPE, MimeTypeFmt::get(s.first.group, s.first.type));
		response(200, S::OK);
		s.second->Serialize(mp);
	}

#define getParams(T) \
	template<> void IHttpRequest::setCookie<T>(const std::string_view & n, const T & v, \
					const Ice::optional<std::string> & d, const Ice::optional<std::string> & p, \
					bool s, Ice::optional<time_t> e) { \
		auto vs = boost::lexical_cast<std::string>(v); \
		setCookie(n, std::string_view(vs), d, p, s, e); \
	} \
	template<> T IHttpRequest::getURLParam<T>(unsigned int idx) const { \
		return wrapLexicalCast<T>(getURLParam(idx)); } \
	template<> Ice::optional<T> IHttpRequest::getQueryStringParam<T>(const std::string_view & key) const { \
		return optionalLexicalCast<T>(getQueryStringParam(key)); } \
	template<> Ice::optional<T> IHttpRequest::getCookieParam<T>(const std::string_view & key) const { \
		return optionalLexicalCast<T>(getCookieParam(key)); } \
	template<> Ice::optional<T> IHttpRequest::getHeaderParam<T>(const std::string_view & key) const { \
		return optionalLexicalCast<T>(getHeaderParam(key)); }
	template<> std::string IHttpRequest::getURLParam<std::string>(unsigned int idx) const {
		return getURLParam(idx); }
	template<> Ice::optional<std::string> IHttpRequest::getQueryStringParam<std::string>(const std::string_view & key) const { \
		return getQueryStringParam(key); }
	template<> Ice::optional<std::string> IHttpRequest::getCookieParam<std::string>(const std::string_view & key) const { \
		return getCookieParam(key); }
	template<> Ice::optional<std::string> IHttpRequest::getHeaderParam<std::string>(const std::string_view & key) const { \
		return getHeaderParam(key); }

	getParams(bool);
	getParams(Ice::Short);
	getParams(Ice::Int);
	getParams(Ice::Long);
	getParams(Ice::Float);
	getParams(Ice::Double);
}

