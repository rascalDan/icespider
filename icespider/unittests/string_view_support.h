#ifndef ICETRAY_STRING_VIEW_SUPPORT_H
#define ICETRAY_STRING_VIEW_SUPPORT_H

#include <Ice/OutputStream.h>
#include <Ice/StreamHelpers.h>
#include <string_view>

namespace Ice {
	template<> struct StreamableTraits<std::string_view> {
		static const StreamHelperCategory helper = StreamHelperCategoryBuiltin;
		static const int minWireSize = 1;
		static const bool fixedLength = false;
	};

	template<> struct StreamHelper<std::string_view, StreamHelperCategoryBuiltin> {
		template<class S>
		static inline void
		write(S * stream, const std::string_view & v)
		{
			stream->write(v.data(), v.size());
		}

		template<class S>
		static inline void
		read(S * stream, std::string_view & v)
		{
			const char * vdata = nullptr;
			size_t vsize = 0;

			stream->read(vdata, vsize);

			if (vsize > 0) {
				v = std::string_view(vdata, vsize);
			}
			else {
				v = {};
			}
		}
	};
}

#endif
