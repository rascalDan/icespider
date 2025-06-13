#include "pathparts.h"
#include <boost/algorithm/string/compare.hpp>
#include <boost/algorithm/string/find_iterator.hpp>
#include <boost/algorithm/string/finder.hpp>

namespace ba = boost::algorithm;

namespace IceSpider {
	const auto SLASH = ba::first_finder("/", ba::is_equal());

	Path::Path(const std::string_view path) : path(path)
	{
		auto relp = path.substr(1);
		if (relp.empty()) {
			return;
		}
		for (auto pi = ba::make_split_iterator(relp, SLASH); pi != decltype(pi)(); ++pi) {
			std::string_view pathPart {pi->begin(), pi->end()};
			if (pathPart.front() == '{' && pathPart.back() == '}') {
				parts.push_back(std::make_unique<PathParameter>(pathPart));
			}
			else {
				parts.push_back(std::make_unique<PathLiteral>(pathPart));
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

	PathLiteral::PathLiteral(const std::string_view value) : value(value) { }

	bool
	PathLiteral::matches(const std::string_view candidate) const
	{
		return value == candidate;
	}

	PathParameter::PathParameter(const std::string_view fmt) : name(fmt.substr(1, fmt.length() - 2)) { }

	bool
	PathParameter::matches(const std::string_view) const
	{
		return true;
	}
}
