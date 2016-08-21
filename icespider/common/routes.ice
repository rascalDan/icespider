#ifndef ICESPIDER_ROUTES_ICE
#define ICESPIDER_ROUTES_ICE

#include "http.ice"

module IceSpider {
	sequence<string> StringSeq;

	class Parameter {
		string name;
		ParameterSource source = URL;
		optional(0) string key;
		bool isOptional = false;
		["slicer:name:default"]
		optional(1) string defaultExpr;

		["slicer:ignore"]
		bool hasUserSource = true;
	};

	sequence<Parameter> Parameters;

	class OutputSerializer {
		string contentType;
		string serializer;
		StringSeq params;
	};

	sequence<OutputSerializer> OutputSerializers;

	class Route {
		string name;
		string path;
		HttpMethod method = GET;
		string operation;
		Parameters params;
		OutputSerializers outputSerializers;
	};

	sequence<Route> Routes;

	class RouteConfiguration {
		string name;
		Routes routes;
		StringSeq slices;
		StringSeq headers;
	};
};

#endif

