#include "exceptions.h"

#define DefineHttpEx(Name, Code, Message) \
	Name::Name() : ::IceSpider::HttpException(__FILE__, __LINE__, CODE, MESSAGE) { } \
	const short Name::CODE(Code); \
	const std::string Name::MESSAGE(Message);

namespace IceSpider {
	DefineHttpEx(Http400_BadRequest, 400, "Bad Request");
	DefineHttpEx(Http404_NotFound, 404, "Not found");
	DefineHttpEx(Http405_MethodNotAllowed, 405, "Method Not Allowed");
	DefineHttpEx(Http406_NotAcceptable, 406, "Not Acceptable");
	DefineHttpEx(Http415_UnsupportedMediaType, 415, "Unsupported Media Type");
}
