#define BOOST_TEST_MODULE TestApp
#include <boost/test/unit_test.hpp>

#include <plugins.h>
#include <irouteHandler.h>
#include <core.h>
#include <test-api.h>
#include <Ice/ObjectAdapter.h>

using namespace UserIceSpider;

BOOST_AUTO_TEST_CASE( testLoadConfiguration )
{
	BOOST_REQUIRE_EQUAL(4, AdHoc::PluginManager::getDefault()->getAll<IceSpider::IRouteHandler>().size());
}

BOOST_FIXTURE_TEST_SUITE(c, IceSpider::Core);

BOOST_AUTO_TEST_CASE( testCoreSettings )
{
	BOOST_REQUIRE_EQUAL(6, routes.size());
	BOOST_REQUIRE_EQUAL(4, routes[HttpMethod::GET].size());
	BOOST_REQUIRE_EQUAL(1, routes[HttpMethod::GET][0].size());
	BOOST_REQUIRE_EQUAL(0, routes[HttpMethod::GET][1].size());
	BOOST_REQUIRE_EQUAL(0, routes[HttpMethod::GET][2].size());
	BOOST_REQUIRE_EQUAL(1, routes[HttpMethod::GET][3].size());
	BOOST_REQUIRE_EQUAL(0, routes[HttpMethod::HEAD].size());
	BOOST_REQUIRE_EQUAL(2, routes[HttpMethod::POST].size());
	BOOST_REQUIRE_EQUAL(0, routes[HttpMethod::POST][0].size());
	BOOST_REQUIRE_EQUAL(1, routes[HttpMethod::POST][1].size());
	BOOST_REQUIRE_EQUAL(0, routes[HttpMethod::PUT].size());
	BOOST_REQUIRE_EQUAL(2, routes[HttpMethod::DELETE].size());
	BOOST_REQUIRE_EQUAL(0, routes[HttpMethod::DELETE][0].size());
	BOOST_REQUIRE_EQUAL(1, routes[HttpMethod::DELETE][1].size());
	BOOST_REQUIRE_EQUAL(0, routes[HttpMethod::OPTIONS].size());
}

class TestRequest : public IceSpider::IHttpRequest {
	public:
		TestRequest(const IceSpider::Core * c, HttpMethod m, const std::string & p) :
			IHttpRequest(c),
			method(m),
			path(p)
		{
		}
			
		std::string getRequestPath() const override
		{
			return path;
		}

		HttpMethod getRequestMethod() const override
		{
			return method;
		}

		const HttpMethod method;
		const std::string path;
};

BOOST_AUTO_TEST_CASE( testFindRoutes )
{
	TestRequest requestGetIndex(this, HttpMethod::GET, "/");
	BOOST_REQUIRE(findRoute(&requestGetIndex));

	TestRequest requestPostIndex(this, HttpMethod::POST, "/");
	BOOST_REQUIRE(!findRoute(&requestPostIndex));

	TestRequest requestPostUpdate(this, HttpMethod::POST, "/something");
	BOOST_REQUIRE(findRoute(&requestPostUpdate));

	TestRequest requestGetUpdate(this, HttpMethod::GET, "/something");
	BOOST_REQUIRE(!findRoute(&requestGetUpdate));

	TestRequest requestGetItem(this, HttpMethod::GET, "/view/something/something");
	BOOST_REQUIRE(findRoute(&requestGetItem));

	TestRequest requestGetItemLong(this, HttpMethod::GET, "/view/something/something/extra");
	BOOST_REQUIRE(!findRoute(&requestGetItemLong));

	TestRequest requestGetItemShort(this, HttpMethod::GET, "/view/missingSomething");
	BOOST_REQUIRE(!findRoute(&requestGetItemShort));

	TestRequest requestGetNothing(this, HttpMethod::GET, "/badview/something/something");
	BOOST_REQUIRE(!findRoute(&requestGetNothing));

	TestRequest requestDeleteThing(this, HttpMethod::DELETE, "/something");
	BOOST_REQUIRE(findRoute(&requestDeleteThing));
}

class TestSerice : public TestIceSpider::TestApi {
	public:
		TestIceSpider::SomeModelPtr index(const Ice::Current &) override
		{
			return NULL;
		}

		TestIceSpider::SomeModelPtr withParams(const std::string &, Ice::Int, const Ice::Current &) override
		{
			return NULL;
		}

		void returnNothing(const std::string &, const Ice::Current &) override
		{
		}

		void complexParam(const std::string &, const TestIceSpider::SomeModelPtr &, const Ice::Current &) override
		{
		}
};

BOOST_AUTO_TEST_CASE( testGetIndex )
{
	auto adp = communicator->createObjectAdapterWithEndpoints("test", "default");
	auto obj = adp->addWithUUID(new TestSerice());
	adp->activate();
	TestRequest requestGetIndex(this, HttpMethod::GET, "/");
	fprintf(stderr, "%s\n", obj->ice_id().c_str());
	communicator->getProperties()->setProperty("N13TestIceSpider7TestApiE", communicator->proxyToString(obj));
	process(&requestGetIndex);
	adp->deactivate();
}

BOOST_AUTO_TEST_SUITE_END();
