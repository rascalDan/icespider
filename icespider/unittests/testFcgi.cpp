#define BOOST_TEST_MODULE TestApp
#include <boost/test/unit_test.hpp>

#include <core.h>
#include <definedDirs.h>
#include <cgiRequestBase.h>
#include <test-fcgi.h>
#include <slicer/modelPartsTypes.h>

using namespace std::literals;
namespace std {
	template<typename T>
	ostream & operator<<(ostream & s, const std::optional<T> & o) {
		if (o) s << *o;
		return s;
	}
}

namespace std {
	ostream & operator<<(ostream & s, const IceSpider::HttpMethod & m) {
		s << Slicer::ModelPartForEnum<IceSpider::HttpMethod>::lookup(m);
		return s;
	}
}


class TestRequest : public IceSpider::CgiRequestBase {
	public:
		TestRequest(IceSpider::Core * c, char ** env) : IceSpider::CgiRequestBase(c, env)
		{
			initialize();
		}

		std::ostream & getOutputStream() const override
		{
			return out;
		}

		// LCOV_EXCL_START we never actually read or write anything here
		std::istream & getInputStream() const override
		{
			return std::cin;
		}
		// LCOV_EXCL_STOP
		mutable std::stringstream out;
};

class TestPayloadRequest : public TestRequest {
	public:
		TestPayloadRequest(IceSpider::Core * c, char ** env, std::istream & s) :
			TestRequest(c, env),
			in(s)
		{
			initialize();
		}

		std::istream & getInputStream() const override
		{
			return in;
		}

	private:
		std::istream & in;
};

class CharPtrPtrArray : public std::vector<char *> {
	public:
		CharPtrPtrArray()
		{
			push_back(NULL);
		}

		CharPtrPtrArray(const std::vector<std::string> & a)
		{
			for (const auto & e : a) {
				push_back(strdup(e.c_str()));
			}
			push_back(NULL);
		}

		~CharPtrPtrArray()
		{
			for (const auto & e : *this) {
				free(e);
			}
		}

		operator char **()
		{
			return &front();
		}
};

namespace std {
	// LCOV_EXCL_START assert failure helper only
	static
	std::ostream &
	operator<<(std::ostream & s, const IceSpider::PathElements & pe)
	{
		for (const auto & e : pe) {
			s << "/" << e;
		}
		return s;
	}
	// LCOV_EXCL_STOP
}

BOOST_FIXTURE_TEST_SUITE( CgiRequestBase, IceSpider::CoreWithDefaultRouter );

BOOST_AUTO_TEST_CASE( NoEnvironment )
{
	BOOST_REQUIRE_THROW({
		CharPtrPtrArray env;
		TestRequest r(this, env);
	}, IceSpider::Http400_BadRequest);
}

BOOST_AUTO_TEST_CASE( script_name_root )
{
	CharPtrPtrArray env ({ "SCRIPT_NAME=/" });
	TestRequest r(this, env);
	BOOST_REQUIRE(r.getRequestPath().empty());
}

BOOST_AUTO_TEST_CASE( redirect_uri_root )
{
	CharPtrPtrArray env ({ "REDIRECT_URL=/" });
	TestRequest r(this, env);
	BOOST_REQUIRE(r.getRequestPath().empty());
}

BOOST_AUTO_TEST_CASE( script_name_foobar )
{
	CharPtrPtrArray env ({ "SCRIPT_NAME=/foo/bar" });
	TestRequest r(this, env);
	BOOST_REQUIRE_EQUAL(IceSpider::PathElements({"foo", "bar"}), r.getRequestPath());
}

BOOST_AUTO_TEST_CASE( query_string_empty )
{
	CharPtrPtrArray env ({ "SCRIPT_NAME=/foo/bar", "QUERY_STRING=" });
	TestRequest r(this, env);
	BOOST_REQUIRE(!r.getQueryStringParam(""));
}

BOOST_AUTO_TEST_CASE( query_string_one )
{
	CharPtrPtrArray env ({ "SCRIPT_NAME=/foo/bar", "QUERY_STRING=one=1" });
	TestRequest r(this, env);
	BOOST_REQUIRE(!r.getQueryStringParam(""));
	BOOST_REQUIRE_EQUAL("1", *r.getQueryStringParam("one"));
}

BOOST_AUTO_TEST_CASE( query_string_two )
{
	CharPtrPtrArray env ({ "SCRIPT_NAME=/foo/bar", "QUERY_STRING=one=1&two=2" });
	TestRequest r(this, env);
	BOOST_REQUIRE(!r.getQueryStringParam(""));
	BOOST_REQUIRE_EQUAL("1", *r.getQueryStringParam("one"));
	BOOST_REQUIRE_EQUAL("2", *r.getQueryStringParam("two"));
}

BOOST_AUTO_TEST_CASE( query_string_three )
{
	CharPtrPtrArray env ({ "SCRIPT_NAME=/foo/bar", "QUERY_STRING=one=1&two=2&three=3" });
	TestRequest r(this, env);
	BOOST_REQUIRE(!r.getQueryStringParam(""));
	BOOST_REQUIRE_EQUAL("1", *r.getQueryStringParam("one"));
	BOOST_REQUIRE_EQUAL("2", *r.getQueryStringParam("two"));
	BOOST_REQUIRE_EQUAL("3", *r.getQueryStringParam("three"));
}

BOOST_AUTO_TEST_CASE( query_string_urlencoding )
{
	CharPtrPtrArray env ({ "SCRIPT_NAME=/foo/bar", "QUERY_STRING=url+%65ncoded=%53tring%2e" });
	TestRequest r(this, env);
	BOOST_REQUIRE(!r.getQueryStringParam(""));
	BOOST_REQUIRE(r.getQueryStringParam("url encoded"));
	BOOST_REQUIRE_EQUAL("String.", *r.getQueryStringParam("url encoded"));
}

BOOST_AUTO_TEST_CASE( query_string_three_emptyVal )
{
	CharPtrPtrArray env ({ "SCRIPT_NAME=/foo/bar", "QUERY_STRING=one=1&two=&three=3" });
	TestRequest r(this, env);
	BOOST_REQUIRE(!r.getQueryStringParam(""));
	BOOST_REQUIRE_EQUAL("1", *r.getQueryStringParam("one"));
	BOOST_REQUIRE_EQUAL("", *r.getQueryStringParam("two"));
	BOOST_REQUIRE_EQUAL("3", *r.getQueryStringParam("three"));
}

BOOST_AUTO_TEST_CASE( query_string_three_noVal )
{
	CharPtrPtrArray env ({ "SCRIPT_NAME=/foo/bar", "QUERY_STRING=one=1&two&three=3" });
	TestRequest r(this, env);
	BOOST_REQUIRE(!r.getQueryStringParam(""));
	BOOST_REQUIRE_EQUAL("1", *r.getQueryStringParam("one"));
	BOOST_REQUIRE_EQUAL("", *r.getQueryStringParam("two"));
	BOOST_REQUIRE_EQUAL("3", *r.getQueryStringParam("three"));
}

BOOST_AUTO_TEST_CASE( requestmethod_get )
{
	CharPtrPtrArray env ({ "SCRIPT_NAME=/", "REQUEST_METHOD=GET" });
	TestRequest r(this, env);
	BOOST_REQUIRE_EQUAL(IceSpider::HttpMethod::GET, r.getRequestMethod());
}

BOOST_AUTO_TEST_CASE( requestmethod_post )
{
	CharPtrPtrArray env ({ "SCRIPT_NAME=/", "REQUEST_METHOD=POST" });
	TestRequest r(this, env);
	BOOST_REQUIRE_EQUAL(IceSpider::HttpMethod::POST, r.getRequestMethod());
}

BOOST_AUTO_TEST_CASE( requestmethod_bad )
{
	CharPtrPtrArray env ({ "SCRIPT_NAME=/", "REQUEST_METHOD=No" });
	TestRequest r(this, env);
	BOOST_REQUIRE_THROW(r.getRequestMethod(), IceSpider::Http405_MethodNotAllowed);
}

BOOST_AUTO_TEST_CASE( requestmethod_missing )
{
	CharPtrPtrArray env ({ "SCRIPT_NAME=/" });
	TestRequest r(this, env);
	BOOST_REQUIRE_THROW(r.getRequestMethod(), IceSpider::Http400_BadRequest);
}

BOOST_AUTO_TEST_CASE( acceptheader )
{
	CharPtrPtrArray env ({ "SCRIPT_NAME=/", "HTTP_ACCEPT=text/html" });
	TestRequest r(this, env);
	BOOST_REQUIRE_EQUAL("text/html", *r.getHeaderParam("ACCEPT"));
	BOOST_REQUIRE_EQUAL("text/html", *r.getHeaderParam(IceSpider::H::ACCEPT));
}

BOOST_AUTO_TEST_CASE( missingheader )
{
	CharPtrPtrArray env ({ "SCRIPT_NAME=/", "HTTP_ACCEPT=text/html" });
	TestRequest r(this, env);
	BOOST_REQUIRE(!r.getHeaderParam("ACCEPT_LANGUAGE"));
}

BOOST_AUTO_TEST_CASE( postxwwwformurlencoded_simple )
{
	CharPtrPtrArray env ({ "SCRIPT_NAME=/", "REQUEST_METHOD=No", "CONTENT_TYPE=application/x-www-form-urlencoded" });
	std::stringstream f("value=314");
	TestPayloadRequest r(this, env, f);
	auto n = r.getBody<int>();
	BOOST_REQUIRE_EQUAL(314, n);
}

BOOST_AUTO_TEST_CASE( postxwwwformurlencoded_dictionary )
{
	CharPtrPtrArray env ({ "SCRIPT_NAME=/", "REQUEST_METHOD=No", "CONTENT_TYPE=application/x-www-form-urlencoded" });
	std::stringstream f("alpha=abcde&number=3.14&boolean=true&spaces=This+is+a%20string.&empty=");
	TestPayloadRequest r(this, env, f);
	auto n = *r.getBody<IceSpider::StringMap>();
	BOOST_REQUIRE_EQUAL(5, n.size());
	BOOST_REQUIRE_EQUAL("abcde", n["alpha"]);
	BOOST_REQUIRE_EQUAL("3.14", n["number"]);
	BOOST_REQUIRE_EQUAL("true", n["boolean"]);
	BOOST_REQUIRE_EQUAL("This is a string.", n["spaces"]);
	BOOST_REQUIRE_EQUAL("", n["empty"]);
}

BOOST_AUTO_TEST_CASE( postxwwwformurlencoded_complex )
{
	CharPtrPtrArray env ({ "SCRIPT_NAME=/", "REQUEST_METHOD=No", "CONTENT_TYPE=application/x-www-form-urlencoded" });
	std::stringstream f("alpha=abcde&number=3.14&boolean=true&empty=&spaces=This+is+a%20string.");
	TestPayloadRequest r(this, env, f);
	auto n = *r.getBody<TestFcgi::ComplexPtr>();
	BOOST_REQUIRE_EQUAL("abcde", n->alpha);
	BOOST_REQUIRE_EQUAL(3.14, n->number);
	BOOST_REQUIRE_EQUAL(true, n->boolean);
	BOOST_REQUIRE_EQUAL("This is a string.", n->spaces);
	BOOST_REQUIRE_EQUAL("", n->empty);
}

BOOST_AUTO_TEST_CASE( postjson_complex )
{
	CharPtrPtrArray env ({ "SCRIPT_NAME=/", "REQUEST_METHOD=No", "CONTENT_TYPE=application/json" });
	std::stringstream f("{\"alpha\":\"abcde\",\"number\":3.14,\"boolean\":true,\"empty\":\"\",\"spaces\":\"This is a string.\"}");
	TestPayloadRequest r(this, env, f);
	auto n = *r.getBody<TestFcgi::ComplexPtr>();
	BOOST_REQUIRE_EQUAL("abcde", n->alpha);
	BOOST_REQUIRE_EQUAL(3.14, n->number);
	BOOST_REQUIRE_EQUAL(true, n->boolean);
	BOOST_REQUIRE_EQUAL("This is a string.", n->spaces);
	BOOST_REQUIRE_EQUAL("", n->empty);
}

BOOST_AUTO_TEST_CASE( postjson_dictionary )
{
	CharPtrPtrArray env ({ "SCRIPT_NAME=/", "REQUEST_METHOD=No", "CONTENT_TYPE=application/json" });
	std::stringstream f("{\"alpha\":\"abcde\",\"number\":\"3.14\",\"boolean\":\"true\",\"empty\":\"\",\"spaces\":\"This is a string.\"}");
	TestPayloadRequest r(this, env, f);
	auto n = *r.getBody<IceSpider::StringMap>();
	BOOST_REQUIRE_EQUAL(5, n.size());
	BOOST_REQUIRE_EQUAL("abcde", n["alpha"]);
	BOOST_REQUIRE_EQUAL("3.14", n["number"]);
	BOOST_REQUIRE_EQUAL("true", n["boolean"]);
	BOOST_REQUIRE_EQUAL("This is a string.", n["spaces"]);
	BOOST_REQUIRE_EQUAL("", n["empty"]);
}

BOOST_AUTO_TEST_CASE( cookies )
{
	CharPtrPtrArray env ({ "SCRIPT_NAME=/", "REQUEST_METHOD=No", "CONTENT_TYPE=application/json", "HTTP_COOKIE=valueA=1234; value+B=Something+with+spaces." });
	TestRequest r(this, env);
	BOOST_REQUIRE_EQUAL(1234, *r.IceSpider::IHttpRequest::getCookieParam<Ice::Int>("valueA"));
	BOOST_REQUIRE_EQUAL("Something with spaces.", *r.IceSpider::IHttpRequest::getCookieParam<std::string>("value B"));
	BOOST_REQUIRE(!r.IceSpider::IHttpRequest::getCookieParam<Ice::Int>("notAThing"));
	r.setCookie("some int.", 1234, "www.com"s, "/dir"s, true, 1476142378);
	BOOST_REQUIRE_EQUAL("Set-Cookie: some+int%2e=1234; expires=Mon, 10 Oct 2016 23:32:58 GMT; domain=www.com; path=/dir; secure\r\n", r.out.str());
}

BOOST_AUTO_TEST_CASE( response )
{
	CharPtrPtrArray env ({ "SCRIPT_NAME=/" });
	TestRequest r(this, env);
	r.response(200, "OK");
	BOOST_REQUIRE_EQUAL("Status: 200 OK\r\n\r\n", r.out.str());
}

BOOST_AUTO_TEST_SUITE_END();

