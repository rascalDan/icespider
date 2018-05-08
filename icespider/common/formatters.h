#ifndef ICESPIDER_COMMON_FORMATTERS_H
#define ICESPIDER_COMMON_FORMATTERS_H

#include <compileTimeFormatter.h>

namespace IceSpider {
	AdHocFormatter(StatusFmt, "Status: %? %?\r\n\r\n");
	AdHocFormatter(HdrFmt, "%?: %?\r\n");
	AdHocFormatter(MimeTypeFmt, "%?/%?");
}

#endif

