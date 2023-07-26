#include "ihttpRequest.h"
#include "exceptions.h"
#include "irouteHandler.h"
#include "util.h"
#include "xwwwFormUrlEncoded.h"
#include <algorithm>
#include <boost/lexical_cast.hpp>
#include <compileTimeFormatter.h>
#include <cstdlib>
#include <ctime>
#include <formatters.h>
#include <http.h>
#include <memory>
#include <plugins.h>
#include <slicer/modelParts.h>
#include <slicer/serializer.h>
#include <stdexcept>

namespace IceSpider {
	using namespace AdHoc::literals;

	IHttpRequest::IHttpRequest(const Core * c) : core(c) { }

	Ice::Context
	IHttpRequest::getContext() const
	{
		return {};
	}

	Slicer::DeserializerPtr
	IHttpRequest::getDeserializer() const
	{
		try {
			return Slicer::StreamDeserializerFactory::createNew(
					getEnv(E::CONTENT_TYPE) / []() -> std::string_view {
						throw Http400_BadRequest();
					},
					getInputStream());
		}
		catch (const AdHoc::NoSuchPluginException &) {
			throw Http415_UnsupportedMediaType();
		}
	}

	Accepted
	IHttpRequest::parseAccept(std::string_view acceptHdr)
	{
		remove_leading(acceptHdr, ' ');
		if (acceptHdr.empty()) {
			throw Http400_BadRequest();
		}
		Accepted accepts;
		accepts.reserve(static_cast<std::size_t>(std::count(acceptHdr.begin(), acceptHdr.end(), ',') + 1));

		auto upto = [](std::string_view & in, const std::string_view term, bool req) {
			remove_leading(in, ' ');
			const auto end = in.find_first_of(term);
			if (req && end == std::string_view::npos) {
				throw Http400_BadRequest();
			}
			auto val = in.substr(0, end);
			remove_trailing(val, ' ');
			if (val.empty()) {
				throw Http400_BadRequest();
			}
			if (end != std::string_view::npos) {
				const auto tc = in[end];
				in.remove_prefix(end + 1);
				return std::make_pair(val, tc);
			}
			else {
				in.remove_prefix(in.length());
				return std::make_pair(val, '\n');
			}
		};

		while (!acceptHdr.empty()) {
			const auto group = upto(acceptHdr, "/", true).first;
			auto [type, tc] = upto(acceptHdr, ";,", false);
			Accept a;
			if (type != "*") {
				a.type.emplace(type);
			}
			if (group != "*") {
				a.group.emplace(group);
			}
			else if (a.type) {
				throw Http400_BadRequest();
			}
			while (tc == ';') {
				const auto paramName = upto(acceptHdr, "=", true);
				const auto paramValue = upto(acceptHdr, ",;", false);
				if (paramName.first == "q") {
					a.q = std::strtof(std::string(paramValue.first).c_str(), nullptr);
					if (a.q <= 0.0F || a.q > 1.0F) {
						throw Http400_BadRequest();
					}
				}
				tc = paramValue.second;
			}
			accepts.push_back(a);
		}

		std::stable_sort(accepts.begin(), accepts.end(), [](const auto & a, const auto & b) {
			return a.q > b.q;
		});
		return accepts;
	}

	ContentTypeSerializer
	IHttpRequest::getSerializer(const IRouteHandler * handler) const
	{
		auto acceptHdr = getHeaderParam(H::ACCEPT);
		if (acceptHdr) {
			auto accepts = parseAccept(*acceptHdr);
			auto & strm = getOutputStream();
			if (accepts.empty()) {
				throw Http400_BadRequest();
			}
			if (!accepts.front().group && !accepts.front().type) {
				return handler->defaultSerializer(strm);
			}
			for (auto & a : accepts) {
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

	template<typename T, typename Y>
	inline T
	wrapLexicalCast(const Y & y)
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
	void
	IHttpRequest::setCookie(const std::string_view & name, const std::string_view & value, const OptionalString & d,
			const OptionalString & p, bool s, std::optional<time_t> e)
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
		if (s) {
			"; secure"_fmt(o);
		}
		"; samesite=strict"_fmt(o);
		setHeader(H::SET_COOKIE, o.str());
	}

	template<typename T>
	inline std::optional<T>
	optionalLexicalCast(const OptionalString & p)
	{
		if (p) {
			return wrapLexicalCast<T>(*p);
		}
		return {};
	}

	void
	IHttpRequest::responseRedirect(const std::string_view & url, const OptionalString & statusMsg) const
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
