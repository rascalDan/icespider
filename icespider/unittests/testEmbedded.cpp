#define BOOST_TEST_MODULE TestEmbedded
#include <boost/test/included/unit_test.hpp>
#include <boost/filesystem/convenience.hpp>

#include <embedded.h>
#include <clientSocket.h>
#include <core.h>
#include <thread>

BOOST_TEST_DONT_PRINT_LOG_VALUE(IceSpider::HttpMethod);

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
		* boost::unit_test::disabled() // Not usually interested
		* boost::unit_test::timeout(5))
{
	BOOST_REQUIRE_EQUAL(0, system("curl -s http://localhost:18080/"));
}

// Throw some requests at it, get a general performance overview
BOOST_AUTO_TEST_CASE(quick_siege_test,
		* boost::unit_test::disabled() // Not usually interested
		* boost::unit_test::timeout(5))
{
	if (wrk_exists()) {
		BOOST_REQUIRE_EQUAL(0, system("wrk -t2 http://localhost:18080/ -c10 -T 15 -d 2"));
	}
}

// Throw lots of requests at it, get a good performance overview
BOOST_AUTO_TEST_CASE(simple_performance_test,
		//* boost::unit_test::disabled() // Not usually interested
		* boost::unit_test::timeout(15))
{
	if (wrk_exists()) {
		BOOST_REQUIRE_EQUAL(0, system("wrk -t20 http://localhost:18080/ -c100 -T 15"));
	}
}

BOOST_AUTO_TEST_SUITE_END();

struct TestClientSocket : public IceSpider::Embedded::ClientSocket {
	TestClientSocket() : IceSpider::Embedded::ClientSocket(0, false) { }
};

BOOST_FIXTURE_TEST_SUITE(Client, TestClientSocket);

BOOST_AUTO_TEST_CASE(get_root_1_1, * boost::unit_test::timeout(1))
{
	parse_request("GET / HTTP/1.1");
	BOOST_CHECK_EQUAL(method, IceSpider::HttpMethod::GET);
	BOOST_CHECK_EQUAL(url, "/");
	BOOST_CHECK_EQUAL(version, "HTTP/1.1");
	BOOST_CHECK(headers.empty());
}

BOOST_AUTO_TEST_CASE(post_path_1_0, * boost::unit_test::timeout(1))
{
	parse_request("POST /path/to/resource.cpp HTTP/1.0");
	BOOST_CHECK_EQUAL(method, IceSpider::HttpMethod::POST);
	BOOST_CHECK_EQUAL(url, "/path/to/resource.cpp");
	BOOST_CHECK_EQUAL(version, "HTTP/1.0");
	BOOST_CHECK(headers.empty());
}

BOOST_AUTO_TEST_CASE(get_root_1_1_hdr_alt, * boost::unit_test::timeout(1))
{
	parse_request("GET / HTTP/1.1\nAccept: text/html");
	BOOST_CHECK_EQUAL(headers.size(), 1);
	BOOST_CHECK_EQUAL(headers.begin()->first, "Accept");
	BOOST_CHECK_EQUAL(headers["Accept"], "text/html");
}

BOOST_AUTO_TEST_CASE(get_root_1_1_hdrs, * boost::unit_test::timeout(1))
{
	parse_request("GET / HTTP/1.1\r\nAccept: text/html\r\nEtag: 43");
	BOOST_CHECK_EQUAL(headers.size(), 2);
	BOOST_CHECK_EQUAL(headers["Accept"], "text/html");
	BOOST_CHECK_EQUAL(headers["Etag"], "43");
}

BOOST_AUTO_TEST_SUITE_END();

