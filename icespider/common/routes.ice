#ifndef ICESPIDER_ROUTES_ICE
#define ICESPIDER_ROUTES_ICE

#include "http.ice"

module IceSpider {
	sequence<string> StringSeq;
	["slicer:json:object"]
	dictionary<string, string> StringMap;

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

	class Operation {
		string operation;
		StringMap paramOverrides;
	};

	["slicer:json:object"]
	dictionary<string, Operation> Operations;

	class Route {
		string name;
		string path;
		HttpMethod method = GET;
		optional(0) string operation;
		Parameters params;
		Operations operations;
		string type;
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

