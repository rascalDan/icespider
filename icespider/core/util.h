#ifndef ICESPIDER_CORE_UTIL_H
#define ICESPIDER_CORE_UTIL_H

#include <IceUtil/Exception.h>
#include <IceUtil/Optional.h>

namespace std::experimental::Ice {
	template <typename T, typename TF>
	auto operator/(const Ice::optional<T> & o, const TF & tf) -> decltype(tf())
	{
		if (o) return *o;
		return tf();
	}
}

template <typename T>
T orelse(const T & a, const T & b)
{
	if (a) return a;
	return b;
}


#endif

