#ifndef ICESPIDER_HTTP_ICE
#define ICESPIDER_HTTP_ICE

[["ice-prefix"]]
[["underscore"]]
module IceSpider {
	["slicer:ignore"]
	local exception HttpException {
		short code;
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

	["slicer:json:object",
		"cpp:type:std::map<std::string, std::string, std::less<>>"]
	local dictionary<string, string> StringMap;

	module S { // Statuses
		const string OK = "OK";
		const string MOVED = "Moved";
	};
	module H { // Header names
		const string ACCEPT = "Accept";
		const string LOCATION = "Location";
		const string SET_COOKIE = "Set-Cookie";
		const string CONTENT_TYPE = "Content-Type";
	};
	module MIME { // Common MIME types
		const string TEXT_PLAIN = "text/plain";
	};
	module E { // Common environment vars
		const string CONTENT_TYPE = "CONTENT_TYPE";
	};
};

#endif

