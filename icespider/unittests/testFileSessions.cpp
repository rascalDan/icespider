#define BOOST_TEST_MODULE TestFileSessions
#include <boost/test/unit_test.hpp>

#include <Ice/Initialize.h>
#include <Ice/Properties.h>
#include <session.h>
#include <core.h>
#include <definedDirs.h>

BOOST_TEST_DONT_PRINT_LOG_VALUE(IceSpider::StringMap);

class TestCore : public IceSpider::Core {
	public:
		TestCore() :
			IceSpider::Core({
				"--IceSpider.SessionManager=IceSpider-FileSessions",
				"--IceSpider.FileSessions.Path=" + (binDir / "test-sessions").string(),
				"--IceSpider.FileSessions.Duration=0"
			}),
			root(communicator->getProperties()->getProperty("IceSpider.FileSessions.Path"))
		{
		}
		const boost::filesystem::path root;
};

BOOST_AUTO_TEST_CASE( clear )
{
	TestCore tc;
	if (boost::filesystem::exists(tc.root)) {
		boost::filesystem::remove_all(tc.root);
	}
}

BOOST_FIXTURE_TEST_SUITE(Core, TestCore);

BOOST_AUTO_TEST_CASE( ping )
{
	auto prx = this->getProxy<IceSpider::SessionManager>();
	BOOST_REQUIRE(prx);
	prx->ice_ping();
}

BOOST_AUTO_TEST_CASE( createAndDestroy )
{
	auto prx = this->getProxy<IceSpider::SessionManager>();
	auto s = prx->createSession();
	BOOST_REQUIRE(boost::filesystem::exists(root / s->id));
	BOOST_REQUIRE_EQUAL(0, s->duration);
	BOOST_REQUIRE_EQUAL(time(NULL), s->lastUsed);
	prx->destroySession(s->id);
	BOOST_REQUIRE(!boost::filesystem::exists(root / s->id));
}

BOOST_AUTO_TEST_CASE( createAndChangeRestore )
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

BOOST_AUTO_TEST_CASE( createAndExpire )
{
	auto prx = this->getProxy<IceSpider::SessionManager>();
	auto s = prx->createSession();
	BOOST_REQUIRE(boost::filesystem::exists(root / s->id));
	BOOST_REQUIRE_EQUAL(0, s->duration);
	BOOST_REQUIRE_EQUAL(time(NULL), s->lastUsed);
	usleep(1001000);
	BOOST_REQUIRE(boost::filesystem::exists(root / s->id));
	BOOST_REQUIRE(!prx->getSession(s->id));
	BOOST_REQUIRE(!boost::filesystem::exists(root / s->id));
}

BOOST_AUTO_TEST_CASE( missing )
{
	auto prx = this->getProxy<IceSpider::SessionManager>();
	BOOST_REQUIRE(!prx->getSession("missing"));
	BOOST_REQUIRE(!boost::filesystem::exists(root / "missing"));
}

BOOST_AUTO_TEST_CASE( createAndLeave )
{
	auto prx = this->getProxy<IceSpider::SessionManager>();
	auto s = prx->createSession();
	BOOST_REQUIRE(boost::filesystem::exists(root / s->id));
}

BOOST_AUTO_TEST_CASE( left )
{
	BOOST_REQUIRE(!boost::filesystem::is_empty(root));
}

BOOST_AUTO_TEST_CASE( expire )
{
	usleep(1001000);
}

BOOST_AUTO_TEST_SUITE_END();

BOOST_AUTO_TEST_CASE( empty )
{
	TestCore tc;
	BOOST_REQUIRE(boost::filesystem::is_empty(tc.root));
}

