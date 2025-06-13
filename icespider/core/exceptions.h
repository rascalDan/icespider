#pragma once

#include "http.h"
#include <string>
#include <visibility.h>

#define DeclareHttpEx(Name) \
	class DLL_PUBLIC Name : public ::IceSpider::HttpException { \
	public: \
		Name(); \
\
		static const short CODE; \
		static const std::string MESSAGE; \
	}

namespace IceSpider {
	DeclareHttpEx(Http400BadRequest);
	DeclareHttpEx(Http404NotFound);
	DeclareHttpEx(Http405MethodNotAllowed);
	DeclareHttpEx(Http406NotAcceptable);
	DeclareHttpEx(Http415UnsupportedMediaType);
	DeclareHttpEx(Http500InternalServerError);
}

#undef DeclareHttpEx
