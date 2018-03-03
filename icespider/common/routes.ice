#ifndef ICESPIDER_ROUTES_ICE
#define ICESPIDER_ROUTES_ICE

#include "http.ice"

module IceSpider {
	local sequence<string> StringSeq;

	local class Parameter {
		ParameterSource source = URL;
		optional(0) string key;
		bool isOptional = false;
		["slicer:name:default"]
		optional(1) string defaultExpr;
		optional(2) string type;

		["slicer:ignore"]
		bool hasUserSource = true;
	};

	["slicer:json:object"]
	local dictionary<string, Parameter> Parameters;

	local class OutputSerializer {
		string serializer;
		StringSeq params;
	};

	["slicer:json:object"]
	local dictionary<string, OutputSerializer> OutputSerializers;

	local class Operation {
		string operation;
		StringMap paramOverrides;
	};

	["slicer:json:object"]
	local dictionary<string, Operation> Operations;

	local class Route {
		string path;
		HttpMethod method = GET;
		optional(0) string operation;
		Parameters params;
		Operations operations;
		string type;
		OutputSerializers outputSerializers;
		StringSeq bases;
		StringSeq mutators;
	};

	["slicer:json:object"]
	local dictionary<string, Route> Routes;

	local class RouteBase {
		StringSeq proxies;
		StringSeq functions;
	};

	["slicer:json:object"]
	local dictionary<string, RouteBase> RouteBases;

	local class RouteConfiguration {
		string name;
		Routes routes;
		RouteBases routeBases;
		StringSeq slices;
		StringSeq headers;
	};
};

#endif

