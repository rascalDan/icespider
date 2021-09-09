#include "xwwwFormUrlEncoded.h"
#include "exceptions.h"
#include <Ice/BuiltinSequences.h>
#include <array>
#include <boost/lexical_cast.hpp>

namespace ba = boost::algorithm;
using namespace std::literals;

constexpr auto CHARMAX = std::numeric_limits<unsigned char>::max();

using HexPair = std::pair<char, char>;
using HexOut = std::array<HexPair, CHARMAX>;
constexpr auto hexout = []() {
	auto hexchar = [](auto c) {
		return static_cast<char>(c < 10 ? '0' + c : 'a' - 10 + c);
	};
	HexOut out {};
	for (unsigned int n = 0; n < CHARMAX; n++) {
		switch (n) {
			case ' ':
				out[n].first = '+';
				break;
			case 'a' ... 'z':
			case 'A' ... 'Z':
			case '0' ... '9':
			case '-':
			case '.':
			case '_':
			case '~':
				// No special encoding
				break;
			default:
				out[n].first = hexchar(n >> 4U);
				out[n].second = hexchar(n & 0xfU);
				break;
		}
	}
	return out;
}();

static_assert(hexout['a'].first == 0);
static_assert(hexout['a'].second == 0);
static_assert(hexout['z'].first == 0);
static_assert(hexout['z'].second == 0);
static_assert(hexout['A'].first == 0);
static_assert(hexout['A'].second == 0);
static_assert(hexout['Z'].first == 0);
static_assert(hexout['Z'].second == 0);
static_assert(hexout['0'].first == 0);
static_assert(hexout['9'].second == 0);
static_assert(hexout['~'].first == 0);
static_assert(hexout['~'].second == 0);
static_assert(hexout[' '].first == '+');
static_assert(hexout[' '].second == 0);
static_assert(hexout['?'].first == '3');
static_assert(hexout['?'].second == 'f');

constexpr std::array<std::optional<unsigned char>, CHARMAX> hextable = []() {
	std::array<std::optional<unsigned char>, CHARMAX> hextable {};
	for (size_t n = '0'; n <= '9'; n++) {
		hextable[n] = {n - '0'};
	}
	for (size_t n = 'a'; n <= 'f'; n++) {
		hextable[n] = hextable[n - 32] = {n - 'a' + 10};
	}
	return hextable;
}();

static_assert(!hextable['~'].has_value());
static_assert(!hextable[' '].has_value());
static_assert(hextable['0'] == 0);
static_assert(hextable['9'] == 9);
static_assert(hextable['a'] == 10);
static_assert(hextable['A'] == 10);
static_assert(hextable['f'] == 15);
static_assert(hextable['F'] == 15);
static_assert(!hextable['g'].has_value());
static_assert(!hextable['G'].has_value());

using HexIn = std::array<std::array<char, CHARMAX>, CHARMAX>;
constexpr HexIn hexin = []() {
	HexIn hexin {};
	size_t firstHex = std::min({'0', 'a', 'A'});
	size_t lastHex = std::max({'9', 'f', 'F'});
	for (auto first = firstHex; first <= lastHex; first++) {
		if (const auto ch1 = hextable[first]) {
			for (auto second = firstHex; second <= lastHex; second++) {
				if (const auto ch2 = hextable[second]) {
					hexin[first][second] = static_cast<char>((*ch1 << 4U) + *ch2);
				}
			}
		}
	}
	hexin['+'][0] = ' ';
	return hexin;
}();

static_assert(hexin[' '][' '] == 0);
static_assert(hexin['0']['a'] == '\n');
static_assert(hexin['+'][0] == ' ');
static_assert(hexin['3']['f'] == '?');
static_assert(hexin['3']['F'] == '?');

namespace IceSpider {
	static constexpr const std::string_view AMP = "&";
	static constexpr const std::string_view TRUE = "true";
	static constexpr const std::string_view FALSE = "false";
	static constexpr const std::string_view KEY = "key";
	static constexpr const std::string_view VALUE = "value";
	static constexpr const std::string_view URL_ESCAPES = "%+";

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
				case Slicer::ModelPartType::Simple:
					this->DeserializeSimple(mp);
					break;
				case Slicer::ModelPartType::Complex:
					this->DeserializeComplex(mp);
					break;
				case Slicer::ModelPartType::Dictionary:
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
		explicit SetFromString(const MaybeString & v) : s(v) { }

		void
		set(bool & t) const override
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

		void
		set(std::string & t) const override
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
		SET(Ice::Double);

	private:
		const MaybeString & s;
	};

	std::string
	XWwwFormUrlEncoded::urlencode(const std::string_view & s)
	{
		return urlencode(s.begin(), s.end());
	}

	constexpr auto urlencoderange = [](auto && o, auto i, auto e) {
		while (i != e) {
			const auto & out = hexout[static_cast<uint8_t>(*i)];
			if (out.second) {
				o = '%';
				o = out.first;
				o = out.second;
			}
			else if (out.first) {
				o = (out.first);
			}
			else {
				o = (*i);
			}
			++i;
		}
	};

	std::string
	XWwwFormUrlEncoded::urlencode(std::string_view::const_iterator i, std::string_view::const_iterator e)
	{
		std::string o;
		o.reserve(static_cast<std::string::size_type>(std::distance(i, e)));
		urlencoderange(std::back_inserter(o), i, e);
		return o;
	}

	void
	XWwwFormUrlEncoded::urlencodeto(
			std::ostream & o, std::string_view::const_iterator i, std::string_view::const_iterator e)
	{
		urlencoderange(std::ostream_iterator<decltype(*i)>(o), i, e);
	}

	MaybeString
	XWwwFormUrlEncoded::urldecode(std::string_view::const_iterator i, std::string_view::const_iterator e)
	{
		if (std::find_first_of(i, e, URL_ESCAPES.begin(), URL_ESCAPES.end()) == e) {
			return std::string_view {i, e};
		}
		std::string t;
		t.reserve(static_cast<std::string::size_type>(std::distance(i, e)));
		while (i != e) {
			switch (*i) {
				case '+':
					t += ' ';
					break;
				case '%':
					if (const auto ch = hexin[static_cast<uint8_t>(*(i + 1))][static_cast<uint8_t>(*(i + 2))]) {
						t += ch;
					}
					else {
						throw Http400_BadRequest();
					}
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
				h(urldecode(pi->begin(), pi->end()), {});
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
			mp->SetValue(SetFromString(std::forward<decltype(v)>(v)));
		});
	}

	void
	XWwwFormUrlEncoded::DeserializeComplex(const Slicer::ModelPartPtr & mp)
	{
		mp->Create();
		iterateVars([mp](auto && k, const auto && v) {
			if (auto m = mp->GetChild(k)) {
				m->SetValue(SetFromString(std::forward<decltype(v)>(v)));
			}
		});
		mp->Complete();
	}

	void
	XWwwFormUrlEncoded::DeserializeDictionary(const Slicer::ModelPartPtr & mp)
	{
		iterateVars([mp](auto && k, const auto && v) {
			auto p = mp->GetAnonChild();
			p->GetChild(KEY)->SetValue(SetFromString(std::forward<decltype(k)>(k)));
			p->GetChild(VALUE)->SetValue(SetFromString(std::forward<decltype(v)>(v)));
			p->Complete();
		});
	}
}

NAMEDFACTORY("application/x-www-form-urlencoded", IceSpider::XWwwFormUrlEncoded, Slicer::StreamDeserializerFactory);
