#define BOOST_TEST_MODULE TestApp
#include <boost/test/unit_test.hpp>

#include <Ice/Config.h>
#include <boost/lexical_cast.hpp>
#include <cgiRequestBase.h>
#include <core.h>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <http.h>
#include <ihttpRequest.h>
#include <iostream>
#include <map>
#include <memory>
#include <optional>
#include <slicer/modelPartsTypes.h>
#include <string>
#include <string_view>
#include <test-fcgi.h>
#include <vector>

namespace IceSpider {
	class Http400_BadRequest;
}

namespace IceSpider {
	class Http405_MethodNotAllowed;
}

using namespace std::literals;

namespace std {
	template<typename T>
	ostream &
	operator<<(ostream & s, const std::optional<T> & o)
	{
		if (o) {
			s << *o;
		}
		return s;
	}
}

namespace std {
	ostream &
	operator<<(ostream & s, const IceSpider::HttpMethod & m)
	{
		s << Slicer::ModelPartForEnum<IceSpider::HttpMethod>::lookup(m);
		return s;
	}
}

class TestRequest : public IceSpider::CgiRequestBase {
public:
	TestRequest(IceSpider::Core * c, const EnvArray env) : IceSpider::CgiRequestBase(c, env) { }

	std::ostream &
	getOutputStream() const override
	{
		return out;
	}

	// LCOV_EXCL_START we never actually read or write anything here
	std::istream &
	getInputStream() const override
	{
		return std::cin;
	}

	// LCOV_EXCL_STOP

	// NOLINTNEXTLINE(misc-non-private-member-variables-in-classes)
	mutable std::stringstream out;
};

class TestPayloadRequest : public TestRequest {
public:
	TestPayloadRequest(IceSpider::Core * c, const EnvArray env, std::istream & s) : TestRequest(c, env), in(s) { }

	std::istream &
	getInputStream() const override
	{
		return in;
	}

private:
	std::istream & in;
};

namespace std {
	// LCOV_EXCL_START assert failure helper only
	static std::ostream &
	operator<<(std::ostream & s, const IceSpider::PathElements & pe)
	{
		for (const auto & e : pe) {
			s << "/" << e;
		}
		return s;
	}

	// LCOV_EXCL_STOP
}

BOOST_FIXTURE_TEST_SUITE(CgiRequestBase, IceSpider::CoreWithDefaultRouter);

BOOST_AUTO_TEST_CASE(NoEnvironment)
{
	BOOST_REQUIRE_THROW({ TestRequest r(this, {}); }, IceSpider::Http400_BadRequest);
}

BOOST_AUTO_TEST_CASE(script_name_root)
{
	TestRequest r(this, {{"SCRIPT_NAME=/"}});
	BOOST_REQUIRE(r.getRequestPath().empty());
	BOOST_CHECK(!r.isSecure());
}

BOOST_AUTO_TEST_CASE(script_name_root_https)
{
	TestRequest r(this, {{"SCRIPT_NAME=/", "HTTPS=on"}});
	BOOST_REQUIRE(r.getRequestPath().empty());
	BOOST_CHECK(r.isSecure());
}

BOOST_AUTO_TEST_CASE(redirect_uri_root)
{
	TestRequest r(this, {{"REDIRECT_URL=/"}});
	BOOST_REQUIRE(r.getRequestPath().empty());
}

BOOST_AUTO_TEST_CASE(script_name_foobar)
{
	TestRequest r(this, {{"SCRIPT_NAME=/foo/bar"}});
	BOOST_REQUIRE_EQUAL(IceSpider::PathElements({"foo", "bar"}), r.getRequestPath());
}

BOOST_AUTO_TEST_CASE(query_string_empty)
{
	TestRequest r(this, {{"SCRIPT_NAME=/foo/bar", "QUERY_STRING="}});
	BOOST_REQUIRE(!r.getQueryStringParamStr(""));
}

BOOST_AUTO_TEST_CASE(query_string_one)
{
	TestRequest r(this, {{"SCRIPT_NAME=/foo/bar", "QUERY_STRING=one=1"}});
	BOOST_REQUIRE(!r.getQueryStringParamStr(""));
	BOOST_REQUIRE_EQUAL("1", *r.getQueryStringParamStr("one"));
}

BOOST_AUTO_TEST_CASE(query_string_two)
{
	TestRequest r(this, {{"SCRIPT_NAME=/foo/bar", "QUERY_STRING=one=1&two=2"}});
	BOOST_REQUIRE(!r.getQueryStringParamStr(""));
	BOOST_REQUIRE_EQUAL("1", *r.getQueryStringParamStr("one"));
	BOOST_REQUIRE_EQUAL("2", *r.getQueryStringParamStr("two"));
}

BOOST_AUTO_TEST_CASE(query_string_three)
{
	TestRequest r(this, {{"SCRIPT_NAME=/foo/bar", "QUERY_STRING=one=1&two=2&three=3"}});
	BOOST_REQUIRE(!r.getQueryStringParamStr(""));
	BOOST_REQUIRE_EQUAL("1", *r.getQueryStringParamStr("one"));
	BOOST_REQUIRE_EQUAL("2", *r.getQueryStringParamStr("two"));
	BOOST_REQUIRE_EQUAL("3", *r.getQueryStringParamStr("three"));
}

BOOST_AUTO_TEST_CASE(query_string_urlencoding)
{
	TestRequest r(this, {{"SCRIPT_NAME=/foo/bar", "QUERY_STRING=url+%65ncoded=%53tring%2e"}});
	BOOST_REQUIRE(!r.getQueryStringParamStr(""));
	BOOST_REQUIRE(r.getQueryStringParamStr("url encoded"));
	BOOST_REQUIRE_EQUAL("String.", *r.getQueryStringParamStr("url encoded"));
}

BOOST_AUTO_TEST_CASE(query_string_three_emptyVal)
{
	TestRequest r(this, {{"SCRIPT_NAME=/foo/bar", "QUERY_STRING=one=1&two=&three=3"}});
	BOOST_REQUIRE(!r.getQueryStringParamStr(""));
	BOOST_REQUIRE_EQUAL("1", *r.getQueryStringParamStr("one"));
	BOOST_REQUIRE_EQUAL("", *r.getQueryStringParamStr("two"));
	BOOST_REQUIRE_EQUAL("3", *r.getQueryStringParamStr("three"));
}

BOOST_AUTO_TEST_CASE(query_string_three_noVal)
{
	TestRequest r(this, {{"SCRIPT_NAME=/foo/bar", "QUERY_STRING=one=1&two&three=3"}});
	BOOST_REQUIRE(!r.getQueryStringParamStr(""));
	BOOST_REQUIRE_EQUAL("1", *r.getQueryStringParamStr("one"));
	BOOST_REQUIRE_EQUAL("", *r.getQueryStringParamStr("two"));
	BOOST_REQUIRE_EQUAL("3", *r.getQueryStringParamStr("three"));
}

BOOST_AUTO_TEST_CASE(requestmethod_get)
{
	TestRequest r(this, {{"SCRIPT_NAME=/", "REQUEST_METHOD=GET"}});
	BOOST_REQUIRE_EQUAL(IceSpider::HttpMethod::GET, r.getRequestMethod());
}

BOOST_AUTO_TEST_CASE(requestmethod_post)
{
	TestRequest r {this, {{"SCRIPT_NAME=/", "REQUEST_METHOD=POST"}}};
	BOOST_REQUIRE_EQUAL(IceSpider::HttpMethod::POST, r.getRequestMethod());
}

BOOST_AUTO_TEST_CASE(requestmethod_bad)
{
	TestRequest r(this, {{"SCRIPT_NAME=/", "REQUEST_METHOD=No"}});
	BOOST_REQUIRE_THROW((void)r.getRequestMethod(), IceSpider::Http405_MethodNotAllowed);
}

BOOST_AUTO_TEST_CASE(requestmethod_missing)
{
	TestRequest r {this, {{"SCRIPT_NAME=/"}}};
	BOOST_REQUIRE_THROW((void)r.getRequestMethod(), IceSpider::Http400_BadRequest);
}

BOOST_AUTO_TEST_CASE(acceptheader)
{
	TestRequest r(this, {{"SCRIPT_NAME=/", "HTTP_ACCEPT=text/html"}});
	BOOST_REQUIRE_EQUAL("text/html", *r.getHeaderParamStr("ACCEPT"));
	BOOST_REQUIRE_EQUAL("text/html", *r.getHeaderParamStr(IceSpider::H::ACCEPT));
}

BOOST_AUTO_TEST_CASE(missingheader)
{
	TestRequest r(this, {{"SCRIPT_NAME=/", "HTTP_ACCEPT=text/html"}});
	BOOST_REQUIRE(!r.getHeaderParamStr("ACCEPT_LANGUAGE"));
}

BOOST_AUTO_TEST_CASE(postxwwwformurlencoded_simple)
{
	std::stringstream f("value=314");
	TestPayloadRequest r(this,
			{{
					"SCRIPT_NAME=/",
					"REQUEST_METHOD=No",
					"CONTENT_TYPE=application/x-www-form-urlencoded",
			}},
			f);
	auto n = r.getBody<int>();
	BOOST_REQUIRE_EQUAL(314, n);
}

BOOST_AUTO_TEST_CASE(postxwwwformurlencoded_dictionary)
{
	std::stringstream f("alpha=abcde&number=3.14&boolean=true&spaces=This+is+a%20string.&empty=");
	TestPayloadRequest r(this,
			{{
					"SCRIPT_NAME=/",
					"REQUEST_METHOD=No",
					"CONTENT_TYPE=application/x-www-form-urlencoded",
			}},
			f);
	auto n = *r.getBody<IceSpider::StringMap>();
	BOOST_REQUIRE_EQUAL(5, n.size());
	BOOST_REQUIRE_EQUAL("abcde", n["alpha"]);
	BOOST_REQUIRE_EQUAL("3.14", n["number"]);
	BOOST_REQUIRE_EQUAL("true", n["boolean"]);
	BOOST_REQUIRE_EQUAL("This is a string.", n["spaces"]);
	BOOST_REQUIRE_EQUAL("", n["empty"]);
}

BOOST_AUTO_TEST_CASE(postxwwwformurlencoded_complex)
{
	std::stringstream f("alpha=abcde&number=3.14&boolean=true&empty=&spaces=This+is+a%20string.");
	TestPayloadRequest r(this,
			{{
					"SCRIPT_NAME=/",
					"REQUEST_METHOD=No",
					"CONTENT_TYPE=application/x-www-form-urlencoded",
			}},
			f);
	auto n = *r.getBody<TestFcgi::ComplexPtr>();
	BOOST_REQUIRE_EQUAL("abcde", n->alpha);
	BOOST_REQUIRE_EQUAL(3.14, n->number);
	BOOST_REQUIRE_EQUAL(true, n->boolean);
	BOOST_REQUIRE_EQUAL("This is a string.", n->spaces);
	BOOST_REQUIRE_EQUAL("", n->empty);
}

BOOST_AUTO_TEST_CASE(postjson_complex)
{
	std::stringstream f {R"J({"alpha":"abcde","number":3.14,"boolean":true,"empty":"","spaces":"This is a string."})J"};
	TestPayloadRequest r(this, {{"SCRIPT_NAME=/", "REQUEST_METHOD=No", "CONTENT_TYPE=application/json"}}, f);
	auto n = *r.getBody<TestFcgi::ComplexPtr>();
	BOOST_REQUIRE_EQUAL("abcde", n->alpha);
	BOOST_REQUIRE_EQUAL(3.14, n->number);
	BOOST_REQUIRE_EQUAL(true, n->boolean);
	BOOST_REQUIRE_EQUAL("This is a string.", n->spaces);
	BOOST_REQUIRE_EQUAL("", n->empty);
}

BOOST_AUTO_TEST_CASE(postjson_dictionary)
{
	std::stringstream f(
			R"J({"alpha":"abcde","number":"3.14","boolean":"true","empty":"","spaces":"This is a string."})J");
	TestPayloadRequest r(this, {{"SCRIPT_NAME=/", "REQUEST_METHOD=No", "CONTENT_TYPE=application/json"}}, f);
	auto n = *r.getBody<IceSpider::StringMap>();
	BOOST_REQUIRE_EQUAL(5, n.size());
	BOOST_REQUIRE_EQUAL("abcde", n["alpha"]);
	BOOST_REQUIRE_EQUAL("3.14", n["number"]);
	BOOST_REQUIRE_EQUAL("true", n["boolean"]);
	BOOST_REQUIRE_EQUAL("This is a string.", n["spaces"]);
	BOOST_REQUIRE_EQUAL("", n["empty"]);
}

BOOST_AUTO_TEST_CASE(cookies)
{
	TestRequest r(this,
			{{
					"SCRIPT_NAME=/",
					"REQUEST_METHOD=No",
					"CONTENT_TYPE=application/json",
					"HTTP_COOKIE=valueA=1234; value+B=Something+with+spaces.",
			}});
	BOOST_REQUIRE_EQUAL(1234, *r.IceSpider::IHttpRequest::getCookieParam<Ice::Int>("valueA"));
	BOOST_REQUIRE_EQUAL("Something with spaces.", *r.IceSpider::IHttpRequest::getCookieParam<std::string>("value B"));
	BOOST_REQUIRE(!r.IceSpider::IHttpRequest::getCookieParam<Ice::Int>("notAThing"));
	r.setCookie("some int?[0]", 1234, "www.com"s, "/dir"s, true, 1476142378);
	BOOST_REQUIRE_EQUAL("Set-Cookie: some+int%3f%5b0%5d=1234; expires=Mon, 10 Oct 2016 23:32:58 GMT; domain=www.com; "
						"path=/dir; secure; samesite=strict\r\n",
			r.out.str());
}

BOOST_AUTO_TEST_CASE(response)
{
	TestRequest r(this, {{"SCRIPT_NAME=/"}});
	r.response(200, "OK");
	BOOST_REQUIRE_EQUAL("Status: 200 OK\r\n\r\n", r.out.str());
}

BOOST_AUTO_TEST_SUITE_END();
