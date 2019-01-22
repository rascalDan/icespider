#define BOOST_TEST_MODULE TestAccept
#include <boost/test/included/unit_test.hpp>
#include <boost/test/data/test_case.hpp>

#include <ihttpRequest.h>
#include <exceptions.h>

auto parse = IceSpider::IHttpRequest::parseAccept;
using namespace boost::unit_test::data;

namespace std {
	ostream & operator<<(ostream & s, const IceSpider::Accept & a)
	{
		s << (a.group ? *a.group : "*")
			<< "/"
			<< (a.type ? *a.type : "*")
			<< ";q=" << a.q;
		return s;
	}
}

BOOST_TEST_DECORATOR(* boost::unit_test::timeout(1))
BOOST_DATA_TEST_CASE(bad_requests, make({
	"", // Can't specify nothing
	"  ", // This is still nothing
	" group ",
	" ; ",
	" / ",
	" ^ ",
	" * / plain ",
	" text / plain ; q = 0.0 ",
	" text / plain ; q = 1.1 ",
}), a)
{
	BOOST_CHECK_THROW(parse(a), IceSpider::Http400_BadRequest);
}

BOOST_DATA_TEST_CASE(texthtml, make({
	"text/html",
	"image/png;q=0.1, text/html",
	"image/png;q=0.9, text/html;q=1.0",
	"image/png;q=0.9, text/html;q=1.0, something/else;q=1.0",
}), a)
{
	auto front = parse(a).front();
	BOOST_REQUIRE(front->group);
	BOOST_REQUIRE_EQUAL(*front->group, "text");
	BOOST_REQUIRE(front->type);
	BOOST_REQUIRE_EQUAL(*front->type, "html");
}

BOOST_DATA_TEST_CASE(textany, make({
	"text/*",
	"image/png;q=0.1, text/*",
	"image/png;q=0.9, text/*;q=1.0",
	"image/png;q=0.9, text/*;q=1.0, something/else;q=1.0",
}), a)
{
	auto front = parse(a).front();
	BOOST_REQUIRE(front->group);
	BOOST_REQUIRE_EQUAL(*front->group, "text");
	BOOST_REQUIRE(!front->type);
}

BOOST_DATA_TEST_CASE(anyhtml, make({
	"any/html",
	"image/png;q=0.1, any/html",
	"image/png;q=0.9, any/html;q=1.0",
	"image/png;q=0.9, any/html;q=1.0, something/else;q=1.0",
	" image / png ; q = 0.9 , any / html ; q = 1.0 , something / else ; q = 1.0",
}), a)
{
	auto front = parse(a).front();
	BOOST_REQUIRE(front->group);
	BOOST_REQUIRE(front->type);
	BOOST_REQUIRE_EQUAL(*front->type, "html");
}

BOOST_DATA_TEST_CASE(anyany, make({
	"*/*",
	"image/png;q=0.1, */*",
	"image/png;q=0.9, */*;q=1.0",
	"image/png;q=0.9, */*;q=1.0, something/else;q=1.0",
}), a)
{
	auto front = parse(a).front();
	BOOST_REQUIRE(!front->group);
	BOOST_REQUIRE(!front->type);
}

BOOST_DATA_TEST_CASE(q1, make({
	"*/*",
	"image/png, */*",
	"image/png;q=1.0, */*",
}), a)
{
	auto all = parse(a);
	for(const auto & accept : all) {
		BOOST_TEST_CONTEXT(*accept) {
			BOOST_CHECK_CLOSE(accept->q, 1.0, 0.1);
		}
	}
}
