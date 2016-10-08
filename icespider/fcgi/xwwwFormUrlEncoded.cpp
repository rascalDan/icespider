#include "xwwwFormUrlEncoded.h"
#include <exceptions.h>
#include <boost/lexical_cast.hpp>
#include <Ice/BuiltinSequences.h>

namespace ba = boost::algorithm;

extern long hextable[];

namespace IceSpider {

	XWwwFormUrlEncoded::XWwwFormUrlEncoded(std::istream & in) :
		input(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>())
	{
	}

	void
	XWwwFormUrlEncoded::Deserialize(Slicer::ModelPartForRootPtr mp)
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
				t = s;
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

	std::string
	XWwwFormUrlEncoded::urldecode(std::string::const_iterator i, std::string::const_iterator e)
	{
		std::string t;
		t.reserve(&*e - &*i);
		while (i != e) {
			if (*i == '+') t += ' ';
			else if (*i == '%') {
				t += (16 * hextable[(int)*++i]) + hextable[(int)*++i];
			}
			else t += *i;
			++i;
		}
		return t;
	}

	void
	XWwwFormUrlEncoded::iterateVars(const KVh & h, ba::split_iterator<std::string::const_iterator> pi)
	{
		for (; pi != decltype(pi)(); ++pi) {
			auto eq = std::find(pi->begin(), pi->end(), '=');
			if (eq == pi->end()) {
				h(urldecode(pi->begin(), pi->end()), std::string());
			}
			else {
				h(urldecode(pi->begin(), eq), urldecode(eq + 1, pi->end()));
			}
		}
	}

	void
	XWwwFormUrlEncoded::iterateVars(const std::string & input, const KVh & h)
	{
		if (!input.empty()) {
			iterateVars(h, ba::make_split_iterator(input, ba::first_finder("&", ba::is_equal())));
		}
	}

	void
	XWwwFormUrlEncoded::iterateVars(const KVh & h)
	{
		iterateVars(input, h);
	}

	void
	XWwwFormUrlEncoded::DeserializeSimple(Slicer::ModelPartPtr mp)
	{
		iterateVars([mp](auto, auto v) {
			mp->SetValue(new SetFromString(v));
		});
	}

	void
	XWwwFormUrlEncoded::DeserializeComplex(Slicer::ModelPartPtr mp)
	{
		mp->Create();
		iterateVars([mp](auto k, auto v) {
			if (auto m = mp->GetChild(k)) {
				m->SetValue(new SetFromString(v));
			}
		});
		mp->Complete();
	}

	void
	XWwwFormUrlEncoded::DeserializeDictionary(Slicer::ModelPartPtr mp)
	{
		iterateVars([mp](auto k, auto v) {
			auto p = mp->GetAnonChild();
			p->GetChild("key")->SetValue(new SetFromString(k));
			p->GetChild("value")->SetValue(new SetFromString(v));
			p->Complete();
		});
	}
}

NAMEDFACTORY("application/x-www-form-urlencoded", IceSpider::XWwwFormUrlEncoded, Slicer::StreamDeserializerFactory);

