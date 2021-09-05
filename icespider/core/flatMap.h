#ifndef ICESPIDER_CORE_FLATMAP_H
#define ICESPIDER_CORE_FLATMAP_H

#include <utility>
#include <vector>

namespace IceSpider {
	template<typename K, typename M, typename Comp = std::less<>>
	class flatmap : std::vector<std::pair<std::decay_t<K>, std::decay_t<M>>> {
	public:
		using V = std::pair<std::decay_t<K>, std::decay_t<M>>;
		using S = std::vector<V>;

	private:
		template<typename N> struct KeyComp {
			bool
			operator()(const V & v, const N & n) const
			{
				return c(v.first, n);
			}
			Comp c;
		};

	public:
		flatmap() = default;
		flatmap(std::size_t n)
		{
			reserve(n);
		}

		auto
		insert(V v)
		{
			return S::insert(lower_bound(v.first), std::move(v));
		}

		template<typename N>
		auto
		lower_bound(const N & n) const
		{
			return std::lower_bound(begin(), end(), n, KeyComp<N> {});
		}

		template<typename N>
		auto
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

		using S::begin;
		using S::empty;
		using S::end;
		using S::reserve;
		using S::size;
		using iterator = typename S::iterator;
		using const_iterator = typename S::const_iterator;
	};
}

#endif
