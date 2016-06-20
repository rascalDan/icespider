#ifndef ICESPIDER_CORE_UTIL_H
#define ICESPIDER_CORE_UTIL_H

#include <IceUtil/Optional.h>

namespace IceUtil {
	template <typename T, typename TF>
	T operator/(const IceUtil::Optional<T> & o, const TF & tf)
	{
		if (o) return *o;
		return tf();
	}
}

#endif

