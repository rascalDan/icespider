#pragma once

#include <c++11Helpers.h>
#include <cstddef>
#include <memory>
#include <string_view>
#include <vector>
#include <visibility.h>

namespace IceSpider {
	using PathElements = std::vector<std::string_view>;

	class DLL_PUBLIC PathPart {
	public:
		PathPart() = default;
		virtual ~PathPart() = default;
		SPECIAL_MEMBERS_DEFAULT(PathPart);

		[[nodiscard]] virtual bool matches(std::string_view) const = 0;
	};

	using PathPartPtr = std::unique_ptr<PathPart>;

	class DLL_PUBLIC PathLiteral : public PathPart {
	public:
		explicit PathLiteral(std::string_view value);

		[[nodiscard]] bool matches(std::string_view) const override;

		const std::string_view value;
	};

	class DLL_PUBLIC PathParameter : public PathPart {
	public:
		explicit PathParameter(std::string_view fmt);

		[[nodiscard]] bool matches(std::string_view) const override;

		const std::string_view name;
	};

	class DLL_PUBLIC Path {
	public:
		using PathParts = std::vector<PathPartPtr>;

		explicit Path(std::string_view path);

		std::string_view path;

		[[nodiscard]] std::size_t pathElementCount() const;
		[[nodiscard]] bool matches(const PathElements &) const;

		PathParts parts;
	};
}
