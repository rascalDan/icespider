#ifndef ICESPIDER_ACCEPTABLE_H
#define ICESPIDER_ACCEPTABLE_H

#include <visibility.h>

namespace IceSpider {
	class DLL_PUBLIC Acceptable {
		public:
			void free();

			char * grp;
			char * type;
			float pri;
	};
}

#endif

