#include "pathparts.h"
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/trim.hpp>

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
				parts.push_back(std::make_shared<PathParameter>(pp));
			}
			else {
				parts.push_back(std::make_shared<PathLiteral>(pp));
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

	PathParameter::PathParameter(const std::string & s) :
		name(boost::algorithm::trim_copy_if(s, ispunct))
	{
	}

	bool
	PathParameter::matches(const std::string &) const
	{
		return true;
	}
}

