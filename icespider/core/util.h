#pragma once

#include <Ice/Optional.h>
#include <array>
#include <boost/lexical_cast.hpp>
#include <charconv>
#include <optional>
#include <string_view>
#include <visibility.h>

namespace std::experimental::Ice {
	template<typename T, typename TF>
	inline const T &
	operator/(const Ice::optional<T> & o, const TF & tf)
	{
		if (o) {
			return *o;
		}
		return tf();
	}

	template<typename T, typename TF>
	inline T
	operator/(Ice::optional<T> && o, const TF & tf)
	{
		if (o) {
			return std::move(*o);
		}
		return tf();
	}
}

namespace std {
	template<typename T, typename TF>
	inline const T &
	operator/(const std::optional<T> & o, const TF & tf)
	{
		if (o) {
			return *o;
		}
		return tf();
	}

	template<typename T, typename TF>
	inline T
	operator/(std::optional<T> && o, const TF & tf)
	{
		if (o) {
			return std::move(*o);
		}
		return tf();
	}
}

template<typename T> struct type_names {
	static constexpr auto
	pf()
	{
		return std::string_view {&__PRETTY_FUNCTION__[0], sizeof(__PRETTY_FUNCTION__)};
	}

	static constexpr auto
	name()
	{
		constexpr std::string_view with_T {"T = "};
		constexpr auto start {pf().find(with_T) + with_T.length()};
		constexpr auto end {pf().find(']', start)};
		constexpr auto name {pf().substr(start, end - start)};
		static_assert(name.find('<') == std::string_view::npos, "Templates not supported");
		return name;
	}

	static constexpr auto
	namespaces()
	{
		return std::count(name().begin(), name().end(), ':') / 2;
	}

	using char_type = typename decltype(name())::value_type;
};

template<typename T> class TypeName : type_names<T> {
public:
	constexpr static inline auto
	str()
	{
		return std::string_view {buf.data(), buf.size() - 1};
	}

	constexpr static inline auto
	c_str()
	{
		return buf.data();
	}

private:
	using tn = type_names<T>;

	constexpr static auto buf {[]() {
		std::array<typename tn::char_type, tn::name().length() - tn::namespaces() + 1> buf {};
		auto out {buf.begin()};
		auto cln = false;
		for (const auto & in : tn::name()) {
			if (in == ':') {
				if (cln) {
					*out++ = '.';
				}
				else {
					cln = true;
				}
			}
			else {
				*out++ = in;
				cln = false;
			}
		}
		return buf;
	}()};
};

namespace IceSpider {
	[[noreturn]] DLL_PUBLIC void conversion_failure();

	static_assert(std::is_assignable_v<std::string, std::string_view>);
	static_assert(std::is_assignable_v<std::string_view, std::string_view>);

	namespace {
		template<typename T>
		inline T
		from_chars(const std::string_view v, T && out)
		{
			if (std::from_chars(v.begin(), v.end(), out).ec != std::errc {}) {
				conversion_failure();
			}
			return out;
		}

		template<typename T>
		inline T
		lexical_cast(const std::string_view v, T && out)
		{
			if (!boost::conversion::try_lexical_convert(v, out)) {
				conversion_failure();
			}
			return out;
		}
	}

	template<typename T>
	inline T
	convert(const std::string_view v, T && out = {})
	{
		if constexpr (std::is_assignable_v<T, std::string_view>) {
			return (out = v);
		}
		else if constexpr (requires { std::from_chars(v.begin(), v.end(), out); }) {
			return from_chars(v, out);
		}
		else {
			return lexical_cast(v, out);
		}
	}

	void remove_trailing(std::string_view & in, const char c);
	void remove_leading(std::string_view & in, const char c);
}
