#ifndef ICESPIDER_CORE_UTIL_H
#define ICESPIDER_CORE_UTIL_H

#include <Ice/Optional.h>
#include <array>
#include <string_view>

namespace std::experimental::Ice {
	template<typename T, typename TF>
	auto
	operator/(const Ice::optional<T> & o, const TF & tf) -> decltype(tf())
	{
		if (o) {
			return *o;
		}
		return tf();
	}
}

namespace std {
	template<typename T, typename TF>
	auto
	operator/(const std::optional<T> & o, const TF & tf) -> decltype(tf())
	{
		if (o) {
			return *o;
		}
		return tf();
	}
}

template<typename T>
T
orelse(const T & a, const T & b)
{
	if (a) {
		return a;
	}
	return b;
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
		const std::string_view with_T {"T = "};
		const auto start {pf().find(with_T) + with_T.length()};
		const auto end {pf().find(']', start)};
		return pf().substr(start, end - start);
	}

	static constexpr auto
	namespaces()
	{
		auto ns {0U};
		for (const auto & c : name()) {
			// cppcheck-suppress useStlAlgorithm; (not constexpr)
			ns += (c == ':') ? 1 : 0;
		}
		return ns / 2;
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
		auto cln {false};
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

#endif
