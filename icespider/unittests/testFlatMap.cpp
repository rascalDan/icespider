#define BOOST_TEST_MODULE FlatMap
#include <boost/test/unit_test.hpp>

#include <flatMap.h>

using TM = IceSpider::flatmap<std::string_view, int>;

BOOST_TEST_DONT_PRINT_LOG_VALUE(TM::const_iterator)

BOOST_FIXTURE_TEST_SUITE(sv2int, TM)

BOOST_AUTO_TEST_CASE(is_empty)
{
	BOOST_CHECK_EQUAL(size(), 0);
	BOOST_CHECK(empty());

	BOOST_CHECK_EQUAL(find(""), end());
	BOOST_CHECK(!contains(""));
}

BOOST_AUTO_TEST_CASE(single)
{
	insert({"a", 1});

	BOOST_CHECK_EQUAL(size(), 1);
	BOOST_CHECK(!empty());
	BOOST_CHECK(!contains(""));
	BOOST_CHECK(contains("a"));
	BOOST_CHECK_EQUAL(at("a"), 1);
	BOOST_CHECK(!contains("b"));
	BOOST_CHECK_THROW(at("b"), std::out_of_range);
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
	BOOST_CHECK(!contains(""));
	BOOST_CHECK(contains("a"));
	BOOST_CHECK(!contains("b"));
	BOOST_CHECK(contains("c"));
	BOOST_CHECK(contains("f"));
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

using TMI = IceSpider::flatmap<int, std::string_view>;

BOOST_TEST_DONT_PRINT_LOG_VALUE(TMI::const_iterator)

BOOST_FIXTURE_TEST_SUITE(int2sv, TMI)

BOOST_AUTO_TEST_CASE(several)
{
	insert({3, "c"});
	BOOST_CHECK_EQUAL(lower_bound(1), begin());

	insert({1, "a"});
	BOOST_CHECK_EQUAL(begin()->first, 1);

	insert({6, "f"});

	BOOST_CHECK_EQUAL(size(), 3);
	BOOST_CHECK(!empty());
	BOOST_CHECK(!contains(0));
	BOOST_CHECK(contains(1));
	BOOST_CHECK_EQUAL(at(1), "a");
	BOOST_CHECK(!contains(2));
	BOOST_CHECK_THROW(at(2), std::out_of_range);
	BOOST_CHECK(contains(3));
	BOOST_CHECK(contains(6));
	BOOST_CHECK_EQUAL(begin()->first, 1);
	BOOST_CHECK_EQUAL(begin()->second, "a");
	BOOST_CHECK_EQUAL(find(1), begin());
	BOOST_CHECK_EQUAL(find(2), end());
	BOOST_CHECK_NE(find(3), end());
	BOOST_CHECK_EQUAL(find(3)->second, "c");
	BOOST_CHECK_NE(find(6), end());
	BOOST_CHECK_EQUAL(find(6)->second, "f");
}

BOOST_AUTO_TEST_SUITE_END()
