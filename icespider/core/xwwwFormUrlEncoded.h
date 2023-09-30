#pragma once

#include <boost/algorithm/string/find_iterator.hpp>
#include <functional>
#include <iosfwd>
#include <maybeString.h>
#include <slicer/modelParts.h>
#include <slicer/serializer.h>
#include <string>
#include <string_view>
#include <visibility.h>

// IWYU pragma: no_forward_declare boost::algorithm::split_iterator

namespace IceSpider {
	class XWwwFormUrlEncoded : public Slicer::Deserializer {
	public:
		using KVh = std::function<void(MaybeString &&, MaybeString &&)>;

		explicit XWwwFormUrlEncoded(std::istream & in);

		void Deserialize(Slicer::ModelPartForRootParam mp) override;
		DLL_PUBLIC static void iterateVars(const std::string_view input, const KVh & h, const std::string_view split);

		DLL_PUBLIC static MaybeString urldecode(std::string_view::const_iterator s, std::string_view::const_iterator);
		DLL_PUBLIC static std::string urlencode(std::string_view::const_iterator s, std::string_view::const_iterator);
		DLL_PUBLIC static void urlencodeto(
				std::ostream &, std::string_view::const_iterator s, std::string_view::const_iterator);
		DLL_PUBLIC static std::string urlencode(const std::string_view s);

	private:
		static inline void iterateVars(
				const KVh & h, boost::algorithm::split_iterator<std::string_view::const_iterator> pi);

		void iterateVars(const KVh & h);

		void DeserializeSimple(const Slicer::ModelPartParam mp);
		void DeserializeComplex(const Slicer::ModelPartParam mp);
		void DeserializeDictionary(const Slicer::ModelPartParam mp);

		const std::string input;
	};

};
