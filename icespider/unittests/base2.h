#ifndef ICESPIDER_TEST_BASE2_H
#define ICESPIDER_TEST_BASE2_H

// Standard headers.
#include <visibility.h>

// Interface headers.
#include "test-api.h"

namespace IceSpider {
	class Core;
	class IHttpRequest;
}

namespace common {
	// Base classes.

	class DLL_PUBLIC base2 {
	protected:
		explicit base2(const IceSpider::Core * core);

		void testMutate(const IceSpider::IHttpRequest *, TestIceSpider::SomeModelPtr &) const;
	}; // base2
} // namespace common

#endif
