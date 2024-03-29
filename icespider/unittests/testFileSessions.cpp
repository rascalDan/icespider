#define BOOST_TEST_MODULE TestFileSessions
#include <boost/test/unit_test.hpp>

#include <Ice/Communicator.h>
#include <Ice/Properties.h>
#include <Ice/Proxy.h>
#include <core.h>
#include <ctime>
#include <definedDirs.h>
#include <filesystem>
#include <map>
#include <memory>
#include <session.h>
#include <string>
#include <string_view>
#include <unistd.h>

BOOST_TEST_DONT_PRINT_LOG_VALUE(IceSpider::Variables);

class TestCore : public IceSpider::CoreWithDefaultRouter {
public:
	TestCore() :
		IceSpider::CoreWithDefaultRouter({"--IceSpider.SessionManager=IceSpider-FileSessions",
				"--IceSpider.FileSessions.Path=" + (binDir / "test-sessions").string(),
				"--IceSpider.FileSessions.Duration=0"}),
		root(communicator->getProperties()->getProperty("IceSpider.FileSessions.Path"))
	{
	}

	// NOLINTNEXTLINE(misc-non-private-member-variables-in-classes)
	const std::filesystem::path root;
};

BOOST_AUTO_TEST_CASE(clear)
{
	TestCore tc;
	if (std::filesystem::exists(tc.root)) {
		std::filesystem::remove_all(tc.root);
	}
}

BOOST_FIXTURE_TEST_SUITE(Core, TestCore);

BOOST_AUTO_TEST_CASE(ping)
{
	auto prx = this->getProxy<IceSpider::SessionManager>();
	BOOST_REQUIRE(prx);
	prx->ice_ping();
}

BOOST_AUTO_TEST_CASE(createAndDestroy)
{
	auto prx = this->getProxy<IceSpider::SessionManager>();
	auto s = prx->createSession();
	BOOST_REQUIRE(std::filesystem::exists(root / s->id));
	BOOST_REQUIRE_EQUAL(0, s->duration);
	BOOST_REQUIRE_EQUAL(time(nullptr), s->lastUsed);
	prx->destroySession(s->id);
	BOOST_REQUIRE(!std::filesystem::exists(root / s->id));
}

BOOST_AUTO_TEST_CASE(createAndChangeRestore)
{
	auto prx = this->getProxy<IceSpider::SessionManager>();
	auto s = prx->createSession();
	s->variables["a"] = "value";
	prx->updateSession(s);

	auto s2 = prx->getSession(s->id);
	BOOST_REQUIRE_EQUAL(s->id, s2->id);
	BOOST_REQUIRE_EQUAL(s->variables, s2->variables);

	prx->destroySession(s->id);
}

BOOST_AUTO_TEST_CASE(createAndExpire)
{
	auto prx = this->getProxy<IceSpider::SessionManager>();
	auto s = prx->createSession();
	BOOST_REQUIRE(std::filesystem::exists(root / s->id));
	BOOST_REQUIRE_EQUAL(0, s->duration);
	BOOST_REQUIRE_EQUAL(time(nullptr), s->lastUsed);
	usleep(1001000);
	BOOST_REQUIRE(std::filesystem::exists(root / s->id));
	BOOST_REQUIRE(!prx->getSession(s->id));
	BOOST_REQUIRE(!std::filesystem::exists(root / s->id));
}

BOOST_AUTO_TEST_CASE(missing)
{
	auto prx = this->getProxy<IceSpider::SessionManager>();
	BOOST_REQUIRE(!prx->getSession("missing"));
	BOOST_REQUIRE(!std::filesystem::exists(root / "missing"));
}

BOOST_AUTO_TEST_CASE(createAndLeave)
{
	auto prx = this->getProxy<IceSpider::SessionManager>();
	auto s = prx->createSession();
	BOOST_REQUIRE(std::filesystem::exists(root / s->id));
}

BOOST_AUTO_TEST_CASE(left)
{
	BOOST_REQUIRE(!std::filesystem::is_empty(root));
}

BOOST_AUTO_TEST_CASE(expire)
{
	usleep(1001000);
}

BOOST_AUTO_TEST_SUITE_END();

BOOST_AUTO_TEST_CASE(empty)
{
	TestCore tc;
	BOOST_REQUIRE(std::filesystem::is_empty(tc.root));
}
