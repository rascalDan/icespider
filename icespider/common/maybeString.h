#pragma once

#include <iosfwd>
#include <string>
#include <string_view>
#include <utility>
#include <variant>

namespace IceSpider {
	class MaybeString {
	public:
		MaybeString() = default;

		// cppcheck-suppress noExplicitConstructor; NOLINTNEXTLINE(hicpp-explicit-conversions)
		MaybeString(std::string str) : valueContainer {std::move(str)} { }

		// cppcheck-suppress noExplicitConstructor; NOLINTNEXTLINE(hicpp-explicit-conversions)
		MaybeString(std::string_view str) : valueContainer {str} { }

		// NOLINTNEXTLINE(hicpp-explicit-conversions)
		[[nodiscard]] operator std::string_view() const
		{
			if (valueContainer.index() == 0) {
				return std::get<0>(valueContainer);
			}
			return std::get<1>(valueContainer);
		}

		[[nodiscard]] std::string_view
		value() const
		{
			return *this;
		}

		[[nodiscard]] bool
		isString() const
		{
			return valueContainer.index() > 0;
		}

		[[nodiscard]] bool
		operator<(const MaybeString & other) const
		{
			return value() < other.value();
		}

		[[nodiscard]] bool
		operator<(const std::string_view other) const
		{
			return value() < other;
		}

	private:
		using ValueType = std::variant<std::string_view, std::string>;
		ValueType valueContainer;
	};
};

namespace std {
	inline std::ostream &
	operator<<(std::ostream & strm, const IceSpider::MaybeString & value)
	{
		return strm << value.value();
	}
}
