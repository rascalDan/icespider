#define BOOST_TEST_MODULE TestCompile
#include <boost/test/unit_test.hpp>

#include <definedDirs.h>
#include <plugins.h>
#include <dlfcn.h>
#include "../compile/routeCompiler.h"
#include "../core/irouteHandler.h"
#include <boost/algorithm/string/join.hpp>

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
	IceSpider::Compile::RouteCompiler rc;
	rc.searchPath.push_back(rootDir);
	auto cfg = rc.loadConfiguration(rootDir / "testRoutes.json");
	auto u = rc.loadUnits(cfg);
	rc.applyDefaults(cfg, u);

	BOOST_REQUIRE_EQUAL("common", cfg->name);
	BOOST_REQUIRE_EQUAL(4, cfg->routes.size());

	BOOST_REQUIRE_EQUAL("index", cfg->routes[0]->name);
	BOOST_REQUIRE_EQUAL("/", cfg->routes[0]->path);
	BOOST_REQUIRE_EQUAL(UserIceSpider::HttpMethod::GET, cfg->routes[0]->method);
	BOOST_REQUIRE_EQUAL("TestIceSpider.TestApi.index", cfg->routes[0]->operation);
	BOOST_REQUIRE_EQUAL(0, cfg->routes[0]->params.size());

	BOOST_REQUIRE_EQUAL("item", cfg->routes[1]->name);
	BOOST_REQUIRE_EQUAL("/view/{s}/{i}", cfg->routes[1]->path);
	BOOST_REQUIRE_EQUAL(2, cfg->routes[1]->params.size());

	BOOST_REQUIRE_EQUAL("del", cfg->routes[2]->name);
	BOOST_REQUIRE_EQUAL(UserIceSpider::HttpMethod::DELETE, cfg->routes[2]->method);
	BOOST_REQUIRE_EQUAL(1, cfg->routes[2]->params.size());

	BOOST_REQUIRE_EQUAL("update", cfg->routes[3]->name);
	BOOST_REQUIRE_EQUAL(UserIceSpider::HttpMethod::POST, cfg->routes[3]->method);
	BOOST_REQUIRE_EQUAL(2, cfg->routes[3]->params.size());

	BOOST_REQUIRE_EQUAL(1, cfg->slices.size());
	BOOST_REQUIRE_EQUAL("test-api.ice", cfg->slices[0]);
}

BOOST_AUTO_TEST_CASE( testCompile )
{
	IceSpider::Compile::RouteCompiler rc;
	rc.searchPath.push_back(rootDir);
	auto input = rootDir / "testRoutes.json";
	auto output = binDir / "testRoutes.cpp";
	auto outputso = boost::filesystem::change_extension(output, ".so");
	auto libGenDir = (rootDir / "bin" / modeDir);
	rc.compile(input, output);

	auto compileCommand = boost::algorithm::join<std::vector<std::string>>({
		"gcc", "-shared", "-std=c++1y", "-fPIC", " -fvisibility=hidden", "-O3", "-Wl,--warn-once,--gc-sections",
		"-I", "/usr/include/adhocutil",
		"-I", (rootDir.parent_path() / "core").string(),
		"-I", (rootDir.parent_path() / "common").string(),
		"-I", (rootDir.parent_path() / "common" / "bin" / modeDir).string(),
		"-I", libGenDir.string(),
		"-o", outputso.string(),
		output.string(),
	}, " ");
	BOOST_TEST_INFO("Compile command: " << compileCommand);
	int compileResult = system(compileCommand.c_str());
	BOOST_REQUIRE_EQUAL(0, compileResult);
}

BOOST_AUTO_TEST_CASE( testLoad )
{
	auto outputso = binDir / "testRoutes.so";
	auto lib = dlopen(outputso.c_str(), RTLD_NOW);
	BOOST_TEST_INFO(dlerror());
	BOOST_REQUIRE(lib);

	BOOST_REQUIRE_EQUAL(4, AdHoc::PluginManager::getDefault()->getAll<IceSpider::IRouteHandler>().size());
	// smoke test (block ensure dlclose dones't cause segfault)
	{
		auto route = AdHoc::PluginManager::getDefault()->get<IceSpider::IRouteHandler>("common::index");
		BOOST_REQUIRE(route);
	}

	BOOST_REQUIRE_EQUAL(0, dlclose(lib));
}

BOOST_AUTO_TEST_SUITE_END();

