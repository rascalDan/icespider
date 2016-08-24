#define BOOST_TEST_MODULE TestApp
#include <boost/test/unit_test.hpp>

#include <core.h>
#include <cgiRequestBase.h>

class TestRequest : public IceSpider::CgiRequestBase {
	public:
		TestRequest(IceSpider::Core * c, char ** env) : IceSpider::CgiRequestBase(c, env)
		{
			initialize();
		}

		std::ostream & getOutputStream() const override
		{
			return std::cout;
		}

		std::istream & getInputStream() const override
		{
			return std::cin;
		}
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
	static
	std::ostream &
	operator<<(std::ostream & s, const IceSpider::PathElements & pe)
	{
		for (const auto & e : pe) {
			s << "/" << e;
		}
		return s;
	}
}

BOOST_FIXTURE_TEST_SUITE( CgiRequestBase, IceSpider::Core );

BOOST_AUTO_TEST_CASE( NoEnvironment )
{
	BOOST_REQUIRE_THROW({
		CharPtrPtrArray env;
		TestRequest r(this, env);
	}, std::runtime_error);
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

BOOST_AUTO_TEST_SUITE_END();

