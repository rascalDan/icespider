module UserIceSpider {
	enum HttpMethod {
		GET, HEAD, POST, PUT, DELETE, OPTIONS
	};
	enum ParameterSource {
		URL, Body, QueryString, Header
	};
	class Parameter {
		string name;
		ParameterSource source;
		optional(0) string key;
	};
	sequence<Parameter> Parameters;
	class Route {
		string name;
		string path;
		HttpMethod method;
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

