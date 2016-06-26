#ifndef ICESPIDER_ROUTES_ICE
#define ICESPIDER_ROUTES_ICE

#include "http.ice"

module IceSpider {
	class Parameter {
		string name;
		ParameterSource source = URL;
		optional(0) string key;
		bool isOptional = false;
		["slicer:name:default"]
		optional(1) string defaultExpr;

		["slicer:ignore"]
		bool hasUserSource;
	};

	sequence<Parameter> Parameters;

	class Route {
		string name;
		string path;
		HttpMethod method = GET;
		string operation;
		Parameters params;
	};

	sequence<Route> Routes;

	sequence<string> Slices;

	class RouteConfiguration {
		string name;
		Routes routes;
		Slices slices;
	};
};

#endif

