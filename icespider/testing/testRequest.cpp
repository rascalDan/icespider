#include "testRequest.h"
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>

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

	const std::vector<std::string> &
	TestRequest::getRequestPath() const
	{
		return url;
	}

	HttpMethod
	TestRequest::getRequestMethod() const
	{
		return method;
	}

	IceUtil::Optional<std::string>
	TestRequest::getEnv(const std::string & key) const
	{
		return env.find(key) == env.end() ? IceUtil::Optional<std::string>() : env.find(key)->second;
	}

	IceUtil::Optional<std::string>
	TestRequest::getQueryStringParam(const std::string & key) const
	{
		return qs.find(key) == qs.end() ? IceUtil::Optional<std::string>() : qs.find(key)->second;
	}

	IceUtil::Optional<std::string>
	TestRequest::getCookieParam(const std::string & key) const
	{
		return cookies.find(key) == cookies.end() ? IceUtil::Optional<std::string>() : cookies.find(key)->second;
	}

	IceUtil::Optional<std::string>
	TestRequest::getHeaderParam(const std::string & key) const
	{
		return hdr.find(key) == hdr.end() ? IceUtil::Optional<std::string>() : hdr.find(key)->second;
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
		getOutputStream()
			<< "Status: " << statusCode << " " << statusMsg << "\r\n"
			<< "\r\n";
	}

	void
	TestRequest::setHeader(const std::string & header, const std::string & value) const
	{
		getOutputStream() << header << ": " << value << "\r\n";
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

