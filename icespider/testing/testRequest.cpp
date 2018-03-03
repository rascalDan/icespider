#include "testRequest.h"
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <formatters.h>

namespace IceSpider {
	TestRequest::TestRequest(const Core * c, HttpMethod m, const std::string & p) :
		IHttpRequest(c),
		method(m)
	{
		namespace ba = boost::algorithm;
		auto path = p.substr(1);
		if (!path.empty()) {
			ba::split(url, path, ba::is_any_of("/"), ba::token_compress_off);
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
	TestRequest::getEnv(const std::string & key) const
	{
		return get(key, env);
	}

	OptionalString
	TestRequest::getQueryStringParam(const std::string & key) const
	{
		return get(key, qs);
	}

	OptionalString
	TestRequest::getCookieParam(const std::string & key) const
	{
		return get(key, cookies);
	}

	OptionalString
	TestRequest::getHeaderParam(const std::string & key) const
	{
		return get(key, hdr);
	}

	OptionalString
	TestRequest::get(const std::string & key, const MapVars & vars) const
	{
		auto i = vars.find(key);
		if (i == vars.end()) {
			return IceUtil::None;
		}
		return i->second;
	}

	void
	TestRequest::setQueryStringParam(const std::string & key, const OptionalString & val)
	{
		set(key, val, qs);
	}

	void
	TestRequest::setHeaderParam(const std::string & key, const OptionalString & val)
	{
		set(key, val, hdr);
	}
		
	void
	TestRequest::setCookieParam(const std::string & key, const OptionalString & val)
	{
		set(key, val, cookies);
	}

	void
	TestRequest::setEnv(const std::string & key, const OptionalString & val)
	{
		set(key, val, env);
	}
		
	void
	TestRequest::set(const std::string & key, const OptionalString & val, MapVars & vars)
	{
		if (val)
			vars[key] = *val;
		else
			vars.erase(key);
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
	TestRequest::response(short statusCode, const std::string & statusMsg) const
	{
		StatusFmt::write(getOutputStream(), statusCode, statusMsg);
	}

	void
	TestRequest::setHeader(const std::string & header, const std::string & value) const
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
				char buf[BUFSIZ], n[BUFSIZ], v[BUFSIZ];
				output.getline(buf, BUFSIZ);
				if (sscanf(buf, "%[^:]: %[^\r]", n, v) != 2) {
					break;
				}
				responseHeaders[n] = v;
			}
		}
		return responseHeaders;
	}
}

