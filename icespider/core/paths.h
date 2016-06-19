#ifndef ICESPIDER_CORE_PATHS_H
#define ICESPIDER_CORE_PATHS_H

#include <vector>
#include <string>
#include <memory>

namespace IceSpider {

	class PathPart {
		public:
			virtual bool matches(const std::string &) const = 0;
	};
	typedef std::shared_ptr<PathPart> PathPartPtr;

	class PathLiteral : public PathPart {
		public:
			PathLiteral(const std::string & v);

			bool matches(const std::string &) const;

		private:
			const std::string value;
	};

	class PathParameter : public PathPart {
		public:
			bool matches(const std::string &) const;
	};
	class Path {
		public:
			typedef std::vector<PathPartPtr> PathParts;

			Path(const std::string &);

			const std::string path;

			unsigned int pathElementCount() const;

			PathParts parts;
	};
}

#endif

