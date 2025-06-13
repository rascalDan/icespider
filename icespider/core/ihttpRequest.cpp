#include "ihttpRequest.h"
#include "exceptions.h"
#include "irouteHandler.h"
#include "util.h"
#include "xwwwFormUrlEncoded.h"
#include <algorithm>
#include <compileTimeFormatter.h>
#include <cstdlib>
#include <ctime>
#include <formatters.h>
#include <http.h>
#include <plugins.h>
#include <slicer/modelParts.h>
#include <slicer/serializer.h>
#include <stdexcept>

namespace IceSpider {
	using namespace AdHoc::literals;

	IHttpRequest::IHttpRequest(const Core * core) : core(core) { }

	Ice::Context
	IHttpRequest::getContext()
	{
		return {};
	}

	Slicer::DeserializerPtr
	IHttpRequest::getDeserializer() const
	{
		try {
			return Slicer::StreamDeserializerFactory::createNew(
					getEnvStr(E::CONTENT_TYPE) / []() -> std::string_view {
						throw Http400BadRequest();
					},
					getInputStream());
		}
		catch (const AdHoc::NoSuchPluginException &) {
			throw Http415UnsupportedMediaType();
		}
	}

	Accepted
	IHttpRequest::parseAccept(std::string_view acceptHdr)
	{
		remove_leading(acceptHdr, ' ');
		if (acceptHdr.empty()) {
			throw Http400BadRequest();
		}
		Accepted accepts;
		accepts.reserve(static_cast<std::size_t>(std::count(acceptHdr.begin(), acceptHdr.end(), ',') + 1));

		auto upto = [](std::string_view & input, const std::string_view term, bool req) {
			remove_leading(input, ' ');
			const auto end = input.find_first_of(term);
			if (req && end == std::string_view::npos) {
				throw Http400BadRequest();
			}
			auto val = input.substr(0, end);
			remove_trailing(val, ' ');
			if (val.empty()) {
				throw Http400BadRequest();
			}
			if (end != std::string_view::npos) {
				const auto terminatorChar = input[end];
				input.remove_prefix(end + 1);
				return std::make_pair(val, terminatorChar);
			}
			input.remove_prefix(input.length());
			return std::make_pair(val, '\n');
		};

		while (!acceptHdr.empty()) {
			const auto group = upto(acceptHdr, "/", true).first;
			auto [type, terminatorChar] = upto(acceptHdr, ";,", false);
			Accept accept;
			if (type != "*") {
				accept.type.emplace(type);
			}
			if (group != "*") {
				accept.group.emplace(group);
			}
			else if (accept.type) {
				throw Http400BadRequest();
			}
			while (terminatorChar == ';') {
				const auto paramName = upto(acceptHdr, "=", true);
				const auto paramValue = upto(acceptHdr, ",;", false);
				if (paramName.first == "q") {
					if (convert(paramValue.first, accept.q); accept.q <= 0.0F || accept.q > 1.0F) {
						throw Http400BadRequest();
					}
				}
				terminatorChar = paramValue.second;
			}
			accepts.emplace_back(accept);
		}

		std::ranges::stable_sort(accepts, std::greater<float> {}, &Accept::q);
		return accepts;
	}

	ContentTypeSerializer
	IHttpRequest::getSerializer(const IRouteHandler * handler) const
	{
		if (auto acceptHdr = getHeaderParamStr(H::ACCEPT)) {
			auto accepts = parseAccept(*acceptHdr);
			auto & strm = getOutputStream();
			if (accepts.empty()) {
				throw Http400BadRequest();
			}
			if (!accepts.front().group && !accepts.front().type) {
				return handler->defaultSerializer(strm);
			}
			for (const auto & accept : accepts) {
				ContentTypeSerializer serializer = handler->getSerializer(accept, strm);
				if (serializer.second) {
					return serializer;
				}
			}
			throw Http406NotAcceptable();
		}
		return handler->defaultSerializer(getOutputStream());
	}

	std::string_view
	IHttpRequest::getURLParamStr(const unsigned int idx) const
	{
		auto & url = getRequestPath();
		if (idx >= url.size()) {
			throw std::runtime_error("Bad Url parameter index");
		}
		return url[idx];
	}

	// Set-Cookie: value[; expires=date][; domain=domain][; path=path][; secure]
	void
	IHttpRequest::setCookie(const std::string_view name, const std::string_view value, const OptionalString & domain,
			const OptionalString & path, bool secure, std::optional<time_t> expiry) const
	{
		std::stringstream cookieString;
		XWwwFormUrlEncoded::urlencodeto(cookieString, name.begin(), name.end());
		cookieString << '=';
		XWwwFormUrlEncoded::urlencodeto(cookieString, value.begin(), value.end());
		if (expiry) {
			static constexpr auto DATE_LENGTH = sizeof("Sat, 02 May 2009 23:38:25 GMT");
			tm timeParts {};

			gmtime_r(&*expiry, &timeParts);
			std::string buf;
			buf.resize_and_overwrite(DATE_LENGTH, [&timeParts](char * out, size_t len) {
				return strftime(out, len, "%a, %d %b %Y %T %Z", &timeParts);
			});
			cookieString << "; expires=" << buf;
		}
		if (domain) {
			"; domain=%?"_fmt(cookieString, *domain);
		}
		if (path) {
			"; path=%?"_fmt(cookieString, *path);
		}
		if (secure) {
			"; secure"_fmt(cookieString);
		}
		"; samesite=strict"_fmt(cookieString);
		setHeader(H::SET_COOKIE, std::move(cookieString).str());
	}

	void
	IHttpRequest::responseRedirect(const std::string_view url, const OptionalString & statusMsg) const
	{
		setHeader(H::LOCATION, url);
		response(303, (statusMsg ? *statusMsg : S::MOVED));
	}

	void
	IHttpRequest::modelPartResponse(const IRouteHandler * route, const Slicer::ModelPartForRootParam modelPart) const
	{
		const auto serializer = getSerializer(route);
		setHeader(H::CONTENT_TYPE, MimeTypeFmt::get(serializer.first.group, serializer.first.type));
		response(200, S::OK);
		serializer.second->Serialize(modelPart);
	}

	static_assert(std::is_convertible_v<OptionalString::value_type, std::string_view>);
	static_assert(!std::is_convertible_v<OptionalString::value_type, std::string>);
	static_assert(std::is_constructible_v<OptionalString::value_type, std::string>);
}
