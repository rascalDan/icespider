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
	BOOST_REQUIRE_EQUAL(8, cfg->routes.size());

	BOOST_REQUIRE_EQUAL("index", cfg->routes[0]->name);
	BOOST_REQUIRE_EQUAL("/", cfg->routes[0]->path);
	BOOST_REQUIRE_EQUAL(HttpMethod::GET, cfg->routes[0]->method);
	BOOST_REQUIRE_EQUAL("TestIceSpider.TestApi.index", cfg->routes[0]->operation);
	BOOST_REQUIRE_EQUAL(0, cfg->routes[0]->params.size());

	BOOST_REQUIRE_EQUAL("item", cfg->routes[1]->name);
	BOOST_REQUIRE_EQUAL("/view/{s}/{i}", cfg->routes[1]->path);
	BOOST_REQUIRE_EQUAL(2, cfg->routes[1]->params.size());

	BOOST_REQUIRE_EQUAL("del", cfg->routes[2]->name);
	BOOST_REQUIRE_EQUAL(HttpMethod::DELETE, cfg->routes[2]->method);
	BOOST_REQUIRE_EQUAL(1, cfg->routes[2]->params.size());

	BOOST_REQUIRE_EQUAL("update", cfg->routes[3]->name);
	BOOST_REQUIRE_EQUAL(HttpMethod::POST, cfg->routes[3]->method);
	BOOST_REQUIRE_EQUAL(2, cfg->routes[3]->params.size());

	BOOST_REQUIRE_EQUAL("mashStruct", cfg->routes[6]->name);
	BOOST_REQUIRE_EQUAL(HttpMethod::GET, cfg->routes[6]->method);
	BOOST_REQUIRE_EQUAL(3, cfg->routes[6]->params.size());

	BOOST_REQUIRE_EQUAL("mashClass", cfg->routes[7]->name);
	BOOST_REQUIRE_EQUAL(HttpMethod::GET, cfg->routes[7]->method);
	BOOST_REQUIRE_EQUAL(3, cfg->routes[7]->params.size());

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
	auto outputso = binDir / "testRoutes.so";

	auto linkCommand = boost::algorithm::join<std::vector<std::string>>({
		"gcc", "-shared", "-Wl,--warn-once,--gc-sections",
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

	auto lib = dlopen(outputso.c_str(), RTLD_NOW);
	BOOST_TEST_INFO(dlerror());
	BOOST_REQUIRE(lib);

	BOOST_REQUIRE_EQUAL(8, AdHoc::PluginManager::getDefault()->getAll<IRouteHandler>().size());
	// smoke test (block ensure dlclose dones't cause segfault)
	{
		auto route = AdHoc::PluginManager::getDefault()->get<IRouteHandler>("common::index");
		BOOST_REQUIRE(route);
	}

	BOOST_REQUIRE_EQUAL(0, dlclose(lib));
}

BOOST_AUTO_TEST_SUITE_END();

