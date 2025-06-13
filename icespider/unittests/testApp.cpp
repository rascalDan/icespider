#define BOOST_TEST_MODULE TestApp
#include <boost/test/unit_test.hpp>

#include <Ice/Communicator.h>
#include <Ice/Config.h>
#include <Ice/Initialize.h>
#include <Ice/ObjectAdapter.h>
#include <Ice/Optional.h>
#include <Ice/Properties.h>
#include <Ice/PropertiesF.h>
#include <boost/algorithm/string/predicate.hpp>
#include <core.h>
#include <definedDirs.h>
#include <exception>
#include <exceptions.h>
#include <factory.impl.h>
#include <filesystem>
#include <http.h>
#include <ihttpRequest.h>
#include <irouteHandler.h>
#include <json/serializer.h>
#include <libxml++/document.h>
#include <libxml++/nodes/element.h>
#include <libxml++/parsers/domparser.h>
#include <map>
#include <memory>
#include <set>
#include <slicer/slicer.h>
#include <slicer/xml/serializer.h>
#include <sstream>
#include <string>
#include <string_view>
#include <test-api.h>
#include <testRequest.h>

namespace Ice {
	struct Current;
}

using namespace IceSpider;

struct forceEarlyChangeDir {
	forceEarlyChangeDir()
	{
		std::filesystem::current_path(rootDir);
	}
};

BOOST_TEST_GLOBAL_FIXTURE(forceEarlyChangeDir);

BOOST_AUTO_TEST_CASE(testLoadConfiguration)
{
	BOOST_REQUIRE_EQUAL(13, AdHoc::PluginManager::getDefault()->getAll<IceSpider::RouteHandlerFactory>().size());
}

class CoreWithProps : public CoreWithDefaultRouter {
public:
	CoreWithProps() : CoreWithDefaultRouter({"--Custom.Prop=value"}) { }
};

BOOST_FIXTURE_TEST_SUITE(props, CoreWithProps);

BOOST_AUTO_TEST_CASE(properties)
{
	BOOST_REQUIRE_EQUAL("Test", this->communicator->getProperties()->getProperty("TestIceSpider.TestApi"));
	BOOST_REQUIRE_EQUAL("value", this->communicator->getProperties()->getProperty("Custom.Prop"));
}

BOOST_AUTO_TEST_SUITE_END();

class CoreWithFileProps : public CoreWithDefaultRouter {
public:
	CoreWithFileProps() : CoreWithDefaultRouter({"--IceSpider.Config=config/custom.properties", "--Custom.Prop=value"})
	{
	}
};

BOOST_FIXTURE_TEST_SUITE(fileProps, CoreWithFileProps);

BOOST_AUTO_TEST_CASE(properties)
{
	BOOST_REQUIRE_EQUAL("", this->communicator->getProperties()->getProperty("TestIceSpider.TestApi"));
	BOOST_REQUIRE_EQUAL("something", this->communicator->getProperties()->getProperty("InFile"));
	BOOST_REQUIRE_EQUAL("value", this->communicator->getProperties()->getProperty("Custom.Prop"));
}

BOOST_AUTO_TEST_SUITE_END();

BOOST_FIXTURE_TEST_SUITE(defaultProps, CoreWithDefaultRouter);

BOOST_AUTO_TEST_CASE(testCoreSettings)
{
	BOOST_REQUIRE_EQUAL(5, routes.size());
	BOOST_REQUIRE_EQUAL(1, routes[0].size());
	BOOST_REQUIRE_EQUAL(6, routes[1].size());
	BOOST_REQUIRE_EQUAL(2, routes[2].size());
	BOOST_REQUIRE_EQUAL(2, routes[3].size());
	BOOST_REQUIRE_EQUAL(2, routes[4].size());
}

BOOST_AUTO_TEST_CASE(testFindRoutes)
{
	TestRequest requestGetIndex(this, HttpMethod::GET, "/");
	BOOST_REQUIRE(findRoute(&requestGetIndex));

	TestRequest requestPostIndex(this, HttpMethod::POST, "/");
	BOOST_REQUIRE_THROW(findRoute(&requestPostIndex), IceSpider::Http405MethodNotAllowed);

	TestRequest requestPostUpdate(this, HttpMethod::POST, "/something");
	BOOST_REQUIRE(findRoute(&requestPostUpdate));

	TestRequest requestGetUpdate(this, HttpMethod::GET, "/something");
	BOOST_REQUIRE_THROW(findRoute(&requestGetUpdate), IceSpider::Http405MethodNotAllowed);

	TestRequest requestGetItem(this, HttpMethod::GET, "/view/something/something");
	BOOST_REQUIRE(findRoute(&requestGetItem));

	TestRequest requestGetItemParam(this, HttpMethod::GET, "/item/something/1234");
	BOOST_REQUIRE(findRoute(&requestGetItemParam));

	TestRequest requestGetItemDefault(this, HttpMethod::GET, "/item/something");
	BOOST_REQUIRE(findRoute(&requestGetItemDefault));

	TestRequest requestGetItemLong(
			this, HttpMethod::GET, "/view/something/something/extra/many/things/longer/than/longest/route");
	BOOST_REQUIRE_THROW(findRoute(&requestGetItemLong), IceSpider::Http404NotFound);

	TestRequest requestGetItemShort(this, HttpMethod::GET, "/view/missingSomething");
	BOOST_REQUIRE_THROW(findRoute(&requestGetItemShort), IceSpider::Http404NotFound);

	TestRequest requestGetNothing(this, HttpMethod::GET, "/badview/something/something");
	BOOST_REQUIRE_THROW(findRoute(&requestGetNothing), IceSpider::Http404NotFound);

	TestRequest requestDeleteThing(this, HttpMethod::DELETE, "/something");
	BOOST_REQUIRE(findRoute(&requestDeleteThing));

	TestRequest requestMashS(this, HttpMethod::GET, "/mashS/mash/1/3");
	BOOST_REQUIRE(findRoute(&requestMashS));

	TestRequest requestMashC(this, HttpMethod::GET, "/mashS/mash/1/3");
	BOOST_REQUIRE(findRoute(&requestMashC));
}

BOOST_AUTO_TEST_SUITE_END();

class TestSerice : public TestIceSpider::TestApi {
public:
	TestIceSpider::SomeModelPtr
	index(const Ice::Current &) override
	{
		return std::make_shared<TestIceSpider::SomeModel>("index");
	}

	TestIceSpider::SomeModelPtr
	withParams(const std::string s, Ice::Int i, const Ice::Current &) override
	{
		BOOST_REQUIRE_EQUAL(s, "something");
		BOOST_REQUIRE_EQUAL(i, 1234);
		return std::make_shared<TestIceSpider::SomeModel>("withParams");
	}

	void
	returnNothing(const std::string_view s, const Ice::Current &) override
	{
		if (s == "error") {
			throw TestIceSpider::Ex("test error");
		}
		else if (s.length() == 3) {
			throw TestIceSpider::Ex(std::string(s));
		}
		BOOST_REQUIRE_EQUAL(s, "some value");
	}

	void
	complexParam(const Ice::optional<std::string> s, const TestIceSpider::SomeModelPtr m, const Ice::Current &) override
	{
		BOOST_REQUIRE(s);
		BOOST_REQUIRE_EQUAL("1234", *s);
		BOOST_REQUIRE(m);
		BOOST_REQUIRE_EQUAL("some value", m->value);
	}

	Ice::Int
	simple(const Ice::Current &) override
	{
		return 1;
	}

	std::string
	simplei(Ice::Int n, const Ice::Current &) override
	{
		return std::to_string(n);
	}
};

// NOLINTNEXTLINE(hicpp-special-member-functions)
class TestApp : public CoreWithDefaultRouter {
public:
	TestApp() : adp(communicator->createObjectAdapterWithEndpoints("test", "default"))
	{
		adp->activate();
		adp->add(std::make_shared<TestSerice>(), Ice::stringToIdentity("Test"));
	}

	~TestApp() override
	{
		adp->deactivate();
		adp->destroy();
	}

private:
	Ice::ObjectAdapterPtr adp;
};

class Dummy : public IceSpider::Plugin, TestIceSpider::DummyPlugin {
public:
	Dummy(const Ice::CommunicatorPtr &, const Ice::PropertiesPtr &) { }
};

NAMEDFACTORY("DummyPlugin", Dummy, IceSpider::PluginFactory);

BOOST_FIXTURE_TEST_SUITE(ta, TestApp);

BOOST_AUTO_TEST_CASE(plugins)
{
	auto prx = this->getProxy<TestIceSpider::DummyPlugin>();
	BOOST_REQUIRE(prx);
	prx->ice_ping();
}

BOOST_AUTO_TEST_CASE(testCallIndex)
{
	TestRequest requestGetIndex(this, HttpMethod::GET, "/");
	process(&requestGetIndex);
	auto h = requestGetIndex.getResponseHeaders();
	BOOST_REQUIRE_EQUAL(h["Status"], "200 OK");
	BOOST_REQUIRE_EQUAL(h["Content-Type"], "application/json");
	auto v = Slicer::DeserializeAny<Slicer::JsonStreamDeserializer, TestIceSpider::SomeModelPtr>(
			requestGetIndex.output);
	BOOST_REQUIRE_EQUAL(v->value, "index");
}

BOOST_AUTO_TEST_CASE(testCallMashS)
{
	TestRequest requestGetMashS(this, HttpMethod::GET, "/mashS/something/something/1234");
	process(&requestGetMashS);
	auto h = requestGetMashS.getResponseHeaders();
	BOOST_REQUIRE_EQUAL(h["Status"], "200 OK");
	BOOST_REQUIRE_EQUAL(h["Content-Type"], "application/json");
	auto v = Slicer::DeserializeAny<Slicer::JsonStreamDeserializer, TestIceSpider::Mash1>(requestGetMashS.output);
	BOOST_REQUIRE_EQUAL(v.a->value, "withParams");
	BOOST_REQUIRE_EQUAL(v.b->value, "withParams");
}

BOOST_AUTO_TEST_CASE(testCallMashC)
{
	TestRequest requestGetMashC(this, HttpMethod::GET, "/mashC/something/something/1234");
	process(&requestGetMashC);
	auto h = requestGetMashC.getResponseHeaders();
	BOOST_REQUIRE_EQUAL(h["Status"], "200 OK");
	BOOST_REQUIRE_EQUAL(h["Content-Type"], "application/json");
	auto v = Slicer::DeserializeAny<Slicer::JsonStreamDeserializer, TestIceSpider::Mash2Ptr>(requestGetMashC.output);
	BOOST_REQUIRE_EQUAL(v->a->value, "withParams");
	BOOST_REQUIRE_EQUAL(v->b->value, "withParams");
}

BOOST_AUTO_TEST_CASE(testCallViewSomething1234)
{
	TestRequest requestGetItem(this, HttpMethod::GET, "/view/something/1234");
	process(&requestGetItem);
	auto h = requestGetItem.getResponseHeaders();
	BOOST_REQUIRE_EQUAL(h["Status"], "200 OK");
	BOOST_REQUIRE_EQUAL(h["Content-Type"], "application/json");
	auto v = Slicer::DeserializeAny<Slicer::JsonStreamDeserializer, TestIceSpider::SomeModelPtr>(requestGetItem.output);
	BOOST_REQUIRE_EQUAL(v->value, "withParams");
}

BOOST_AUTO_TEST_CASE(testCallViewSomething1234_)
{
	TestRequest requestGetItemGiven(this, HttpMethod::GET, "/item/something/1234");
	process(&requestGetItemGiven);
	auto h = requestGetItemGiven.getResponseHeaders();
	BOOST_REQUIRE_EQUAL(h["Status"], "200 OK");
	BOOST_REQUIRE_EQUAL(h["Content-Type"], "application/json");
	auto v = Slicer::DeserializeAny<Slicer::JsonStreamDeserializer, TestIceSpider::SomeModelPtr>(
			requestGetItemGiven.output);
	BOOST_REQUIRE_EQUAL(v->value, "withParams");
}

BOOST_AUTO_TEST_CASE(testCallViewSomething)
{
	TestRequest requestGetItemDefault(this, HttpMethod::GET, "/item/something");
	process(&requestGetItemDefault);
	auto h = requestGetItemDefault.getResponseHeaders();
	BOOST_REQUIRE_EQUAL(h["Status"], "200 OK");
	BOOST_REQUIRE_EQUAL(h["Content-Type"], "application/json");
	auto v = Slicer::DeserializeAny<Slicer::JsonStreamDeserializer, TestIceSpider::SomeModelPtr>(
			requestGetItemDefault.output);
	BOOST_REQUIRE_EQUAL(v->value, "withParams");
}

BOOST_AUTO_TEST_CASE(testCallDeleteSomeValue)
{
	TestRequest requestDeleteItem(this, HttpMethod::DELETE, "/some value");
	process(&requestDeleteItem);
	auto h = requestDeleteItem.getResponseHeaders();
	BOOST_REQUIRE_EQUAL(h["Status"], "200 OK");
	requestDeleteItem.output.get();
	BOOST_REQUIRE(requestDeleteItem.output.eof());
}

BOOST_AUTO_TEST_CASE(testCallPost1234)
{
	TestRequest requestUpdateItem(this, HttpMethod::POST, "/1234");
	requestUpdateItem.env["CONTENT_TYPE"] = "application/json";
	requestUpdateItem.input << R"({"value": "some value"})";
	process(&requestUpdateItem);
	auto h = requestUpdateItem.getResponseHeaders();
	BOOST_REQUIRE_EQUAL(h["Status"], "200 OK");
	requestUpdateItem.output.get();
	BOOST_REQUIRE(requestUpdateItem.output.eof());
}

BOOST_AUTO_TEST_CASE(testCallPost1234NoContentType)
{
	TestRequest requestUpdateItem(this, HttpMethod::POST, "/1234");
	requestUpdateItem.input << R"({"value": "some value"})";
	process(&requestUpdateItem);
	auto h = requestUpdateItem.getResponseHeaders();
	BOOST_REQUIRE_EQUAL(h["Status"], "400 Bad Request");
	requestUpdateItem.output.get();
	BOOST_REQUIRE(requestUpdateItem.output.eof());
}

BOOST_AUTO_TEST_CASE(testCallPost1234UnsupportedMediaType)
{
	TestRequest requestUpdateItem(this, HttpMethod::POST, "/1234");
	requestUpdateItem.env["CONTENT_TYPE"] = "application/notathing";
	requestUpdateItem.input << R"(value="some value")";
	process(&requestUpdateItem);
	auto h = requestUpdateItem.getResponseHeaders();
	BOOST_REQUIRE_EQUAL(h["Status"], "415 Unsupported Media Type");
	requestUpdateItem.output.get();
	BOOST_REQUIRE(requestUpdateItem.output.eof());
}

BOOST_AUTO_TEST_CASE(testCallIndexAcceptJson)
{
	TestRequest requestJson(this, HttpMethod::GET, "/");
	requestJson.hdr["Accept"] = "application/json";
	process(&requestJson);
	auto h = requestJson.getResponseHeaders();
	BOOST_REQUIRE_EQUAL(h["Status"], "200 OK");
	BOOST_REQUIRE_EQUAL(h["Content-Type"], "application/json");
	auto v = Slicer::DeserializeAny<Slicer::JsonStreamDeserializer, TestIceSpider::SomeModelPtr>(requestJson.output);
	BOOST_REQUIRE_EQUAL(v->value, "index");
}

BOOST_AUTO_TEST_CASE(testCallIndexAcceptAny)
{
	TestRequest requestAnyAny(this, HttpMethod::GET, "/");
	requestAnyAny.hdr["Accept"] = "*/*";
	process(&requestAnyAny);
	auto h = requestAnyAny.getResponseHeaders();
	BOOST_REQUIRE_EQUAL(h["Status"], "200 OK");
	auto v = Slicer::DeserializeAny<Slicer::JsonStreamDeserializer, TestIceSpider::SomeModelPtr>(requestAnyAny.output);
	BOOST_REQUIRE_EQUAL(v->value, "index");
}

BOOST_AUTO_TEST_CASE(testCallIndexAcceptApplicationAny)
{
	TestRequest requestApplicationAny(this, HttpMethod::GET, "/");
	requestApplicationAny.hdr["Accept"] = "application/*";
	process(&requestApplicationAny);
	auto h = requestApplicationAny.getResponseHeaders();
	BOOST_REQUIRE_EQUAL(h["Status"], "200 OK");
	BOOST_REQUIRE(boost::algorithm::starts_with(h["Content-Type"], "application/"));
	auto v = Slicer::DeserializeAny<Slicer::JsonStreamDeserializer, TestIceSpider::SomeModelPtr>(
			requestApplicationAny.output);
	BOOST_REQUIRE_EQUAL(v->value, "index");
}

BOOST_AUTO_TEST_CASE(testCallIndexAcceptXml)
{
	TestRequest requestXml(this, HttpMethod::GET, "/");
	requestXml.hdr["Accept"] = "application/xml";
	process(&requestXml);
	auto h = requestXml.getResponseHeaders();
	BOOST_REQUIRE_EQUAL(h["Status"], "200 OK");
	BOOST_REQUIRE_EQUAL(h["Content-Type"], "application/xml");
	BOOST_TEST_INFO(requestXml.output.view());
	BOOST_REQUIRE_NE(requestXml.output.view().find("<?xml version"), std::string_view::npos);
	auto v = Slicer::DeserializeAny<Slicer::XmlStreamDeserializer, TestIceSpider::SomeModelPtr>(requestXml.output);
	BOOST_REQUIRE_EQUAL(v->value, "index");
}

BOOST_AUTO_TEST_CASE(testCallIndexAcceptXsltXml)
{
	TestRequest requestXml(this, HttpMethod::GET, "/");
	requestXml.hdr["Accept"] = "application/xml+test";
	process(&requestXml);
	auto h = requestXml.getResponseHeaders();
	BOOST_REQUIRE_EQUAL(h["Status"], "200 OK");
	BOOST_REQUIRE_EQUAL(h["Content-Type"], "application/xml+test");
	BOOST_TEST_INFO(requestXml.output.view());
	BOOST_REQUIRE_NE(requestXml.output.view().find("<?xml version"), std::string_view::npos);
	xmlpp::DomParser d;
	d.parse_stream(requestXml.output);
}

BOOST_AUTO_TEST_CASE(testCallIndexAcceptTextHtml)
{
	TestRequest requestHtml(this, HttpMethod::GET, "/");
	requestHtml.hdr["Accept"] = "text/html";
	process(&requestHtml);
	auto h = requestHtml.getResponseHeaders();
	BOOST_REQUIRE_EQUAL(h["Status"], "200 OK");
	BOOST_REQUIRE_EQUAL(h["Content-Type"], "text/html");
	xmlpp::DomParser d;
	d.parse_stream(requestHtml.output);
	BOOST_REQUIRE_EQUAL(d.get_document()->get_root_node()->get_name(), "html");
}

BOOST_AUTO_TEST_CASE(testCallViewSomethingAcceptHtml)
{
	TestRequest requestHtml(this, HttpMethod::GET, "/view/something/1234");
	requestHtml.hdr["Accept"] = "text/html";
	process(&requestHtml);
	auto h = requestHtml.getResponseHeaders();
	BOOST_REQUIRE_EQUAL(h["Status"], "406 Not Acceptable");
	requestHtml.output.get();
	BOOST_REQUIRE(requestHtml.output.eof());
}

BOOST_AUTO_TEST_CASE(testCallIndexAcceptNotSupported)
{
	TestRequest requestBadAccept(this, HttpMethod::GET, "/");
	requestBadAccept.hdr["Accept"] = "not/supported";
	process(&requestBadAccept);
	auto h = requestBadAccept.getResponseHeaders();
	BOOST_REQUIRE_EQUAL(h["Status"], "406 Not Acceptable");
	requestBadAccept.output.get();
	BOOST_REQUIRE(requestBadAccept.output.eof());
}

BOOST_AUTO_TEST_CASE(testCallIndexComplexAccept)
{
	TestRequest requestChoice(this, HttpMethod::GET, "/");
	requestChoice.hdr["Accept"] = "something/special ; q = 0.9, application/json ; q = 0.8, application/xml;q=1.0";
	process(&requestChoice);
	auto h = requestChoice.getResponseHeaders();
	BOOST_REQUIRE_EQUAL(h["Status"], "200 OK");
	BOOST_REQUIRE_EQUAL(h["Content-Type"], "application/xml");
	auto v = Slicer::DeserializeAny<Slicer::XmlStreamDeserializer, TestIceSpider::SomeModelPtr>(requestChoice.output);
	BOOST_REQUIRE_EQUAL(v->value, "index");
}

BOOST_AUTO_TEST_CASE(testCall404)
{
	TestRequest requestGetIndex(this, HttpMethod::GET, "/this/404");
	process(&requestGetIndex);
	auto h = requestGetIndex.getResponseHeaders();
	BOOST_REQUIRE_EQUAL(h["Status"], "404 Not found");
	requestGetIndex.output.get();
	BOOST_REQUIRE(requestGetIndex.output.eof());
}

BOOST_AUTO_TEST_CASE(testCall405)
{
	TestRequest requestGetIndex(this, HttpMethod::GET, "/405");
	process(&requestGetIndex);
	auto h = requestGetIndex.getResponseHeaders();
	BOOST_REQUIRE_EQUAL(h["Status"], "405 Method Not Allowed");
	requestGetIndex.output.get();
	BOOST_REQUIRE(requestGetIndex.output.eof());
}

BOOST_AUTO_TEST_CASE(testCallSearch)
{
	TestRequest request(this, HttpMethod::GET, "/search");
	request.qs["s"] = "something";
	request.qs["i"] = "1234";
	process(&request);
	auto h = request.getResponseHeaders();
	BOOST_REQUIRE_EQUAL(h["Status"], "200 OK");
	const auto response
			= Slicer::DeserializeAny<Slicer::JsonStreamDeserializer, TestIceSpider::SomeModelPtr>(request.output);
	BOOST_REQUIRE(response);
	BOOST_CHECK_EQUAL(response->value, "withParams");
}

BOOST_AUTO_TEST_CASE(testCallSearchBadLexicalCast)
{
	TestRequest request(this, HttpMethod::GET, "/search");
	request.qs["s"] = "something";
	request.qs["i"] = "bar";
	process(&request);
	auto h = request.getResponseHeaders();
	BOOST_REQUIRE_EQUAL(h["Status"], "400 Bad Request");
	request.output.get();
	BOOST_REQUIRE(request.output.eof());
}

BOOST_AUTO_TEST_CASE(testCallSearchMissingS)
{
	TestRequest request(this, HttpMethod::GET, "/search");
	request.qs["i"] = "1234";
	process(&request);
	auto h = request.getResponseHeaders();
	BOOST_REQUIRE_EQUAL(h["Status"], "400 Bad Request");
	request.output.get();
	BOOST_REQUIRE(request.output.eof());
}

BOOST_AUTO_TEST_CASE(testCallSearchMissingI)
{
	TestRequest request(this, HttpMethod::GET, "/search");
	request.qs["s"] = "something";
	process(&request);
	auto h = request.getResponseHeaders();
	BOOST_REQUIRE_EQUAL(h["Status"], "400 Bad Request");
	request.output.get();
	BOOST_REQUIRE(request.output.eof());
}

BOOST_AUTO_TEST_CASE(testCookies)
{
	TestRequest request(this, HttpMethod::GET, "/cookies");
	request.cookies["mycookievar"] = "something";
	request.qs["i"] = "1234";
	process(&request);
	auto h = request.getResponseHeaders();
	BOOST_REQUIRE_EQUAL(h["Status"], "200 OK");
}

class DummyErrorHandler : public IceSpider::ErrorHandler {
public:
	IceSpider::ErrorHandlerResult
	handleError(IceSpider::IHttpRequest * request, const std::exception & ex) const override
	{
		if (const auto * tex = dynamic_cast<const TestIceSpider::Ex *>(&ex)) {
			if (tex->message == "404") {
				throw IceSpider::Http404NotFound();
			}
			if (tex->message == "304") {
				request->getRequestPath().front() = "some value";
				return IceSpider::ErrorHandlerResult::Modified;
			}
			if (tex->message == "400") {
				request->response(400, "Handled");
				return IceSpider::ErrorHandlerResult::Handled;
			}
		}
		return IceSpider::ErrorHandlerResult::Unhandled;
	}
};

PLUGIN(DummyErrorHandler, IceSpider::ErrorHandler);

BOOST_AUTO_TEST_CASE(testErrorHandler_Unhandled)
{
	TestRequest requestDeleteItem(this, HttpMethod::DELETE, "/error");
	process(&requestDeleteItem);
	auto h = requestDeleteItem.getResponseHeaders();
	BOOST_REQUIRE_EQUAL(h["Status"], "500 TestIceSpider::Ex");
	BOOST_REQUIRE_EQUAL(h["Content-Type"], "text/plain");
	auto & o = requestDeleteItem.output;
	auto b = o.str().substr(static_cast<std::string::size_type>(o.tellg()));
	BOOST_REQUIRE_EQUAL(b, "Exception type: TestIceSpider::Ex\nDetail: test error\n");
}

BOOST_AUTO_TEST_CASE(testErrorHandler_Handled1)
{
	TestRequest requestDeleteItem(this, HttpMethod::DELETE, "/404");
	process(&requestDeleteItem);
	auto h = requestDeleteItem.getResponseHeaders();
	BOOST_REQUIRE_EQUAL(h["Status"], "404 Not found");
	requestDeleteItem.output.get();
	BOOST_REQUIRE(requestDeleteItem.output.eof());
}

BOOST_AUTO_TEST_CASE(testErrorHandler_Handled2)
{
	TestRequest requestDeleteItem(this, HttpMethod::DELETE, "/400");
	process(&requestDeleteItem);
	auto h = requestDeleteItem.getResponseHeaders();
	BOOST_REQUIRE_EQUAL(h["Status"], "400 Handled");
	requestDeleteItem.output.get();
	BOOST_REQUIRE(requestDeleteItem.output.eof());
}

BOOST_AUTO_TEST_CASE(testErrorHandler_Handled3)
{
	TestRequest requestDeleteItem(this, HttpMethod::DELETE, "/304");
	process(&requestDeleteItem);
	auto h = requestDeleteItem.getResponseHeaders();
	BOOST_REQUIRE_EQUAL(h["Status"], "200 OK");
	requestDeleteItem.output.get();
	BOOST_REQUIRE(requestDeleteItem.output.eof());
}

BOOST_AUTO_TEST_SUITE_END();
