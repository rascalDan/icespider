#include "testRequest.h"
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <formatters.h>

namespace IceSpider {
	constexpr std::string_view slash("/");
	TestRequest::TestRequest(const Core * c, HttpMethod m, const std::string_view & p) :
		IHttpRequest(c),
		method(m)
	{
		namespace ba = boost::algorithm;
		auto path = p.substr(1);
		// NOLINTNEXTLINE(clang-analyzer-cplusplus.NewDeleteLeaks)
		if (!path.empty()) {
			// NOLINTNEXTLINE(clang-analyzer-cplusplus.NewDeleteLeaks)
			ba::split(url, path, ba::is_any_of(slash), ba::token_compress_off);
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
	TestRequest::getEnv(const std::string_view & key) const
	{
		return get(key, env);
	}

	OptionalString
	TestRequest::getQueryStringParam(const std::string_view & key) const
	{
		return get(key, qs);
	}

	OptionalString
	TestRequest::getCookieParam(const std::string_view & key) const
	{
		return get(key, cookies);
	}

	OptionalString
	TestRequest::getHeaderParam(const std::string_view & key) const
	{
		return get(key, hdr);
	}

	OptionalString
	TestRequest::get(const std::string_view & key, const MapVars & vars) const
	{
		auto i = vars.find(key);
		if (i == vars.end()) {
			return {};
		}
		return i->second;
	}

	void
	TestRequest::setQueryStringParam(const std::string_view & key, const OptionalString & val)
	{
		set(key, val, qs);
	}

	void
	TestRequest::setHeaderParam(const std::string_view & key, const OptionalString & val)
	{
		set(key, val, hdr);
	}

	void
	TestRequest::setCookieParam(const std::string_view & key, const OptionalString & val)
	{
		set(key, val, cookies);
	}

	void
	TestRequest::setEnv(const std::string_view & key, const OptionalString & val)
	{
		set(key, val, env);
	}

	void
	TestRequest::set(const std::string_view & key, const OptionalString & val, MapVars & vars)
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
	TestRequest::response(short statusCode, const std::string_view & statusMsg) const
	{
		StatusFmt::write(getOutputStream(), statusCode, statusMsg);
	}

	void
	TestRequest::setHeader(const std::string_view & header, const std::string_view & value) const
	{
		HdrFmt::write(getOutputStream(), header, value);
	}

	std::ostream &
	TestRequest::dump(std::ostream & s) const
	{
		return s;
	}

	const TestRequest::MapVars &
	TestRequest::getResponseHeaders()
	{
		if (responseHeaders.empty()) {
			while (true) {
				std::array<char, BUFSIZ> buf {}, n {}, v {};
				output.getline(buf.data(), BUFSIZ);
				// NOLINTNEXTLINE(hicpp-vararg)
				if (sscanf(buf.data(), "%[^:]: %[^\r]", n.data(), v.data()) != 2) {
					break;
				}
				responseHeaders[n.data()] = v.data();
			}
		}
		return responseHeaders;
	}
}

