#pragma once

#include <Ice/Optional.h>
#include <algorithm>
#include <array>
#include <boost/lexical_cast.hpp>
#include <charconv>
#include <optional>
#include <string_view>
#include <visibility.h>

namespace std::experimental::Ice {
	template<typename T, typename TF>
	inline const T &
	operator/(const Ice::optional<T> & value, const TF & whenNullOpt)
	{
		if (value) {
			return *value;
		}
		return whenNullOpt();
	}

	template<typename T, typename TF>
	inline T
	operator/(Ice::optional<T> && value, const TF & whenNullOpt)
	{
		if (value) {
			return *std::move(value);
		}
		return whenNullOpt();
	}
}

namespace std {
	template<typename T, typename TF>
	inline const T &
	operator/(const std::optional<T> & value, const TF & whenNullOpt)
	{
		if (value) {
			return *value;
		}
		return whenNullOpt();
	}

	template<typename T, typename TF>
	inline T
	operator/(std::optional<T> && value, const TF & whenNullOpt)
	{
		if (value) {
			return *std::move(value);
		}
		return whenNullOpt();
	}
}

template<typename T> struct TypeNames {
	static constexpr auto
	pf()
	{
		return std::string_view {&__PRETTY_FUNCTION__[0], sizeof(__PRETTY_FUNCTION__)};
	}

	static constexpr auto
	name()
	{
		constexpr std::string_view WITH_T {"T = "};
		constexpr auto START {pf().find(WITH_T) + WITH_T.length()};
		constexpr auto END {pf().find(']', START)};
		constexpr auto NAME {pf().substr(START, END - START)};
		static_assert(NAME.find('<') == std::string_view::npos, "Templates not supported");
		return NAME;
	}

	static constexpr auto
	namespaces()
	{
		return std::ranges::count(name(), ':') / 2;
	}

	using CharType = typename decltype(name())::value_type;
};

template<typename T> class TypeName : TypeNames<T> {
public:
	constexpr static auto
	str()
	{
		return std::string_view {BUF.data(), BUF.size() - 1};
	}

	constexpr static auto
	c_str() // NOLINT(readability-identifier-naming) - STL like
	{
		return BUF.data();
	}

private:
	using Tn = TypeNames<T>;

	constexpr static auto BUF {[]() {
		std::array<typename Tn::CharType, Tn::name().length() - Tn::namespaces() + 1> buf {};
		auto out {buf.begin()};
		auto cln = false;
		for (const auto & input : Tn::name()) {
			if (input == ':') {
				if (cln) {
					*out++ = '.';
				}
				else {
					cln = true;
				}
			}
			else {
				*out++ = input;
				cln = false;
			}
		}
		return buf;
	}()};
};

namespace IceSpider {
	[[noreturn]] DLL_PUBLIC void conversionFailure();

	static_assert(std::is_assignable_v<std::string, std::string_view>);
	static_assert(std::is_assignable_v<std::string_view, std::string_view>);

	namespace {
		template<typename T>
		inline T
		from_chars(const std::string_view chars, T && out) // NOLINT(readability-identifier-naming) - STL like
		{
			if (std::from_chars(chars.begin(), chars.end(), std::forward<T>(out)).ec != std::errc {}) {
				conversionFailure();
			}
			return out;
		}

		template<typename T>
		inline T
		lexical_cast(const std::string_view value, T && out) // NOLINT(readability-identifier-naming) - Boost like
		{
			if (!boost::conversion::try_lexical_convert(value, std::forward<T>(out))) {
				conversionFailure();
			}
			return out;
		}
	}

	template<typename T>
	inline T
	convert(const std::string_view value, T && out)
	{
		if constexpr (std::is_assignable_v<T, std::string_view>) {
			return (out = value);
		}
		else if constexpr (requires { std::from_chars(value.begin(), value.end(), out); }) {
			return from_chars(value, std::forward<T>(out));
		}
		else {
			return lexical_cast(value, std::forward<T>(out));
		}
	}

	template<typename T>
	inline T
	convert(const std::string_view value)
	{
		T tmp;
		return convert(value, tmp);
	}

	inline void
	remove_trailing(std::string_view & input, const char chr) // NOLINT(readability-identifier-naming) - STL like
	{
		if (const auto pos = input.find_last_not_of(chr); pos != std::string_view::npos) {
			input.remove_suffix(input.length() - pos - 1);
		}
	}

	inline void
	remove_leading(std::string_view & input, const char chr) // NOLINT(readability-identifier-naming) - STL like
	{
		if (const auto pos = input.find_first_not_of(chr); pos != std::string_view::npos) {
			input.remove_prefix(pos);
		}
	}
}
