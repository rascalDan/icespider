#ifndef ICESPIDER_HTTP_ICE
#define ICESPIDER_HTTP_ICE

module IceSpider {
	["slicer:ignore"]
	local exception HttpException {
		int code;
		string message;
	};

	local enum HttpMethod {
		GET, HEAD, POST, PUT, DELETE, OPTIONS
	};

	local enum ParameterSource {
		URL, Body, QueryString, Header, Cookie
	};

	["slicer:ignore"]
	local struct MimeType {
		string group;
		string type;
	};

	["slicer:ignore"]
	local class Accept {
		optional(0) string group;
		optional(1) string type;
		float q = 1.0;
	};

	["slicer:json:object"]
	local dictionary<string, string> StringMap;
};

#endif

