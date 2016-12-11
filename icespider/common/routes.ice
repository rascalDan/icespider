#ifndef ICESPIDER_ROUTES_ICE
#define ICESPIDER_ROUTES_ICE

#include "http.ice"

module IceSpider {
	sequence<string> StringSeq;

	class Parameter {
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
	dictionary<string, Parameter> Parameters;

	class OutputSerializer {
		string serializer;
		StringSeq params;
	};

	["slicer:json:object"]
	dictionary<string, OutputSerializer> OutputSerializers;

	class Operation {
		string operation;
		StringMap paramOverrides;
	};

	["slicer:json:object"]
	dictionary<string, Operation> Operations;

	class Route {
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
	dictionary<string, Route> Routes;

	class RouteBase {
		StringSeq proxies;
		StringSeq functions;
	};

	["slicer:json:object"]
	dictionary<string, RouteBase> RouteBases;

	class RouteConfiguration {
		string name;
		Routes routes;
		RouteBases routeBases;
		StringSeq slices;
		StringSeq headers;
	};
};

#endif

