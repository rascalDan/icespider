#include "util.h"
#include "exceptions.h"

namespace IceSpider {
	void
	conversion_failure()
	{
		throw Http400_BadRequest();
	}

	void
	remove_trailing(std::string_view & in, const char c)
	{
		if (const auto n = in.find_last_not_of(c); n != std::string_view::npos) {
			in.remove_suffix(in.length() - n - 1);
		}
	}

	void
	remove_leading(std::string_view & in, const char c)
	{
		if (const auto n = in.find_first_not_of(c); n != std::string_view::npos) {
			in.remove_prefix(n);
		}
	}
}
