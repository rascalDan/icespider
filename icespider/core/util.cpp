#include "util.h"
#include "exceptions.h"

namespace IceSpider {
	void
	conversion_failure()
	{
		throw Http400_BadRequest();
	}
}
