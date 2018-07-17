#define BOOST_TEST_MODULE TestEmbedded
#include <boost/test/unit_test.hpp>
#include <boost/filesystem/convenience.hpp>

#include <embedded.h>
#include <thread>

class EmbeddedIceSpiderInstance : public IceSpider::Embedded::Listener {
	public:
		EmbeddedIceSpiderInstance() :
			fd(listen(18080))
		{
			BOOST_REQUIRE_GE(fd, 0);
		}

		int fd;
};

class EmbeddedIceSpiderRunner : public EmbeddedIceSpiderInstance {
	public:
		EmbeddedIceSpiderRunner() :
			th([this]{ run(); })
		{
		}

		~EmbeddedIceSpiderRunner()
		{
			unlisten(fd);
			th.join();
		}

		static bool wrk_exists()
		{
			return boost::filesystem::exists("/usr/bin/wrk");
		}

		std::thread th;
};

BOOST_AUTO_TEST_CASE(construct_destruct_cycle, * boost::unit_test::timeout(1))
{
	EmbeddedIceSpiderInstance instance;
}

BOOST_AUTO_TEST_CASE(startup_and_shutdown_cycle, * boost::unit_test::timeout(2))
{
	EmbeddedIceSpiderRunner runner;
	sleep(1);
}

BOOST_FIXTURE_TEST_SUITE(EmbeddedIceSpider, EmbeddedIceSpiderRunner);

BOOST_AUTO_TEST_CASE(simple_curl_test,
		* boost::unit_test::timeout(5))
{
	BOOST_REQUIRE_EQUAL(0, system("curl -s http://localhost:18080/"));
}

// Throw some requests at it, get a general performance overview
BOOST_AUTO_TEST_CASE(quick_siege_test,
		* boost::unit_test::timeout(5))
{
	if (wrk_exists()) {
		BOOST_REQUIRE_EQUAL(0, system("wrk -t2 http://localhost:18080/ -c10 -T 15 -d 2"));
	}
}

// Throw lots of requests at it, get a good performance overview
BOOST_AUTO_TEST_CASE(simple_performance_test,
		* boost::unit_test::disabled() // Not usually interested
		* boost::unit_test::timeout(15))
{
	if (wrk_exists()) {
		BOOST_REQUIRE_EQUAL(0, system("wrk -t20 http://localhost:18080/ -c100 -T 15"));
	}
}

BOOST_AUTO_TEST_SUITE_END();

