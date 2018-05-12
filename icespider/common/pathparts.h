#ifndef ICESPIDER_CORE_PATHS_H
#define ICESPIDER_CORE_PATHS_H

#include <vector>
#include <string_view>
#include <memory>
#include <visibility.h>

namespace IceSpider {
	class DLL_PUBLIC PathPart {
		public:
			virtual ~PathPart() = default;

			virtual bool matches(const std::string_view &) const = 0;
	};
	typedef std::unique_ptr<PathPart> PathPartPtr;

	class DLL_PUBLIC PathLiteral : public PathPart {
		public:
			PathLiteral(const std::string_view & v);

			bool matches(const std::string_view &) const;

			const std::string_view value;
	};

	class DLL_PUBLIC PathParameter : public PathPart {
		public:
			PathParameter(const std::string_view &);

			bool matches(const std::string_view &) const;

			const std::string_view name;
	};

	class DLL_PUBLIC Path {
		public:
			typedef std::vector<PathPartPtr> PathParts;

			Path(const std::string_view &);

			const std::string_view path;

			unsigned int pathElementCount() const;

			PathParts parts;
	};
}

#endif

