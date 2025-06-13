#include "testRequest.h"
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/constants.hpp>
#include <boost/algorithm/string/find_iterator.hpp>
#include <boost/algorithm/string/predicate_facade.hpp>
#include <boost/algorithm/string/split.hpp>
#include <cstdio>
#include <formatters.h>
#include <http.h>
#include <ihttpRequest.h>
#include <utility>

namespace IceSpider {
	constexpr std::string_view SLASH("/");

	TestRequest::TestRequest(const Core * core, HttpMethod method, std::string_view path) :
		IHttpRequest(core), method(method)
	{
		namespace ba = boost::algorithm;
		path.remove_prefix(1);
		if (!path.empty()) {
			ba::split(url, path, ba::is_any_of(SLASH), ba::token_compress_off);
		}
	}

	const PathElements &
	TestRequest::getRequestPath() const
	{
		return url;
	}

	PathElements &
	TestRequest::getRequestPath()
	{
		return url;
	}

	HttpMethod
	TestRequest::getRequestMethod() const
	{
		return method;
	}

	OptionalString
	TestRequest::getEnvStr(const std::string_view key) const
	{
		return get(key, env);
	}

	bool
	TestRequest::isSecure() const
	{
		return env.find("HTTPS") != env.end();
	}

	OptionalString
	TestRequest::getQueryStringParamStr(const std::string_view key) const
	{
		return get(key, qs);
	}

	OptionalString
	TestRequest::getCookieParamStr(const std::string_view key) const
	{
		return get(key, cookies);
	}

	OptionalString
	TestRequest::getHeaderParamStr(const std::string_view key) const
	{
		return get(key, hdr);
	}

	OptionalString
	TestRequest::get(const std::string_view key, const MapVars & vars)
	{
		auto iter = vars.find(key);
		if (iter == vars.end()) {
			return {};
		}
		return iter->second;
	}

	void
	TestRequest::setQueryStringParam(const std::string_view key, const OptionalString & val)
	{
		set(key, val, qs);
	}

	void
	TestRequest::setHeaderParam(const std::string_view key, const OptionalString & val)
	{
		set(key, val, hdr);
	}

	void
	TestRequest::setCookieParam(const std::string_view key, const OptionalString & val)
	{
		set(key, val, cookies);
	}

	void
	TestRequest::setEnv(const std::string_view key, const OptionalString & val)
	{
		set(key, val, env);
	}

	void
	TestRequest::set(const std::string_view key, const OptionalString & val, MapVars & vars)
	{
		if (val) {
			vars[std::string(key)] = *val;
		}
		else {
			vars.erase(vars.find(key));
		}
	}

	std::istream &
	TestRequest::getInputStream() const
	{
		return input;
	}

	std::ostream &
	TestRequest::getOutputStream() const
	{
		return output;
	}

	void
	TestRequest::response(short statusCode, const std::string_view statusMsg) const
	{
		StatusFmt::write(getOutputStream(), statusCode, statusMsg);
	}

	void
	TestRequest::setHeader(const std::string_view header, const std::string_view value) const
	{
		HdrFmt::write(getOutputStream(), header, value);
	}

	std::ostream &
	TestRequest::dump(std::ostream & strm) const
	{
		return strm;
	}

	const TestRequest::MapVars &
	TestRequest::getResponseHeaders()
	{
		if (responseHeaders.empty()) {
			while (true) {
				std::string lineBuffer;
				lineBuffer.resize_and_overwrite(BUFSIZ, [this](char * buf, size_t len) {
					output.getline(buf, static_cast<std::streamsize>(len));
					return static_cast<size_t>(output.gcount());
				});
				if (lineBuffer.empty()) {
					break;
				}
				auto colonPos = lineBuffer.find(':');
				if (colonPos == std::string::npos) {
					break;
				}
				auto valStart = lineBuffer.find_first_not_of(' ', colonPos + 1);
				auto valEnd = lineBuffer.find_first_of("\r\n", valStart);
				responseHeaders.emplace(lineBuffer.substr(0, colonPos), lineBuffer.substr(valStart, valEnd - valStart));
			}
		}
		return responseHeaders;
	}
}
