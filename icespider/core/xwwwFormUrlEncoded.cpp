#include "xwwwFormUrlEncoded.h"
#include "exceptions.h"
#include "util.h"
#include <Ice/Config.h>
#include <algorithm>
#include <array>
#include <boost/algorithm/string/compare.hpp>
#include <boost/algorithm/string/finder.hpp>
#include <cstddef>
#include <cstdint>
#include <factory.h>
#include <istream>
#include <iterator>
#include <limits>
#include <maybeString.h>
#include <optional>
#include <slicer/modelParts.h>
#include <slicer/serializer.h>
#include <utility>

namespace ba = boost::algorithm;
using namespace std::literals;

namespace {
	constexpr auto CHARMAX = std::numeric_limits<unsigned char>::max();

	// NOLINTBEGIN(readability-magic-numbers)
	using HexPair = std::pair<char, char>;
	using HexOut = std::array<HexPair, CHARMAX>;
	constexpr auto HEXOUT = []() {
		auto hexchar = [](auto chr) {
			return static_cast<char>(chr < 10 ? '0' + chr : 'a' - 10 + chr);
		};
		HexOut out {};
		for (unsigned int chr = 0; chr < CHARMAX; chr++) {
			switch (chr) {
				case ' ':
					out[chr].first = '+';
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
					out[chr].first = hexchar(chr >> 4U);
					out[chr].second = hexchar(chr & 0xfU);
					break;
			}
		}
		return out;
	}();

	static_assert(HEXOUT['a'].first == 0);
	static_assert(HEXOUT['a'].second == 0);
	static_assert(HEXOUT['z'].first == 0);
	static_assert(HEXOUT['z'].second == 0);
	static_assert(HEXOUT['A'].first == 0);
	static_assert(HEXOUT['A'].second == 0);
	static_assert(HEXOUT['Z'].first == 0);
	static_assert(HEXOUT['Z'].second == 0);
	static_assert(HEXOUT['0'].first == 0);
	static_assert(HEXOUT['9'].second == 0);
	static_assert(HEXOUT['~'].first == 0);
	static_assert(HEXOUT['~'].second == 0);
	static_assert(HEXOUT[' '].first == '+');
	static_assert(HEXOUT[' '].second == 0);
	static_assert(HEXOUT['?'].first == '3');
	static_assert(HEXOUT['?'].second == 'f');

	constexpr std::array<std::optional<unsigned char>, CHARMAX> HEXTABLE = []() {
		std::array<std::optional<unsigned char>, CHARMAX> hextable {};
		for (size_t chr = '0'; chr <= '9'; chr++) {
			hextable[chr] = {chr - '0'};
		}
		for (size_t chr = 'a'; chr <= 'f'; chr++) {
			hextable[chr] = hextable[chr - 32] = {chr - 'a' + 10};
		}
		return hextable;
	}();

	static_assert(!HEXTABLE['~'].has_value());
	static_assert(!HEXTABLE[' '].has_value());
	static_assert(HEXTABLE['0'] == 0);
	static_assert(HEXTABLE['9'] == 9);
	static_assert(HEXTABLE['a'] == 10);
	static_assert(HEXTABLE['A'] == 10);
	static_assert(HEXTABLE['f'] == 15);
	static_assert(HEXTABLE['F'] == 15);
	static_assert(!HEXTABLE['g'].has_value());
	static_assert(!HEXTABLE['G'].has_value());
	// NOLINTEND(readability-magic-numbers)

	using HexIn = std::array<std::array<char, CHARMAX>, CHARMAX>;
	constexpr HexIn HEXIN = []() {
		HexIn hexin {};
		size_t firstHex = std::min({'0', 'a', 'A'});
		size_t lastHex = std::max({'9', 'f', 'F'});
		for (auto first = firstHex; first <= lastHex; first++) {
			if (const auto & ch1 = HEXTABLE[first]) {
				for (auto second = firstHex; second <= lastHex; second++) {
					if (const auto & ch2 = HEXTABLE[second]) {
						hexin[first][second] = static_cast<char>((*ch1 << 4U) + *ch2);
					}
				}
			}
		}
		hexin['+'][0] = ' ';
		return hexin;
	}();

	static_assert(HEXIN[' '][' '] == 0);
	static_assert(HEXIN['0']['a'] == '\n');
	static_assert(HEXIN['+'][0] == ' ');
	static_assert(HEXIN['3']['f'] == '?');
	static_assert(HEXIN['3']['F'] == '?');

	constexpr auto
	urlEncodeRange(auto && outIter, auto input, auto end)
	{
		while (input != end) {
			const auto & out = HEXOUT[static_cast<uint8_t>(*input)];
			if (out.second) {
				outIter = '%';
				outIter = out.first;
				outIter = out.second;
			}
			else if (out.first) {
				outIter = (out.first);
			}
			else {
				outIter = (*input);
			}
			++input;
		}
	}
}

namespace IceSpider {
	static constexpr const std::string_view AMP = "&";
	static constexpr const std::string_view TRUE = "true";
	static constexpr const std::string_view FALSE = "false";
	static constexpr const std::string_view KEY = "key";
	static constexpr const std::string_view VALUE = "value";
	static constexpr const std::string_view URL_ESCAPES = "%+";

	XWwwFormUrlEncoded::XWwwFormUrlEncoded(std::istream & input) :
		input(std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>())
	{
	}

	void
	XWwwFormUrlEncoded::Deserialize(Slicer::ModelPartForRootParam modelPart)
	{
		modelPart->Create();
		modelPart->OnEachChild([this](const auto &, auto child, auto) {
			switch (child->GetType()) {
				case Slicer::ModelPartType::Simple:
					this->deserializeSimple(child);
					break;
				case Slicer::ModelPartType::Complex:
					this->deserializeComplex(child);
					break;
				case Slicer::ModelPartType::Dictionary:
					this->deserializeDictionary(child);
					break;
				default:
					throw IceSpider::Http400BadRequest();
					break;
			}
		});
		modelPart->Complete();
	}

	class SetFromString : public Slicer::ValueSource {
	public:
		explicit SetFromString(const MaybeString & value) : s(value) { }

		void
		set(bool & target) const override
		{
			if (s == TRUE) {
				target = true;
			}
			else if (s == FALSE) {
				target = false;
			}
			else {
				throw Http400BadRequest();
			}
		}

		void
		set(std::string & target) const override
		{
			target = s;
		}

#define SET(T) \
	/* NOLINTNEXTLINE(bugprone-macro-parentheses) */ \
	void set(T & target) const override \
	{ \
		convert(s, target); \
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
	XWwwFormUrlEncoded::urlencode(const std::string_view str)
	{
		return urlencode(str.begin(), str.end());
	}

	std::string
	XWwwFormUrlEncoded::urlencode(std::string_view::const_iterator input, std::string_view::const_iterator end)
	{
		std::string out;
		out.reserve(static_cast<std::string::size_type>(std::distance(input, end)));
		urlEncodeRange(std::back_inserter(out), input, end);
		return out;
	}

	void
	XWwwFormUrlEncoded::urlencodeto(
			std::ostream & outStrm, std::string_view::const_iterator input, std::string_view::const_iterator end)
	{
		urlEncodeRange(std::ostream_iterator<decltype(*input)>(outStrm), input, end);
	}

	MaybeString
	XWwwFormUrlEncoded::urlDecode(std::string_view::const_iterator input, std::string_view::const_iterator end)
	{
		const auto getNext = [end, &input] {
			return std::find_first_of(input, end, URL_ESCAPES.begin(), URL_ESCAPES.end());
		};
		auto next = getNext();
		if (next == end) {
			return std::string_view {input, end};
		}
		std::string target;
		target.reserve(static_cast<std::string::size_type>(std::distance(input, end)));
		for (; input != end; next = getNext()) {
			if (next != input) {
				target.append(input, next);
				input = next;
			}
			else {
				switch (*input) {
					case '+':
						target += ' ';
						++input;
						break;
					case '%':
						if (const auto chr
								= HEXIN[static_cast<uint8_t>(*(input + 1))][static_cast<uint8_t>(*(input + 2))]) {
							target += chr;
						}
						else {
							throw Http400BadRequest();
						}
						input += 3;
						break;
					default:
						std::unreachable();
				}
			}
		}
		return target;
	}

	void
	XWwwFormUrlEncoded::iterateVars(const KVh & handler, ba::split_iterator<std::string_view::const_iterator> pi)
	{
		for (; pi != decltype(pi)(); ++pi) {
			auto equalPos = std::find(pi->begin(), pi->end(), '=');
			if (equalPos == pi->end()) {
				handler(urlDecode(pi->begin(), pi->end()), {});
			}
			else {
				handler(urlDecode(pi->begin(), equalPos), urlDecode(equalPos + 1, pi->end()));
			}
		}
	}

	void
	XWwwFormUrlEncoded::iterateVars(const std::string_view input, const KVh & handler, const std::string_view split)
	{
		if (!input.empty()) {
			iterateVars(handler, ba::make_split_iterator(input, ba::first_finder(split, ba::is_equal())));
		}
	}

	void
	XWwwFormUrlEncoded::iterateVars(const KVh & handler)
	{
		iterateVars(input, handler, AMP);
	}

	void
	XWwwFormUrlEncoded::deserializeSimple(const Slicer::ModelPartParam modelPart)
	{
		iterateVars([modelPart](auto &&, const auto && value) {
			modelPart->SetValue(SetFromString(std::forward<decltype(value)>(value)));
		});
	}

	void
	XWwwFormUrlEncoded::deserializeComplex(const Slicer::ModelPartParam modelPart)
	{
		modelPart->Create();
		iterateVars([modelPart](auto && key, const auto && value) {
			modelPart->OnChild(
					[&value](Slicer::ModelPartParam child, const Slicer::Metadata &) {
						child->SetValue(SetFromString(std::forward<decltype(value)>(value)));
					},
					key);
		});
		modelPart->Complete();
	}

	void
	XWwwFormUrlEncoded::deserializeDictionary(const Slicer::ModelPartParam modelPart)
	{
		iterateVars([modelPart](auto && key, const auto && value) {
			modelPart->OnAnonChild([&key, &value](Slicer::ModelPartParam child, const Slicer::Metadata &) {
				child->OnChild(
						[&key](Slicer::ModelPartParam keyPart, const Slicer::Metadata &) {
							keyPart->SetValue(SetFromString(std::forward<decltype(key)>(key)));
						},
						KEY);
				child->OnChild(
						[&value](Slicer::ModelPartParam valuePart, const Slicer::Metadata &) {
							valuePart->SetValue(SetFromString(std::forward<decltype(value)>(value)));
						},
						VALUE);
				child->Complete();
			});
		});
	}
}

NAMEDFACTORY("application/x-www-form-urlencoded", IceSpider::XWwwFormUrlEncoded, Slicer::StreamDeserializerFactory);
