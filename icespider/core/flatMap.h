#pragma once

#include <algorithm>
#include <cstddef>
#include <functional>
#include <iterator>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace IceSpider {
	template<typename K, typename M, typename Comp = std::less<>> class flatmap : std::vector<std::pair<K, M>> {
	public:
		using V = std::pair<K, M>;
		using S = std::vector<V>;

	private:
		template<typename N> struct KeyComp {
			bool
			operator()(const V & v, const N & n) const
			{
				return c(v.first, n);
			}

			bool
			operator()(const N & n, const V & v) const
			{
				return c(n, v.first);
			}

			Comp c;
		};

	public:
		flatmap() = default;

		explicit flatmap(std::size_t n)
		{
			reserve(n);
		}

		auto
		insert(V v)
		{
			return S::insert(lower_bound(v.first), std::move(v));
		}

		template<typename N>
		[[nodiscard]] auto
		lower_bound(const N & n) const
		{
			return std::lower_bound(begin(), end(), n, KeyComp<N> {});
		}

		template<typename N>
		[[nodiscard]] auto
		contains(const N & n) const
		{
			return std::binary_search(begin(), end(), n, KeyComp<N> {});
		}

		template<typename N>
		[[nodiscard]] auto
		find(const N & n) const
		{
			const auto lb = lower_bound(n);
			if (lb == end()) {
				return lb;
			}
			if (Comp {}(n, lb->first)) {
				return end();
			}
			return lb;
		}

		template<typename Ex = std::out_of_range, typename N>
		[[nodiscard]] const auto &
		at(const N & n) const
		{
			if (const auto i = find(n); i != end()) {
				return i->second;
			}
			if constexpr (std::is_constructible_v<Ex, N>) {
				// NOLINTNEXTLINE(hicpp-no-array-decay)
				throw Ex(n);
			}
			else {
				throw Ex(std::to_string(n));
			}
		}

		[[nodiscard]] auto
		begin() const
		{
			return cbegin();
		}

		[[nodiscard]] auto
		end() const
		{
			return cend();
		}

		using S::cbegin;
		using S::cend;
		using S::empty;
		using S::reserve;
		using S::size;
		using iterator = typename S::iterator;
		using const_iterator = typename S::const_iterator;
	};
}
