#pragma once

#include <c++11Helpers.h>
#include <cstddef>
#include <memory>
#include <string_view>
#include <vector>
#include <visibility.h>

namespace IceSpider {
	using PathElements = std::vector<std::string>;

	class DLL_PUBLIC PathPart {
	public:
		PathPart() = default;
		virtual ~PathPart() = default;
		SPECIAL_MEMBERS_DEFAULT(PathPart);

		[[nodiscard]] virtual bool matches(const std::string_view &) const = 0;
	};

	using PathPartPtr = std::unique_ptr<PathPart>;

	class DLL_PUBLIC PathLiteral : public PathPart {
	public:
		explicit PathLiteral(const std::string_view & v);

		[[nodiscard]] bool matches(const std::string_view &) const override;

		const std::string_view value;
	};

	class DLL_PUBLIC PathParameter : public PathPart {
	public:
		explicit PathParameter(const std::string_view &);

		[[nodiscard]] bool matches(const std::string_view &) const override;

		const std::string_view name;
	};

	class DLL_PUBLIC Path {
	public:
		using PathParts = std::vector<PathPartPtr>;

		explicit Path(const std::string_view &);

		std::string_view path;

		[[nodiscard]] std::size_t pathElementCount() const;
		[[nodiscard]] bool matches(const PathElements &) const;

		PathParts parts;
	};
}
