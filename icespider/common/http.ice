#ifndef ICESPIDER_HTTP_ICE
#define ICESPIDER_HTTP_ICE

module IceSpider {
	["slicer:ignore"]
	exception HttpException {
		int code;
		string message;
	};

	enum HttpMethod {
		GET, HEAD, POST, PUT, DELETE, OPTIONS
	};

	enum ParameterSource {
		URL, Body, QueryString, Header, Cookie
	};

	["slicer:ignore"]
	struct MimeType {
		string group;
		string type;
	};

	["slicer:ignore"]
	class Accept {
		optional(0) string group;
		optional(1) string type;
		float q = 1.0;
	};

	["slicer:json:object"]
	dictionary<string, string> StringMap;
};

#endif

