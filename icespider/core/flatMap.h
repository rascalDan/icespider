#pragma once

#include <algorithm>
#include <cstddef>
#include <functional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace IceSpider {
	template<typename K, typename M, typename Comp = std::less<>> class FlatMap : std::vector<std::pair<K, M>> {
	public:
		using V = std::pair<K, M>;
		using S = std::vector<V>;

	private:
		template<typename N> struct KeyComp {
			bool
			operator()(const V & value, const N & name) const
			{
				return comparator(value.first, name);
			}

			bool
			operator()(const N & name, const V & value) const
			{
				return comparator(name, value.first);
			}

			Comp comparator;
		};

	public:
		FlatMap() = default;

		explicit FlatMap(std::size_t n)
		{
			reserve(n);
		}

		auto
		insert(V value)
		{
			return S::insert(lower_bound(value.first), std::move(value));
		}

		auto
		emplace(K key, M mapped)
		{
			auto pos = lower_bound(key);
			return S::emplace(pos, std::move(key), std::move(mapped));
		}

		template<typename N>
		[[nodiscard]] auto
		lower_bound(const N & n) const // NOLINT(readability-identifier-naming) - STL like
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
			const auto lower = lower_bound(n);
			if (lower == end()) {
				return lower;
			}
			if (Comp {}(n, lower->first)) {
				return end();
			}
			return lower;
		}

		template<typename Ex = std::out_of_range, typename N>
		[[nodiscard]] const auto &
		at(const N & n) const
		{
			if (const auto iter = find(n); iter != end()) {
				return iter->second;
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
		using iterator = typename S::iterator; // NOLINT(readability-identifier-naming) - STL like
		using const_iterator = typename S::const_iterator; // NOLINT(readability-identifier-naming) - STL like
	};
}
