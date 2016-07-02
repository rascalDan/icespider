#define BOOST_TEST_MODULE TestApp
#include <boost/test/unit_test.hpp>

#include <plugins.h>
#include <safeMapFind.h>
#include <irouteHandler.h>
#include <core.h>
#include <test-api.h>
#include <Ice/ObjectAdapter.h>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>

using namespace IceSpider;

BOOST_AUTO_TEST_CASE( testLoadConfiguration )
{
	BOOST_REQUIRE_EQUAL(6, AdHoc::PluginManager::getDefault()->getAll<IRouteHandler>().size());
}

BOOST_FIXTURE_TEST_SUITE(c, Core);

BOOST_AUTO_TEST_CASE( testCoreSettings )
{
	BOOST_REQUIRE_EQUAL(6, routes.size());
	BOOST_REQUIRE_EQUAL(4, routes[HttpMethod::GET].size());
	BOOST_REQUIRE_EQUAL(1, routes[HttpMethod::GET][0].size());
	BOOST_REQUIRE_EQUAL(0, routes[HttpMethod::GET][1].size());
	BOOST_REQUIRE_EQUAL(1, routes[HttpMethod::GET][2].size());
	BOOST_REQUIRE_EQUAL(2, routes[HttpMethod::GET][3].size());
	BOOST_REQUIRE_EQUAL(1, routes[HttpMethod::HEAD].size());
	BOOST_REQUIRE_EQUAL(2, routes[HttpMethod::POST].size());
	BOOST_REQUIRE_EQUAL(0, routes[HttpMethod::POST][0].size());
	BOOST_REQUIRE_EQUAL(1, routes[HttpMethod::POST][1].size());
	BOOST_REQUIRE_EQUAL(1, routes[HttpMethod::PUT].size());
	BOOST_REQUIRE_EQUAL(2, routes[HttpMethod::DELETE].size());
	BOOST_REQUIRE_EQUAL(0, routes[HttpMethod::DELETE][0].size());
	BOOST_REQUIRE_EQUAL(1, routes[HttpMethod::DELETE][1].size());
	BOOST_REQUIRE_EQUAL(1, routes[HttpMethod::OPTIONS].size());
}

class TestRequest : public IHttpRequest {
	public:
		TestRequest(const Core * c, HttpMethod m, const std::string & p) :
			IHttpRequest(c),
			method(m)
		{
			namespace ba = boost::algorithm;
			auto path = p.substr(1);
			if (!path.empty()) {
				ba::split(url, path, ba::is_any_of("/"), ba::token_compress_off);
			}
		}

		const std::vector<std::string> & getRequestPath() const override
		{
			return url;
		}

		HttpMethod getRequestMethod() const override
		{
			return method;
		}

		IceUtil::Optional<std::string> getQueryStringParam(const std::string & key) const override
		{
			return AdHoc::safeMapLookup<std::runtime_error>(qs, key);
		}

		IceUtil::Optional<std::string> getHeaderParam(const std::string & key) const override
		{
			return AdHoc::safeMapLookup<std::runtime_error>(hdr, key);
		}

		std::istream & getInputStream() const override
		{
			return input;
		}

		std::ostream & getOutputStream() const override
		{
			return output;
		}

		typedef std::map<std::string, std::string> MapVars;
		typedef std::vector<std::string> UrlVars;
		UrlVars url;
		MapVars qs;
		MapVars hdr;
		mutable std::stringstream input;
		mutable std::stringstream output;

		const HttpMethod method;
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

	TestRequest requestGetItemParam(this, HttpMethod::GET, "/item/something/1234");
	BOOST_REQUIRE(findRoute(&requestGetItemParam));

	TestRequest requestGetItemDefault(this, HttpMethod::GET, "/item/something");
	BOOST_REQUIRE(findRoute(&requestGetItemDefault));

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
			return new TestIceSpider::SomeModel("index");
		}

		TestIceSpider::SomeModelPtr withParams(const std::string & s, Ice::Int i, const Ice::Current &) override
		{
			BOOST_REQUIRE_EQUAL(s, "something");
			BOOST_REQUIRE_EQUAL(i, 1234);
			return new TestIceSpider::SomeModel("withParams");
		}

		void returnNothing(const std::string & s, const Ice::Current &) override
		{
			BOOST_REQUIRE_EQUAL(s, "some value");
		}

		void complexParam(const IceUtil::Optional<std::string> & s, const TestIceSpider::SomeModelPtr & m, const Ice::Current &) override
		{
			BOOST_REQUIRE(s);
			BOOST_REQUIRE_EQUAL("1234", *s);
			BOOST_REQUIRE(m);
			BOOST_REQUIRE_EQUAL("some value", m->value);
		}
};

BOOST_AUTO_TEST_CASE( testCallMethods )
{
	auto adp = communicator->createObjectAdapterWithEndpoints("test", "default");
	auto obj = adp->addWithUUID(new TestSerice());
	adp->activate();
	fprintf(stderr, "%s\n", obj->ice_id().c_str());
	communicator->getProperties()->setProperty("TestIceSpider::TestApi", communicator->proxyToString(obj));

	TestRequest requestGetIndex(this, HttpMethod::GET, "/");
	process(&requestGetIndex);
	BOOST_REQUIRE_EQUAL(requestGetIndex.output.str(), "Status: 200 OK\r\n\r\n{\"value\":\"index\"}");

	TestRequest requestGetItem(this, HttpMethod::GET, "/view/something/1234");
	process(&requestGetItem);
	BOOST_REQUIRE_EQUAL(requestGetItem.output.str(), "Status: 200 OK\r\n\r\n{\"value\":\"withParams\"}");

	TestRequest requestGetItemGiven(this, HttpMethod::GET, "/item/something/1234");
	process(&requestGetItemGiven);
	BOOST_REQUIRE_EQUAL(requestGetItemGiven.output.str(), "Status: 200 OK\r\n\r\n{\"value\":\"withParams\"}");

	TestRequest requestGetItemDefault(this, HttpMethod::GET, "/item/something");
	process(&requestGetItemDefault);
	BOOST_REQUIRE_EQUAL(requestGetItemDefault.output.str(), "Status: 200 OK\r\n\r\n{\"value\":\"withParams\"}");

	TestRequest requestDeleteItem(this, HttpMethod::DELETE, "/some value");
	process(&requestDeleteItem);
	BOOST_REQUIRE_EQUAL(requestDeleteItem.output.str(), "Status: 200 OK\r\n\r\n");

	TestRequest requestUpdateItem(this, HttpMethod::POST, "/1234");
	requestUpdateItem.hdr["Content-Type"] = "application/json";
	requestUpdateItem.input << "{\"value\": \"some value\"}";
	process(&requestUpdateItem);
	BOOST_REQUIRE_EQUAL(requestDeleteItem.output.str(), "Status: 200 OK\r\n\r\n");

	adp->deactivate();
}

BOOST_AUTO_TEST_CASE( test404 )
{
	TestRequest requestGetIndex(this, HttpMethod::GET, "/404");
	process(&requestGetIndex);
	BOOST_REQUIRE_EQUAL(requestGetIndex.output.str(), "Status: 404 Not found\r\n\r\n");
}

BOOST_AUTO_TEST_SUITE_END();

