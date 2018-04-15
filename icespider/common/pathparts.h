#ifndef ICESPIDER_CORE_PATHS_H
#define ICESPIDER_CORE_PATHS_H

#include <vector>
#include <string>
#include <memory>
#include <visibility.h>

namespace IceSpider {
	class DLL_PUBLIC PathPart {
		public:
			virtual ~PathPart() = default;

			virtual bool matches(const std::string &) const = 0;
	};
	typedef std::shared_ptr<PathPart> PathPartPtr;

	class DLL_PUBLIC PathLiteral : public PathPart {
		public:
			PathLiteral(const std::string & v);

			bool matches(const std::string &) const;

			const std::string value;
	};

	class DLL_PUBLIC PathParameter : public PathPart {
		public:
			PathParameter(const std::string &);

			bool matches(const std::string &) const;

			const std::string name;
	};

	class DLL_PUBLIC Path {
		public:
			typedef std::vector<PathPartPtr> PathParts;

			Path(const std::string &);

			const std::string path;

			unsigned int pathElementCount() const;

			PathParts parts;
	};
}

#endif

