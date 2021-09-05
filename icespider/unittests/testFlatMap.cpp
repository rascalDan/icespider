#define BOOST_TEST_MODULE FlatMap
#include <boost/test/unit_test.hpp>

#include <flatMap.h>

using TM = IceSpider::flatmap<std::string_view, int>;

BOOST_TEST_DONT_PRINT_LOG_VALUE(TM::iterator)
BOOST_TEST_DONT_PRINT_LOG_VALUE(TM::const_iterator)

BOOST_FIXTURE_TEST_SUITE(sv2int, TM)

BOOST_AUTO_TEST_CASE(is_empty)
{
	BOOST_CHECK_EQUAL(size(), 0);
	BOOST_CHECK(empty());

	BOOST_CHECK_EQUAL(find(""), end());
}

BOOST_AUTO_TEST_CASE(single)
{
	insert({"a", 1});

	BOOST_CHECK_EQUAL(size(), 1);
	BOOST_CHECK(!empty());
	BOOST_CHECK_EQUAL(begin()->first, "a");
	BOOST_CHECK_EQUAL(begin()->second, 1);
	BOOST_CHECK_EQUAL(find("a"), begin());
	BOOST_CHECK_EQUAL(find("b"), end());
}

BOOST_AUTO_TEST_CASE(several)
{
	insert({"c", 3});
	BOOST_CHECK_EQUAL(lower_bound("a"), begin());

	insert({"a", 1});
	BOOST_CHECK_EQUAL(begin()->first, "a");

	insert({"f", 6});

	BOOST_CHECK_EQUAL(size(), 3);
	BOOST_CHECK(!empty());
	BOOST_CHECK_EQUAL(begin()->first, "a");
	BOOST_CHECK_EQUAL(begin()->second, 1);
	BOOST_CHECK_EQUAL(find("a"), begin());
	BOOST_CHECK_EQUAL(find("b"), end());
	BOOST_CHECK_NE(find("c"), end());
	BOOST_CHECK_EQUAL(find("c")->second, 3);
	BOOST_CHECK_NE(find("f"), end());
	BOOST_CHECK_EQUAL(find("f")->second, 6);
}

BOOST_AUTO_TEST_SUITE_END()
