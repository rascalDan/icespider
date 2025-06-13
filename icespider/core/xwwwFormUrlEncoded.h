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

		explicit XWwwFormUrlEncoded(std::istream & input);

		void Deserialize(Slicer::ModelPartForRootParam modelPart) override;
		DLL_PUBLIC static void iterateVars(std::string_view input, const KVh & h, std::string_view split);

		DLL_PUBLIC static MaybeString urlDecode(std::string_view::const_iterator, std::string_view::const_iterator);
		DLL_PUBLIC static std::string urlencode(std::string_view::const_iterator, std::string_view::const_iterator);
		DLL_PUBLIC static void urlencodeto(
				std::ostream &, std::string_view::const_iterator, std::string_view::const_iterator);
		DLL_PUBLIC static std::string urlencode(std::string_view);

	private:
		static inline void iterateVars(
				const KVh & h, boost::algorithm::split_iterator<std::string_view::const_iterator> pi);

		void iterateVars(const KVh & h);

		void deserializeSimple(Slicer::ModelPartParam modelPart);
		void deserializeComplex(Slicer::ModelPartParam modelPart);
		void deserializeDictionary(Slicer::ModelPartParam modelPart);

		const std::string input;
	};

};
