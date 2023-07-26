#pragma once

#include <compileTimeFormatter.h>

namespace IceSpider {
	AdHocFormatter(StatusFmt, "Status: %? %?\r\n\r\n");
	AdHocFormatter(HdrFmt, "%?: %?\r\n");
	AdHocFormatter(MimeTypeFmt, "%?/%?");
}
