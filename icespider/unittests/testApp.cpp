#define BOOST_TEST_MODULE TestApp
#include <boost/test/unit_test.hpp>

#include <plugins.h>
#include <irouteHandler.h>

BOOST_AUTO_TEST_CASE( testLoadConfiguration )
{
	BOOST_REQUIRE_EQUAL(4, AdHoc::PluginManager::getDefault()->getAll<IceSpider::IRouteHandler>().size());
}

