#include "util.h"
#include "exceptions.h"

namespace IceSpider {
	void
	conversionFailure()
	{
		throw Http400BadRequest();
	}
}
