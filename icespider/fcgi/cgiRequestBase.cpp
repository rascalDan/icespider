#include "cgiRequestBase.h"
#include "xwwwFormUrlEncoded.h"
#include <boost/algorithm/string/case_conv.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <util.h>
#include <slicer/modelPartsTypes.h>
#include <slicer/common.h>
#include <formatters.h>

namespace ba = boost::algorithm;
using namespace std::literals;

#define CGI_CONST(NAME) static const std::string_view NAME(#NAME)

namespace IceSpider {
	static const std::string_view amp("&");
	static const std::string_view semi("; ");
	static const std::string_view HEADER_PREFIX("HTTP_");
	CGI_CONST(REDIRECT_URL);
	CGI_CONST(SCRIPT_NAME);
	CGI_CONST(QUERY_STRING);
	CGI_CONST(HTTP_COOKIE);
	CGI_CONST(REQUEST_METHOD);

	CgiRequestBase::CgiRequestBase(Core * c, const char * const * const env) :
		IHttpRequest(c)
	{
		for(const char * const * e = env; *e; ++e) {
			addenv(*e);
		}
	}

	void
	CgiRequestBase::addenv(const char * const e)
	{
		if (auto eq = strchr(e, '=')) {
			envmap.insert({ std::string_view(e, eq - e), eq + 1 });
		}
	}

	template<typename in, typename out>
	inline
	void
	mapVars(const std::string_view & vn, const in & envmap, out & map, const std::string_view & sp) {
		auto qs = envmap.find(vn);
		if (qs != envmap.end()) {
			XWwwFormUrlEncoded::iterateVars(qs->second, [&map](const auto && k, const auto && v) {
				map.emplace(std::move(k), std::move(v));
			}, sp);
		}
	}

	template<typename Ex, typename Map>
	const std::string_view &
	findFirstOrElse(const Map &)
	{
		throw Ex();
	}

	template<typename Ex, typename Map, typename ... Ks>
	const std::string_view &
	findFirstOrElse(const Map & map, const std::string_view & k, const Ks & ... ks)
	{
		auto i = map.find(k);
		if (i == map.end()) {
			return findFirstOrElse<Ex>(map, ks...);
		}
		return i->second;
	}

	bool CgiRequestBase::ciLess::operator() (const std::string_view & s1, const std::string_view & s2) const
	{
		return lexicographical_compare(s1, s2, ba::is_iless());
	}

	void
	CgiRequestBase::initialize()
	{
		namespace ba = boost::algorithm;
		if (auto path = findFirstOrElse<Http400_BadRequest>(envmap, REDIRECT_URL, SCRIPT_NAME).substr(1);
				// NOLINTNEXTLINE(clang-analyzer-cplusplus.NewDeleteLeaks)
				!path.empty()) {
			ba::split(pathElements, path, ba::is_any_of("/"), ba::token_compress_off);
		}

		mapVars(QUERY_STRING, envmap, qsmap, amp);
		mapVars(HTTP_COOKIE, envmap, cookiemap, semi);
		for (auto header = envmap.lower_bound(HEADER_PREFIX);
				header != envmap.end() && ba::starts_with(header->first, HEADER_PREFIX);
				header++) {
			hdrmap.insert({ header->first.substr(HEADER_PREFIX.length()), header->second });
		}
	}

	AdHocFormatter(VarFmt, "\t%?: [%?]\n");
	AdHocFormatter(PathFmt, "\t[%?]\n");
	template<typename Fmt, typename Map>
	void
	dumpMap(std::ostream & s, const std::string_view & n, const Map & map)
	{
		s << n << std::endl;
		for (const auto & p : map) {
			Fmt::write(s, p.first, p.second);
		}
	}

	std::ostream &
	CgiRequestBase::dump(std::ostream & s) const
	{
		dumpMap<VarFmt>(s, "Environment dump"sv, envmap);
		s << "Path dump" << std::endl;
		for (const auto & e : pathElements) {
			PathFmt::write(s, e);
		}
		dumpMap<VarFmt>(s, "Query string dump"sv, qsmap);
		dumpMap<VarFmt>(s, "Cookie dump"sv, cookiemap);
		return s;
	}

	template<typename MapType>
	OptionalString
	CgiRequestBase::optionalLookup(const std::string_view & key, const MapType & vm)
	{
		auto i = vm.find(key);
		if (i == vm.end()) {
			return {};
		}
		return i->second;
	}

	const PathElements &
	CgiRequestBase::getRequestPath() const
	{
		return pathElements;
	}

	PathElements &
	CgiRequestBase::getRequestPath()
	{
		return pathElements;
	}

	HttpMethod
	CgiRequestBase::getRequestMethod() const
	{
		try {
			auto i = envmap.find(REQUEST_METHOD);
			if (i == envmap.end()) {
				throw IceSpider::Http400_BadRequest();
			}
			return Slicer::ModelPartForEnum<HttpMethod>::lookup(i->second);
		}
		catch (const Slicer::InvalidEnumerationSymbol &) {
			throw IceSpider::Http405_MethodNotAllowed();
		}
	}

	OptionalString
	CgiRequestBase::getQueryStringParam(const std::string_view & key) const
	{
		return optionalLookup(key, qsmap);
	}

	OptionalString
	CgiRequestBase::getCookieParam(const std::string_view & key) const
	{
		return optionalLookup(key, cookiemap);
	}

	OptionalString
	CgiRequestBase::getEnv(const std::string_view & key) const
	{
		return optionalLookup(key, envmap);
	}

	OptionalString
	CgiRequestBase::getHeaderParam(const std::string_view & key) const
	{
		return optionalLookup(key, hdrmap);
	}

	void CgiRequestBase::response(short statusCode, const std::string_view & statusMsg) const
	{
		StatusFmt::write(getOutputStream(), statusCode, statusMsg);
	}

	void
	CgiRequestBase::setHeader(const std::string_view & header, const std::string_view & value) const
	{
		HdrFmt::write(getOutputStream(), header, value);
	}
}

