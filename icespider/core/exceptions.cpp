#include "exceptions.h"

namespace IceSpider {
	DefineHttpEx(Http400_BadRequest, 400, "Bad Request");
	DefineHttpEx(Http404_NotFound, 404, "Not found");
	DefineHttpEx(Http405_MethodNotAllowed, 405, "Method Not Allowed");
	DefineHttpEx(Http406_NotAcceptable, 406, "Not Acceptable");
	DefineHttpEx(Http415_UnsupportedMediaType, 415, "Unsupported Media Type");
}
