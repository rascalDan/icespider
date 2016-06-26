#ifndef ICESPIDER_HTTP_ICE
#define ICESPIDER_HTTP_ICE

module IceSpider {
	enum HttpMethod {
		GET, HEAD, POST, PUT, DELETE, OPTIONS
	};

	enum ParameterSource {
		URL, Body, QueryString, Header
	};
};

#endif

