#define BOOST_TEST_MODULE TestCompile
#include <boost/test/unit_test.hpp>

#include "../compile/routeCompiler.h"
#include "../core/irouteHandler.h"
#include <boost/algorithm/string/join.hpp>
#include <definedDirs.h>
#include <dlfcn.h>
#include <plugins.h>
#include <slicer/modelPartsTypes.h>

using namespace IceSpider;

static void forceEarlyChangeDir() __attribute__((constructor(101)));
void
forceEarlyChangeDir()
{
	std::filesystem::current_path(XSTR(ROOT));
}

class CoreFixture {
protected:
	CoreFixture() : modeDir(binDir.lexically_relative(rootDir / "bin" / "testCompile.test")) { }
	// NOLINTNEXTLINE(misc-non-private-member-variables-in-classes)
	const std::filesystem::path modeDir;
};

namespace std {
	ostream &
	operator<<(ostream & s, const IceSpider::HttpMethod & m)
	{
		s << Slicer::ModelPartForEnum<IceSpider::HttpMethod>::lookup(m);
		return s;
	}
}

BOOST_FIXTURE_TEST_SUITE(cf, CoreFixture)

BOOST_AUTO_TEST_CASE(testLoadConfiguration)
{
	Compile::RouteCompiler rc;
	rc.searchPath.push_back(rootDir);
	auto cfg = rc.loadConfiguration(rootDir / "testRoutes.json");
	BOOST_REQUIRE(cfg);
	auto units = rc.loadUnits(cfg);
	BOOST_REQUIRE(!units.empty());
	rc.applyDefaults(cfg, units);

	BOOST_REQUIRE_EQUAL("common", cfg->name);
	BOOST_REQUIRE_EQUAL(13, cfg->routes.size());

	BOOST_REQUIRE_EQUAL("/", cfg->routes["index"]->path);
	BOOST_REQUIRE_EQUAL(HttpMethod::GET, cfg->routes["index"]->method);
	BOOST_REQUIRE_EQUAL("TestIceSpider.TestApi.index", *cfg->routes["index"]->operation);
	BOOST_REQUIRE_EQUAL(0, cfg->routes["index"]->params.size());

	BOOST_REQUIRE_EQUAL("/view/{s}/{i}", cfg->routes["item"]->path);
	BOOST_REQUIRE_EQUAL(2, cfg->routes["item"]->params.size());

	BOOST_REQUIRE_EQUAL(HttpMethod::DELETE, cfg->routes["del"]->method);
	BOOST_REQUIRE_EQUAL(1, cfg->routes["del"]->params.size());

	BOOST_REQUIRE_EQUAL(HttpMethod::POST, cfg->routes["update"]->method);
	BOOST_REQUIRE_EQUAL(2, cfg->routes["update"]->params.size());

	BOOST_REQUIRE_EQUAL(HttpMethod::GET, cfg->routes["mashStruct"]->method);
	BOOST_REQUIRE_EQUAL(3, cfg->routes["mashStruct"]->params.size());

	BOOST_REQUIRE_EQUAL(HttpMethod::GET, cfg->routes["mashClass"]->method);
	BOOST_REQUIRE_EQUAL(3, cfg->routes["mashClass"]->params.size());

	BOOST_REQUIRE_EQUAL(1, cfg->slices.size());
	BOOST_REQUIRE_EQUAL("test-api.ice", cfg->slices[0]);

	for (auto & u : units) {
		u.second->destroy();
	}
}

BOOST_AUTO_TEST_CASE(testRouteCompile)
{
	auto input = rootDir / "testRoutes.json";
	auto outputc = binDir / "testRoutes.cpp";

	Compile::RouteCompiler rc;
	rc.searchPath.push_back(rootDir);
	rc.compile(input, outputc);
}

BOOST_AUTO_TEST_SUITE_END();
