#include "paths.h"
#include <boost/algorithm/string/split.hpp>

namespace ba = boost::algorithm;

namespace IceSpider {
	Path::Path(const std::string & p) :
		path(p)
	{
		auto relp = p.substr(1);
		if (relp.empty()) return;
		for (auto pi = ba::make_split_iterator(relp, ba::first_finder("/", ba::is_equal())); pi != decltype(pi)(); ++pi) {
			std::string pp(pi->begin(), pi->end());
			if (pp.front() == '{' && pp.back() == '}') {
				parts.push_back(PathPartPtr(new PathParameter()));
			}
			else {
				parts.push_back(PathPartPtr(new PathLiteral(pp)));
			}
		}
	}

	unsigned int
	Path::pathElementCount() const
	{
		return parts.size();
	}

	PathLiteral::PathLiteral(const std::string & p) :
		value(p)
	{
		
	}

	bool
	PathLiteral::matches(const std::string & v) const
	{
		return value == v;
	}

	bool
	PathParameter::matches(const std::string &) const
	{
		return true;
	}
}

