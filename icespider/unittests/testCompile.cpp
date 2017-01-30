#define BOOST_TEST_MODULE TestCompile
#include <boost/test/unit_test.hpp>

#include <definedDirs.h>
#include <plugins.h>
#include <dlfcn.h>
#include "../compile/routeCompiler.h"
#include "../core/irouteHandler.h"
#include <boost/algorithm/string/join.hpp>

using namespace IceSpider;

static void forceEarlyChangeDir() __attribute__((constructor(101)));
void forceEarlyChangeDir()
{
	boost::filesystem::current_path(XSTR(ROOT));
}

class CoreFixture {
	protected:
		CoreFixture() :
			modeDir(binDir.lexically_relative(rootDir / "bin" / "testCompile.test"))
		{
		}
		boost::filesystem::path modeDir;
};

BOOST_FIXTURE_TEST_SUITE(cf, CoreFixture)

BOOST_AUTO_TEST_CASE( testLoadConfiguration )
{
	Compile::RouteCompiler rc;
	rc.searchPath.push_back(rootDir);
	auto cfg = rc.loadConfiguration(rootDir / "testRoutes.json");
	auto u = rc.loadUnits(cfg);
	rc.applyDefaults(cfg, u);

	BOOST_REQUIRE_EQUAL("common", cfg->name);
	BOOST_REQUIRE_EQUAL(14, cfg->routes.size());

	BOOST_REQUIRE_EQUAL("/", cfg->routes["index"]->path);
	BOOST_REQUIRE_EQUAL(HttpMethod::GET, cfg->routes["index"]->method);
	BOOST_REQUIRE_EQUAL("TestIceSpider.TestApi.index", cfg->routes["index"]->operation);
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
}

BOOST_AUTO_TEST_CASE( testRouteCompile )
{
	auto input = rootDir / "testRoutes.json";
	auto outputc = binDir / "testRoutes.cpp";

	Compile::RouteCompiler rc;
	rc.searchPath.push_back(rootDir);
	rc.compile(input, outputc);
}

BOOST_AUTO_TEST_CASE( testCompile )
{
	auto outputc = binDir / "testRoutes.cpp";
	auto outputo = binDir / "testRoutes.o";
	auto libGenDir = (rootDir / "bin" / modeDir / "slicer-yes");

	auto compileCommand = boost::algorithm::join<std::vector<std::string>>({
		"gcc", "-c", "-std=c++1y", "-fPIC", "-fvisibility=hidden", "-O3",
		"-I", "/usr/include/adhocutil",
		"-I", "/usr/include/glib-2.0",
		"-I", "/usr/include/glibmm-2.4",
		"-I", "/usr/include/libxml2",
		"-I", "/usr/include/libxml++-2.6",
		"-I", "/usr/include/libxslt",
		"-I", "/usr/include/slicer",
		"-I", "/usr/lib/glib-2.0/include",
		"-I", "/usr/lib/glibmm-2.4/include",
		"-I", "/usr/lib/libxml++-2.6/include",
		"-I", rootDir.string(),
		"-I", (rootDir.parent_path() / "core").string(),
		"-I", (rootDir.parent_path() / "common").string(),
		"-I", (rootDir.parent_path() / "xslt").string(),
		"-I", (rootDir.parent_path() / "common" / "bin" / modeDir / "allow-ice-yes").string(),
		"-I", libGenDir.string(),
		"-o", outputo.string(),
		outputc.string(),
	}, " ");
	BOOST_TEST_INFO("Compile command: " << compileCommand);
	int compileResult = system(compileCommand.c_str());
	BOOST_REQUIRE_EQUAL(0, compileResult);
}

BOOST_AUTO_TEST_CASE( testLink )
{
	auto outputo = binDir / "testRoutes.o";
	BOOST_REQUIRE(boost::filesystem::exists(outputo));
	auto outputso = binDir / "testRoutes.so";

	auto linkCommand = boost::algorithm::join<std::vector<std::string>>({
		"gcc", "-shared", "-Wl,--warn-once,--gc-sections,-z,lazy",
		"-o", outputso.string(),
		outputo.string(),
	}, " ");
	BOOST_TEST_INFO("Link command: " << linkCommand);
	int linkResult = system(linkCommand.c_str());
	BOOST_REQUIRE_EQUAL(0, linkResult);
}

BOOST_AUTO_TEST_CASE( testLoad )
{
	auto outputso = binDir / "testRoutes.so";
	BOOST_REQUIRE(boost::filesystem::exists(outputso));

	auto lib = dlopen(outputso.c_str(), RTLD_LAZY);
	BOOST_TEST_INFO(dlerror());
	BOOST_REQUIRE(lib);

	BOOST_REQUIRE_EQUAL(14, AdHoc::PluginManager::getDefault()->getAll<IceSpider::RouteHandlerFactory>().size());
	// smoke test (block ensure dlclose dones't cause segfault)
	{
		auto route = AdHoc::PluginManager::getDefault()->get<IceSpider::RouteHandlerFactory>("common::index");
		BOOST_REQUIRE(route);
	}

	BOOST_REQUIRE_EQUAL(0, dlclose(lib));
}

BOOST_AUTO_TEST_SUITE_END();

