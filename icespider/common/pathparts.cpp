#include "pathparts.h"
#include <boost/algorithm/string/compare.hpp>
#include <boost/algorithm/string/find_iterator.hpp>
#include <boost/algorithm/string/finder.hpp>
#include <string>

namespace ba = boost::algorithm;

namespace IceSpider {
	const auto slash = ba::first_finder("/", ba::is_equal());

	Path::Path(const std::string_view p) : path(p)
	{
		auto relp = p.substr(1);
		if (relp.empty()) {
			return;
		}
		for (auto pi = ba::make_split_iterator(relp, slash); pi != decltype(pi)(); ++pi) {
			std::string_view pp {pi->begin(), pi->end()};
			if (pp.front() == '{' && pp.back() == '}') {
				parts.push_back(std::make_unique<PathParameter>(pp));
			}
			else {
				parts.push_back(std::make_unique<PathLiteral>(pp));
			}
		}
	}

	std::size_t
	Path::pathElementCount() const
	{
		return parts.size();
	}

	bool
	Path::matches(const PathElements & pathparts) const
	{
		return std::mismatch(pathparts.begin(), pathparts.end(), parts.begin(),
					   [](const auto & pathpart, const auto & routepart) {
						   return routepart->matches(pathpart);
					   })
					   .first
				== pathparts.end();
	}

	PathLiteral::PathLiteral(const std::string_view p) : value(p) { }

	bool
	PathLiteral::matches(const std::string_view v) const
	{
		return value == v;
	}

	PathParameter::PathParameter(const std::string_view s) : name(s.substr(1, s.length() - 2)) { }

	bool
	PathParameter::matches(const std::string_view) const
	{
		return true;
	}
}
