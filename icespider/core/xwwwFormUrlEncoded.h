#ifndef ICESPIDER_CGI_XWWWFORMURLENCODED_H
#define ICESPIDER_CGI_XWWWFORMURLENCODED_H

#include <visibility.h>
#include <slicer/serializer.h>
#include <boost/algorithm/string/split.hpp>

namespace IceSpider {
	class XWwwFormUrlEncoded : public Slicer::Deserializer {
		public:
			typedef boost::function<void(const std::string &, const std::string &)> KVh;

			XWwwFormUrlEncoded(std::istream & in);

			void Deserialize(Slicer::ModelPartForRootPtr mp) override;
			DLL_PUBLIC static void iterateVars(const std::string_view & input, const KVh & h, const std::string_view & split);

			DLL_PUBLIC static std::string urldecode(std::string_view::const_iterator s, std::string_view::const_iterator);
			DLL_PUBLIC static std::string urlencode(std::string_view::const_iterator s, std::string_view::const_iterator);
			DLL_PUBLIC static void urlencodeto(std::ostream &, std::string_view::const_iterator s, std::string_view::const_iterator);
			DLL_PUBLIC static std::string urlencode(const std::string_view & s);

		private:
			static inline void iterateVars(const KVh & h, boost::algorithm::split_iterator<std::string_view::const_iterator> pi);

			void iterateVars(const KVh & h);

			void DeserializeSimple(Slicer::ModelPartPtr mp);
			void DeserializeComplex(Slicer::ModelPartPtr mp);
			void DeserializeDictionary(Slicer::ModelPartPtr mp);

			const std::string input;
	};

};

#endif

