#ifndef ICESPIDER_EXCEPTIONS_H
#define ICESPIDER_EXCEPTIONS_H

#include "http.h"
#include <visibility.h>

#define DeclareHttpEx(Name) \
	class DLL_PUBLIC Name : public ::IceSpider::HttpException { \
		public: \
			Name(); \
			static const short code; \
			static const std::string message; \
	}
#define DefineHttpEx(Name, Code, Message) \
	Name::Name() : ::IceSpider::HttpException(__FILE__, __LINE__, code, message) { } \
	const short Name::code(Code); \
	const std::string Name::message(Message);

namespace IceSpider {
	DeclareHttpEx(Http400_BadRequest);
	DeclareHttpEx(Http404_NotFound);
	DeclareHttpEx(Http405_MethodNotAllowed);
	DeclareHttpEx(Http406_NotAcceptable);
	DeclareHttpEx(Http415_UnsupportedMediaType);
}

#endif

