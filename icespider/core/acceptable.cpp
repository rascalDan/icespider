#include "acceptable.h"
#include <stdlib.h>
#include <string.h>

namespace IceSpider {
	void
	Acceptable::free()
	{
		::free(grp);
		::free(type);
	}
}
