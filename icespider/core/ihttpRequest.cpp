#include "ihttpRequest.h"
#include "irouteHandler.h"
#include "util.h"
#include "exceptions.h"
#include "xwwwFormUrlEncoded.h"
#include <boost/lexical_cast.hpp>
#include <ctime>
#include <cstdio>
#include <formatters.h>

namespace IceSpider {
	using namespace AdHoc::literals;

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
				getEnv(E::CONTENT_TYPE) / []() -> std::string_view {
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
				// NOLINTNEXTLINE(hicpp-vararg)
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
			// NOLINTNEXTLINE(hicpp-vararg)
			if (fscanf(accept.get(), " ; q = %f ", &a->q) == 1) {
				if (a->q <= 0.0F || a->q > 1.0F) {
					throw Http400_BadRequest();
				}
			}
			accepts.push_back(a);
			// NOLINTNEXTLINE(hicpp-vararg)
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

	OptionalString
	IHttpRequest::getURLParam(const unsigned int & idx) const
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
			const OptionalString & d, const OptionalString & p, bool s,
			std::optional<time_t> e)
	{
		std::stringstream o;
		XWwwFormUrlEncoded::urlencodeto(o, name.begin(), name.end());
		o << '=';
		XWwwFormUrlEncoded::urlencodeto(o, value.begin(), value.end());
		if (e) {
			std::string buf(45, 0);
			struct tm tm { };
			gmtime_r(&*e, &tm);
			buf.resize(strftime(buf.data(), buf.length(), "; expires=%a, %d %b %Y %T %Z", &tm));
			o << buf;
		}
		if (d) {
			"; domain=%?"_fmt(o, *d);
		}
		if (p) {
			"; path=%?"_fmt(o, *p);
		}
		if (s){
			"; secure"_fmt(o);
		}
		"; samesite=strict"_fmt(o);
		setHeader(H::SET_COOKIE, o.str());
	}

	template <typename T>
	inline std::optional<T> optionalLexicalCast(const OptionalString & p)
	{
		if (p) {
			return wrapLexicalCast<T>(*p);
		}
		return {};
	}

	void IHttpRequest::responseRedirect(const std::string_view & url, const OptionalString & statusMsg) const
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

	static_assert(std::is_convertible<OptionalString::value_type, std::string_view>::value);
	static_assert(!std::is_convertible<OptionalString::value_type, std::string>::value);
	static_assert(std::is_constructible<OptionalString::value_type, std::string>::value);
}


