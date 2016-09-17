#define BOOST_TEST_MODULE TestApp
#include <boost/test/unit_test.hpp>

#include <safeMapFind.h>
#include <irouteHandler.h>
#include <core.h>
#include <exceptions.h>
#include <test-api.h>
#include <Ice/ObjectAdapter.h>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/filesystem/operations.hpp>
#include <definedDirs.h>
#include <slicer/slicer.h>
#include <xml/serializer.h>
#include <json/serializer.h>
#include <libxml++/parsers/domparser.h>

using namespace IceSpider;

static void forceEarlyChangeDir() __attribute__((constructor(101)));
void forceEarlyChangeDir()
{
	boost::filesystem::current_path(XSTR(ROOT));
}

BOOST_AUTO_TEST_CASE( testLoadConfiguration )
{
	BOOST_REQUIRE_EQUAL(10, AdHoc::PluginManager::getDefault()->getAll<IceSpider::RouteHandlerFactory>().size());
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
			return qs.find(key) == qs.end() ? IceUtil::Optional<std::string>() : qs.find(key)->second;
		}

		IceUtil::Optional<std::string> getHeaderParam(const std::string & key) const override
		{
			return hdr.find(key) == hdr.end() ? IceUtil::Optional<std::string>() : hdr.find(key)->second;
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

BOOST_FIXTURE_TEST_SUITE(c, Core);

BOOST_AUTO_TEST_CASE( testCoreSettings )
{
	BOOST_REQUIRE_EQUAL(5, routes.size());
	BOOST_REQUIRE_EQUAL(1, routes[0].size());
	BOOST_REQUIRE_EQUAL(4, routes[1].size());
	BOOST_REQUIRE_EQUAL(1, routes[2].size());
	BOOST_REQUIRE_EQUAL(2, routes[3].size());
	BOOST_REQUIRE_EQUAL(2, routes[4].size());
}

BOOST_AUTO_TEST_CASE( testFindRoutes )
{
	TestRequest requestGetIndex(this, HttpMethod::GET, "/");
	BOOST_REQUIRE(findRoute(&requestGetIndex));

	TestRequest requestPostIndex(this, HttpMethod::POST, "/");
	BOOST_REQUIRE_THROW(findRoute(&requestPostIndex), IceSpider::Http405_MethodNotAllowed);

	TestRequest requestPostUpdate(this, HttpMethod::POST, "/something");
	BOOST_REQUIRE(findRoute(&requestPostUpdate));

	TestRequest requestGetUpdate(this, HttpMethod::GET, "/something");
	BOOST_REQUIRE_THROW(findRoute(&requestGetUpdate), IceSpider::Http405_MethodNotAllowed);

	TestRequest requestGetItem(this, HttpMethod::GET, "/view/something/something");
	BOOST_REQUIRE(findRoute(&requestGetItem));

	TestRequest requestGetItemParam(this, HttpMethod::GET, "/item/something/1234");
	BOOST_REQUIRE(findRoute(&requestGetItemParam));

	TestRequest requestGetItemDefault(this, HttpMethod::GET, "/item/something");
	BOOST_REQUIRE(findRoute(&requestGetItemDefault));

	TestRequest requestGetItemLong(this, HttpMethod::GET, "/view/something/something/extra/many/things/longer/than/longest/route");
	BOOST_REQUIRE_THROW(findRoute(&requestGetItemLong), IceSpider::Http404_NotFound);

	TestRequest requestGetItemShort(this, HttpMethod::GET, "/view/missingSomething");
	BOOST_REQUIRE_THROW(findRoute(&requestGetItemShort), IceSpider::Http404_NotFound);

	TestRequest requestGetNothing(this, HttpMethod::GET, "/badview/something/something");
	BOOST_REQUIRE_THROW(findRoute(&requestGetNothing), IceSpider::Http404_NotFound);

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

class TestApp : public Core {
	public:
		TestApp() :
			adp(communicator->createObjectAdapterWithEndpoints("test", "default"))
		{
			adp->activate();
			communicator->proxyToString(adp->add(new TestSerice(), communicator->stringToIdentity("Test")));
		}

		~TestApp()
		{
			adp->deactivate();
		}

		Ice::ObjectAdapterPtr adp;
};

typedef std::map<std::string, std::string> Headers;
Headers
parseHeaders(std::istream & strm)
{
	Headers h;
	while (true) {
		char buf[BUFSIZ], n[BUFSIZ], v[BUFSIZ];
		strm.getline(buf, BUFSIZ);
		if (sscanf(buf, "%[^:]: %[^\r]", n, v) != 2) {
			break;
		}
		h[n] = v;
	}
	return h;
}

BOOST_FIXTURE_TEST_SUITE(ta, TestApp);

BOOST_AUTO_TEST_CASE( testCallIndex )
{
	TestRequest requestGetIndex(this, HttpMethod::GET, "/");
	process(&requestGetIndex);
	auto h = parseHeaders(requestGetIndex.output);
	BOOST_REQUIRE_EQUAL(h["Status"], "200 OK");
	BOOST_REQUIRE_EQUAL(h["Content-Type"], "application/json");
	auto v = Slicer::DeserializeAny<Slicer::JsonStreamDeserializer, TestIceSpider::SomeModelPtr>(requestGetIndex.output);
	BOOST_REQUIRE_EQUAL(v->value, "index");
}

BOOST_AUTO_TEST_CASE( testCallMashS )
{
	TestRequest requestGetMashS(this, HttpMethod::GET, "/mashS/something/something/1234");
	process(&requestGetMashS);
	auto h = parseHeaders(requestGetMashS.output);
	BOOST_REQUIRE_EQUAL(h["Status"], "200 OK");
	BOOST_REQUIRE_EQUAL(h["Content-Type"], "application/json");
	auto v = Slicer::DeserializeAny<Slicer::JsonStreamDeserializer, TestIceSpider::Mash1>(requestGetMashS.output);
	BOOST_REQUIRE_EQUAL(v.a->value, "withParams");
	BOOST_REQUIRE_EQUAL(v.b->value, "withParams");
}

BOOST_AUTO_TEST_CASE( testCallMashC )
{
	TestRequest requestGetMashC(this, HttpMethod::GET, "/mashC/something/something/1234");
	process(&requestGetMashC);
	auto h = parseHeaders(requestGetMashC.output);
	BOOST_REQUIRE_EQUAL(h["Status"], "200 OK");
	BOOST_REQUIRE_EQUAL(h["Content-Type"], "application/json");
	auto v = Slicer::DeserializeAny<Slicer::JsonStreamDeserializer, TestIceSpider::Mash2Ptr>(requestGetMashC.output);
	BOOST_REQUIRE_EQUAL(v->a->value, "withParams");
	BOOST_REQUIRE_EQUAL(v->b->value, "withParams");
}

BOOST_AUTO_TEST_CASE( testCallViewSomething1234 )
{
	TestRequest requestGetItem(this, HttpMethod::GET, "/view/something/1234");
	process(&requestGetItem);
	auto h = parseHeaders(requestGetItem.output);
	BOOST_REQUIRE_EQUAL(h["Status"], "200 OK");
	BOOST_REQUIRE_EQUAL(h["Content-Type"], "application/json");
	auto v = Slicer::DeserializeAny<Slicer::JsonStreamDeserializer, TestIceSpider::SomeModelPtr>(requestGetItem.output);
	BOOST_REQUIRE_EQUAL(v->value, "withParams");
}

BOOST_AUTO_TEST_CASE( testCallViewSomething1234_ )
{
	TestRequest requestGetItemGiven(this, HttpMethod::GET, "/item/something/1234");
	process(&requestGetItemGiven);
	auto h = parseHeaders(requestGetItemGiven.output);
	BOOST_REQUIRE_EQUAL(h["Status"], "200 OK");
	BOOST_REQUIRE_EQUAL(h["Content-Type"], "application/json");
	auto v = Slicer::DeserializeAny<Slicer::JsonStreamDeserializer, TestIceSpider::SomeModelPtr>(requestGetItemGiven.output);
	BOOST_REQUIRE_EQUAL(v->value, "withParams");
}

BOOST_AUTO_TEST_CASE( testCallViewSomething )
{
	TestRequest requestGetItemDefault(this, HttpMethod::GET, "/item/something");
	process(&requestGetItemDefault);
	auto h = parseHeaders(requestGetItemDefault.output);
	BOOST_REQUIRE_EQUAL(h["Status"], "200 OK");
	BOOST_REQUIRE_EQUAL(h["Content-Type"], "application/json");
	auto v = Slicer::DeserializeAny<Slicer::JsonStreamDeserializer, TestIceSpider::SomeModelPtr>(requestGetItemDefault.output);
	BOOST_REQUIRE_EQUAL(v->value, "withParams");
}

BOOST_AUTO_TEST_CASE( testCallDeleteSomeValue )
{
	TestRequest requestDeleteItem(this, HttpMethod::DELETE, "/some value");
	process(&requestDeleteItem);
	auto h = parseHeaders(requestDeleteItem.output);
	BOOST_REQUIRE_EQUAL(h["Status"], "200 OK");
	requestDeleteItem.output.get();
	BOOST_REQUIRE(requestDeleteItem.output.eof());
}

BOOST_AUTO_TEST_CASE( testCallPost1234 )
{
	TestRequest requestUpdateItem(this, HttpMethod::POST, "/1234");
	requestUpdateItem.hdr["Content-Type"] = "application/json";
	requestUpdateItem.input << "{\"value\": \"some value\"}";
	process(&requestUpdateItem);
	auto h = parseHeaders(requestUpdateItem.output);
	BOOST_REQUIRE_EQUAL(h["Status"], "200 OK");
	requestUpdateItem.output.get();
	BOOST_REQUIRE(requestUpdateItem.output.eof());
}

BOOST_AUTO_TEST_CASE( testCallPost1234NoContentType )
{
	TestRequest requestUpdateItem(this, HttpMethod::POST, "/1234");
	requestUpdateItem.input << "{\"value\": \"some value\"}";
	process(&requestUpdateItem);
	auto h = parseHeaders(requestUpdateItem.output);
	BOOST_REQUIRE_EQUAL(h["Status"], "400 Bad Request");
	requestUpdateItem.output.get();
	BOOST_REQUIRE(requestUpdateItem.output.eof());
}

BOOST_AUTO_TEST_CASE( testCallPost1234UnsupportedMediaType )
{
	TestRequest requestUpdateItem(this, HttpMethod::POST, "/1234");
	requestUpdateItem.hdr["Content-Type"] = "application/notathing";
	requestUpdateItem.input << "value=\"some value\"";
	process(&requestUpdateItem);
	auto h = parseHeaders(requestUpdateItem.output);
	BOOST_REQUIRE_EQUAL(h["Status"], "415 Unsupported Media Type");
	requestUpdateItem.output.get();
	BOOST_REQUIRE(requestUpdateItem.output.eof());
}

BOOST_AUTO_TEST_CASE( testCallIndexAcceptJson )
{
	TestRequest requestJson(this, HttpMethod::GET, "/");
	requestJson.hdr["Accept"] = "application/json";
	process(&requestJson);
	auto h = parseHeaders(requestJson.output);
	BOOST_REQUIRE_EQUAL(h["Status"], "200 OK");
	BOOST_REQUIRE_EQUAL(h["Content-Type"], "application/json");
	auto v = Slicer::DeserializeAny<Slicer::JsonStreamDeserializer, TestIceSpider::SomeModelPtr>(requestJson.output);
	BOOST_REQUIRE_EQUAL(v->value, "index");
}

BOOST_AUTO_TEST_CASE( testCallIndexAcceptAny )
{
	TestRequest requestAnyAny(this, HttpMethod::GET, "/");
	requestAnyAny.hdr["Accept"] = "*/*";
	process(&requestAnyAny);
	auto h = parseHeaders(requestAnyAny.output);
	BOOST_REQUIRE_EQUAL(h["Status"], "200 OK");
	auto v = Slicer::DeserializeAny<Slicer::JsonStreamDeserializer, TestIceSpider::SomeModelPtr>(requestAnyAny.output);
	BOOST_REQUIRE_EQUAL(v->value, "index");
}

BOOST_AUTO_TEST_CASE( testCallIndexAcceptApplicationAny )
{
	TestRequest requestApplicationAny(this, HttpMethod::GET, "/");
	requestApplicationAny.hdr["Accept"] = "application/*";
	process(&requestApplicationAny);
	auto h = parseHeaders(requestApplicationAny.output);
	BOOST_REQUIRE_EQUAL(h["Status"], "200 OK");
	BOOST_REQUIRE(boost::algorithm::starts_with(h["Content-Type"], "application/"));
	auto v = Slicer::DeserializeAny<Slicer::JsonStreamDeserializer, TestIceSpider::SomeModelPtr>(requestApplicationAny.output);
	BOOST_REQUIRE_EQUAL(v->value, "index");
}

BOOST_AUTO_TEST_CASE( testCallIndexAcceptXml )
{
	TestRequest requestXml(this, HttpMethod::GET, "/");
	requestXml.hdr["Accept"] = "application/xml";
	process(&requestXml);
	auto h = parseHeaders(requestXml.output);
	BOOST_REQUIRE_EQUAL(h["Status"], "200 OK");
	BOOST_REQUIRE_EQUAL(h["Content-Type"], "application/xml");
	auto v = Slicer::DeserializeAny<Slicer::XmlStreamDeserializer, TestIceSpider::SomeModelPtr>(requestXml.output);
	BOOST_REQUIRE_EQUAL(v->value, "index");
}

BOOST_AUTO_TEST_CASE( testCallIndexAcceptTextHtml )
{
	TestRequest requestHtml(this, HttpMethod::GET, "/");
	requestHtml.hdr["Accept"] = "text/html";
	process(&requestHtml);
	auto h = parseHeaders(requestHtml.output);
	BOOST_REQUIRE_EQUAL(h["Status"], "200 OK");
	BOOST_REQUIRE_EQUAL(h["Content-Type"], "text/html");
	xmlpp::DomParser d;
	d.parse_stream(requestHtml.output);
	BOOST_REQUIRE_EQUAL(d.get_document()->get_root_node()->get_name(), "html");
}

BOOST_AUTO_TEST_CASE( testCallViewSomethingAcceptHtml )
{
	TestRequest requestHtml(this, HttpMethod::GET, "/view/something/1234");
	requestHtml.hdr["Accept"] = "text/html";
	process(&requestHtml);
	auto h = parseHeaders(requestHtml.output);
	BOOST_REQUIRE_EQUAL(h["Status"], "406 Not Acceptable");
	requestHtml.output.get();
	BOOST_REQUIRE(requestHtml.output.eof());
}

BOOST_AUTO_TEST_CASE( testCallIndexAcceptNotSupported )
{
	TestRequest requestBadAccept(this, HttpMethod::GET, "/");
	requestBadAccept.hdr["Accept"] = "not/supported";
	process(&requestBadAccept);
	auto h = parseHeaders(requestBadAccept.output);
	BOOST_REQUIRE_EQUAL(h["Status"], "406 Not Acceptable");
	requestBadAccept.output.get();
	BOOST_REQUIRE(requestBadAccept.output.eof());
}

BOOST_AUTO_TEST_CASE( testCallIndexComplexAccept )
{
	TestRequest requestChoice(this, HttpMethod::GET, "/");
	requestChoice.hdr["Accept"] = "something/special ; q = 20, application/json, application/xml;q=1.1";
	process(&requestChoice);
	auto h = parseHeaders(requestChoice.output);
	BOOST_REQUIRE_EQUAL(h["Status"], "200 OK");
	BOOST_REQUIRE_EQUAL(h["Content-Type"], "application/xml");
	auto v = Slicer::DeserializeAny<Slicer::XmlStreamDeserializer, TestIceSpider::SomeModelPtr>(requestChoice.output);
	BOOST_REQUIRE_EQUAL(v->value, "index");
}

BOOST_AUTO_TEST_CASE( testCall404 )
{
	TestRequest requestGetIndex(this, HttpMethod::GET, "/this/404");
	process(&requestGetIndex);
	auto h = parseHeaders(requestGetIndex.output);
	BOOST_REQUIRE_EQUAL(h["Status"], "404 Not found");
	requestGetIndex.output.get();
	BOOST_REQUIRE(requestGetIndex.output.eof());
}

BOOST_AUTO_TEST_CASE( testCall405 )
{
	TestRequest requestGetIndex(this, HttpMethod::GET, "/405");
	process(&requestGetIndex);
	auto h = parseHeaders(requestGetIndex.output);
	BOOST_REQUIRE_EQUAL(h["Status"], "405 Method Not Allowed");
	requestGetIndex.output.get();
	BOOST_REQUIRE(requestGetIndex.output.eof());
}

BOOST_AUTO_TEST_CASE( testCallSearch )
{
	TestRequest request(this, HttpMethod::GET, "/search");
	request.qs["s"] = "something";
	request.qs["i"] = "1234";
	process(&request);
	auto h = parseHeaders(request.output);
	BOOST_REQUIRE_EQUAL(h["Status"], "200 OK");
	Slicer::DeserializeAny<Slicer::JsonStreamDeserializer, TestIceSpider::SomeModelPtr>(request.output);
}

BOOST_AUTO_TEST_CASE( testCallSearchBadLexicalCast )
{
	TestRequest request(this, HttpMethod::GET, "/search");
	request.qs["s"] = "something";
	request.qs["i"] = "bar";
	process(&request);
	auto h = parseHeaders(request.output);
	BOOST_REQUIRE_EQUAL(h["Status"], "400 Bad Request");
	request.output.get();
	BOOST_REQUIRE(request.output.eof());
}

BOOST_AUTO_TEST_CASE( testCallSearchMissingS )
{
	TestRequest request(this, HttpMethod::GET, "/search");
	request.qs["i"] = "1234";
	process(&request);
	auto h = parseHeaders(request.output);
	BOOST_REQUIRE_EQUAL(h["Status"], "400 Bad Request");
	request.output.get();
	BOOST_REQUIRE(request.output.eof());
}

BOOST_AUTO_TEST_CASE( testCallSearchMissingI )
{
	TestRequest request(this, HttpMethod::GET, "/search");
	request.qs["s"] = "something";
	process(&request);
	auto h = parseHeaders(request.output);
	BOOST_REQUIRE_EQUAL(h["Status"], "400 Bad Request");
	request.output.get();
	BOOST_REQUIRE(request.output.eof());
}

BOOST_AUTO_TEST_SUITE_END();

