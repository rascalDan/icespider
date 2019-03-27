#include "pathparts.h"
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/trim.hpp>

namespace ba = boost::algorithm;

namespace IceSpider {
	Path::Path(const std::string_view & p) :
		path(p)
	{
		auto relp = p.substr(1);
		if (relp.empty()) {
			return;
		}
		for (auto pi = ba::make_split_iterator(relp, ba::first_finder("/", ba::is_equal())); pi != decltype(pi)(); ++pi) {
			std::string_view pp(pi->begin(), pi->end() - pi->begin());
			if (pp.front() == '{' && pp.back() == '}') {
				parts.push_back(std::make_unique<PathParameter>(pp));
			}
			else {
				parts.push_back(std::make_unique<PathLiteral>(pp));
			}
		}
	}

	unsigned int
	Path::pathElementCount() const
	{
		return parts.size();
	}

	PathLiteral::PathLiteral(const std::string_view & p) :
		value(p)
	{
	}

	bool
	PathLiteral::matches(const std::string_view & v) const
	{
		return value == v;
	}

	PathParameter::PathParameter(const std::string_view & s) :
		name(s.substr(1, s.length() - 2))
	{
	}

	bool
	PathParameter::matches(const std::string_view &) const
	{
		return true;
	}
}

