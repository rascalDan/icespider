#include "exceptions.h"
#include "http.h"

#define DefineHttpEx(Name, Code, Message) \
	Name::Name() : ::IceSpider::HttpException(__FILE__, __LINE__, CODE, MESSAGE) { } \
	const short Name::CODE(Code); \
	const std::string Name::MESSAGE(Message);

namespace IceSpider {
	DefineHttpEx(Http400BadRequest, 400, "Bad Request");
	DefineHttpEx(Http404NotFound, 404, "Not found");
	DefineHttpEx(Http405MethodNotAllowed, 405, "Method Not Allowed");
	DefineHttpEx(Http406NotAcceptable, 406, "Not Acceptable");
	DefineHttpEx(Http415UnsupportedMediaType, 415, "Unsupported Media Type");
	DefineHttpEx(Http500InternalServerError, 500, "Internal Server Error");
}
