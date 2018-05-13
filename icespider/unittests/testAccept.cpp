#define BOOST_TEST_MODULE TestAccept
#include <boost/test/unit_test.hpp>
#include <boost/test/data/test_case.hpp>

#include <ihttpRequest.h>

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

BOOST_DATA_TEST_CASE(empty, make({
	"",
	"  ",
}), a)
{
	BOOST_REQUIRE(parse(a).empty());
}

BOOST_DATA_TEST_CASE(texthtml, make({
	"text/html",
	"image/png;q=0, text/html",
	"image/png;q=1.0, text/html;q=1.1",
	"image/png;q=1.0, text/html;q=1.1, something/else;q=1.1",
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
	"image/png;q=0, text/*",
	"image/png;q=1.0, text/*;q=1.1",
	"image/png;q=1.0, text/*;q=1.1, something/else;q=1.1",
}), a)
{
	auto front = parse(a).front();
	BOOST_REQUIRE(front->group);
	BOOST_REQUIRE_EQUAL(*front->group, "text");
	BOOST_REQUIRE(!front->type);
}

BOOST_DATA_TEST_CASE(anyhtml, make({
	"*/html",
	"image/png;q=0, */html",
	"image/png;q=1.0, */html;q=1.1",
	"image/png;q=1.0, */html;q=1.1, something/else;q=1.1",
}), a)
{
	auto front = parse(a).front();
	BOOST_REQUIRE(!front->group);
	BOOST_REQUIRE(front->type);
	BOOST_REQUIRE_EQUAL(*front->type, "html");
}

BOOST_DATA_TEST_CASE(anyany, make({
	"*/*",
	"image/png;q=0, */*",
	"image/png;q=1.0, */*;q=1.1",
	"image/png;q=1.0, */*;q=1.1, something/else;q=1.1",
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

