#pragma once

#include <compare>
#include <iosfwd>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>

namespace IceSpider {
	class MaybeString {
	public:
		MaybeString() = default;

		// cppcheck-suppress noExplicitConstructor; NOLINTNEXTLINE(hicpp-explicit-conversions)
		inline MaybeString(std::string s) : value_ {std::move(s)} { }

		// cppcheck-suppress noExplicitConstructor; NOLINTNEXTLINE(hicpp-explicit-conversions)
		inline MaybeString(std::string_view s) : value_ {s} { }

		// NOLINTNEXTLINE(hicpp-explicit-conversions)
		[[nodiscard]] inline operator std::string_view() const
		{
			if (value_.index() == 0) {
				return std::get<0>(value_);
			}
			return std::get<1>(value_);
		}

		[[nodiscard]] inline std::string_view
		value() const
		{
			return *this;
		}

		[[nodiscard]] inline bool
		isString() const
		{
			return value_.index() > 0;
		}

		[[nodiscard]] inline bool
		operator<(const MaybeString & o) const
		{
			return value() < o.value();
		}

		[[nodiscard]] inline bool
		operator<(const std::string_view o) const
		{
			return value() < o;
		}

	private:
		using value_type = std::variant<std::string_view, std::string>;
		value_type value_;
	};
};

namespace std {
	inline std::ostream &
	operator<<(std::ostream & s, const IceSpider::MaybeString & ms)
	{
		return s << ms.value();
	}
}
