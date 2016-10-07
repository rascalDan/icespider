#include <slicer/slicer.h>
#include <exceptions.h>
#include <boost/algorithm/string/split.hpp>
#include <boost/lexical_cast.hpp>
#include <Ice/BuiltinSequences.h>

namespace ba = boost::algorithm;

extern long hextable[];

namespace IceSpider {

	class XWwwFormUrlEncoded : public Slicer::Deserializer {
		public:
			XWwwFormUrlEncoded(std::istream & in) :
				input(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>())
			{
			}

			void Deserialize(Slicer::ModelPartForRootPtr mp) override
			{
				mp->Create();
				mp->OnEachChild([this](auto, auto mp, auto) {
					switch (mp->GetType()) {
						case Slicer::mpt_Simple:
							this->DeserializeSimple(mp);
							break;
						case Slicer::mpt_Complex:
							this->DeserializeComplex(mp);
							break;
						case Slicer::mpt_Dictionary:
							this->DeserializeDictionary(mp);
							break;
						default:
							throw IceSpider::Http400_BadRequest();
							break;
					}
				});
				mp->Complete();
			}

		private:
			class SetFromString : public Slicer::ValueSource {
				public:
					SetFromString(const std::string & v) : s(v)
					{
					}

					void set(bool & t) const override
					{
						if (s == "true") t = true;
						else if (s == "false") t = false;
						else throw Http400_BadRequest();
					}

					void set(std::string & t) const override
					{
						t.reserve(s.size());
						for (auto i = s.begin(); i != s.end(); i++) {
							if (*i == '+') t += ' ';
							else if (*i == '%') {
								t += (16 * hextable[(int)*++i]) + hextable[(int)*++i];
							}
							else t += *i;
						}
					}

#define SET(T) \
					void set(T & t) const override \
					{ \
						t = boost::lexical_cast<T>(s); \
					}

					SET(Ice::Byte);
					SET(Ice::Short);
					SET(Ice::Int);
					SET(Ice::Long);
					SET(Ice::Float);
					SET(Ice::Double);
					const std::string & s;
			};

			typedef boost::function<void(const std::string &, const std::string &)> KVh;
			void iterateVars(const KVh & h)
			{
				for (auto pi = ba::make_split_iterator(input, ba::first_finder("&", ba::is_equal())); pi != decltype(pi)(); ++pi) {
					auto eq = std::find(pi->begin(), pi->end(), '=');
					if (eq == pi->end()) {
						h(std::string(pi->begin(), pi->end()), std::string());
					}
					else {
						h(std::string(pi->begin(), eq), std::string(eq + 1, pi->end()));
					}
				}
			}
			void DeserializeSimple(Slicer::ModelPartPtr mp)
			{
				iterateVars([mp](auto, auto v) {
					mp->SetValue(new SetFromString(v));
				});
			}
			void DeserializeComplex(Slicer::ModelPartPtr mp)
			{
				mp->Create();
				iterateVars([mp](auto k, auto v) {
					if (auto m = mp->GetChild(k)) {
						m->SetValue(new SetFromString(v));
					}
				});
				mp->Complete();
			}
			void DeserializeDictionary(Slicer::ModelPartPtr mp)
			{
				iterateVars([mp](auto k, auto v) {
					auto p = mp->GetAnonChild();
					p->GetChild("key")->SetValue(new SetFromString(k));
					p->GetChild("value")->SetValue(new SetFromString(v));
					p->Complete();
				});
			}
			const std::string input;
	};
}

NAMEDFACTORY("application/x-www-form-urlencoded", IceSpider::XWwwFormUrlEncoded, Slicer::StreamDeserializerFactory);

