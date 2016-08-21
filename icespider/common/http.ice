#ifndef ICESPIDER_HTTP_ICE
#define ICESPIDER_HTTP_ICE

module IceSpider {
	enum HttpMethod {
		GET, HEAD, POST, PUT, DELETE, OPTIONS
	};

	enum ParameterSource {
		URL, Body, QueryString, Header
	};

	struct MimeType {
		string group;
		string type;
	};

	class Accept {
		optional(0) string group;
		optional(1) string type;
		float q = 1.0;
	};
};

#endif

