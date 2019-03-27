#include "xwwwFormUrlEncoded.h"
#include "exceptions.h"
#include <boost/lexical_cast.hpp>
#include <Ice/BuiltinSequences.h>
#include <array>

namespace ba = boost::algorithm;
using namespace std::literals;

constexpr std::array<char, 255> hextable = []()
{
    std::array<char, 255> hextable{};
    for (int n = 0; n < 255; n++) {
        hextable[n] = -1;
    }
    for (int n = '0'; n <= '9'; n++) {
        hextable[n] = n - '0';
    }
    for (int n = 'a'; n <= 'f'; n++) {
        hextable[n] = hextable[n - 32] = n - 'a' + 10;
    }
    return hextable;
}();

static_assert(hextable['~'] == -1);
static_assert(hextable[' '] == -1);
static_assert(hextable['0'] == 0);
static_assert(hextable['9'] == 9);
static_assert(hextable['a'] == 10);
static_assert(hextable['A'] == 10);
static_assert(hextable['f'] == 15);
static_assert(hextable['F'] == 15);
static_assert(hextable['g'] == -1);
static_assert(hextable['G'] == -1);

namespace IceSpider {
	static const std::string AMP = "&";
	static const std::string TRUE = "true";
	static const std::string FALSE = "false";
	static const std::string KEY = "key";
	static const std::string VALUE = "value";

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
			explicit SetFromString(const std::string & v) : s(v)
			{
			}

			void set(bool & t) const override
			{
				if (s == TRUE) {
					t = true;
				}
				else if (s == FALSE) {
					t = false;
				}
				else {
					throw Http400_BadRequest();
				}
			}

			void set(std::string & t) const override
			{
				t = s;
			}

#define SET(T) \
			/* NOLINTNEXTLINE(bugprone-macro-parentheses) */ \
			void set(T & t) const override \
			{ \
				t = boost::lexical_cast<T>(s); \
			}

			SET(Ice::Byte);
			SET(Ice::Short);
			SET(Ice::Int);
			SET(Ice::Long);
			SET(Ice::Float);
			// NOLINTNEXTLINE(clang-analyzer-optin.cplusplus.VirtualCall)
			SET(Ice::Double);

		private:
			const std::string & s;
	};

	std::string
	XWwwFormUrlEncoded::urlencode(const std::string_view & s)
	{
		return urlencode(s.begin(), s.end());
	}

	inline auto hexchar(int c) { return c < 10 ? '0' + c : 'a' - 10 + c; }
	template<typename T>
	inline char hexlookup(const T & i) { return hextable[(int)*i]; }

	std::string
	XWwwFormUrlEncoded::urlencode(std::string_view::const_iterator i, std::string_view::const_iterator e)
	{
		std::stringstream o;
		urlencodeto(o, i, e);
		return o.str();
	}

	void
	XWwwFormUrlEncoded::urlencodeto(std::ostream & o, std::string_view::const_iterator i, std::string_view::const_iterator e)
	{
		while (i != e) {
			if (*i == ' ') {
				o.put('+');
			}
			else if (!isalnum(*i)) {
				o.put('%');
				o.put(hexchar(*i >> 4)); // NOLINT(hicpp-signed-bitwise)
				o.put(hexchar(*i & 0xf)); // NOLINT(hicpp-signed-bitwise)
			}
			else {
				o.put(*i);
			}
			++i;
		}
	}

	std::string
	XWwwFormUrlEncoded::urldecode(std::string_view::const_iterator i, std::string_view::const_iterator e)
	{
		std::string t;
		t.reserve(std::distance(i, e));
		while (i != e) {
			switch (*i) {
				case '+':
					t += ' ';
					break;
				case '%':
					t += static_cast<char>((16 * hexlookup(i + 1)) + hexlookup(i + 2));
					i += 2;
					break;
				default:
					t += *i;
			}
			++i;
		}
		return t;
	}

	void
	XWwwFormUrlEncoded::iterateVars(const KVh & h, ba::split_iterator<std::string_view::const_iterator> pi)
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
	XWwwFormUrlEncoded::iterateVars(const std::string_view & input, const KVh & h, const std::string_view & split)
	{
		if (!input.empty()) {
			iterateVars(h, ba::make_split_iterator(input, ba::first_finder(split, ba::is_equal())));
		}
	}

	void
	XWwwFormUrlEncoded::iterateVars(const KVh & h)
	{
		iterateVars(input, h, AMP);
	}

	void
	XWwwFormUrlEncoded::DeserializeSimple(const Slicer::ModelPartPtr & mp)
	{
		iterateVars([mp](auto &&, const auto && v) {
			mp->SetValue(SetFromString(v));
		});
	}

	void
	XWwwFormUrlEncoded::DeserializeComplex(const Slicer::ModelPartPtr & mp)
	{
		mp->Create();
		iterateVars([mp](auto && k, const auto && v) {
			if (auto m = mp->GetChild(k)) {
				m->SetValue(SetFromString(v));
			}
		});
		mp->Complete();
	}

	void
	XWwwFormUrlEncoded::DeserializeDictionary(const Slicer::ModelPartPtr & mp)
	{
		iterateVars([mp](auto && k, const auto && v) {
			auto p = mp->GetAnonChild();
			p->GetChild(KEY)->SetValue(SetFromString(k));
			p->GetChild(VALUE)->SetValue(SetFromString(v));
			p->Complete();
		});
	}
}

NAMEDFACTORY("application/x-www-form-urlencoded", IceSpider::XWwwFormUrlEncoded, Slicer::StreamDeserializerFactory);

