#ifndef ICESPIDER_COMMON_MAYBESTRING_H
#define ICESPIDER_COMMON_MAYBESTRING_H

#include <string>
#include <string_view>
#include <variant>

namespace IceSpider {
	class MaybeString {
	public:
		MaybeString() { }

		// NOLINTNEXTLINE(hicpp-explicit-conversions)
		inline MaybeString(std::string s) : value_ {std::move(s)} { }

		// NOLINTNEXTLINE(hicpp-explicit-conversions)
		inline MaybeString(std::string_view s) : value_ {std::move(s)} { }

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
		operator<(const std::string_view & o) const
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

#endif
