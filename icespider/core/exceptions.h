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
	DeclareHttpEx(Http400_BadRequest);
	DeclareHttpEx(Http404_NotFound);
	DeclareHttpEx(Http405_MethodNotAllowed);
	DeclareHttpEx(Http406_NotAcceptable);
	DeclareHttpEx(Http415_UnsupportedMediaType);
	DeclareHttpEx(Http500_InternalServerError);
}

#undef DeclareHttpEx
