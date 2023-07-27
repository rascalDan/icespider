#include "cgiRequestBase.h"
#include "xwwwFormUrlEncoded.h"
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/constants.hpp>
#include <boost/algorithm/string/find_iterator.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/split.hpp>
#include <compileTimeFormatter.h>
#include <exceptions.h>
#include <flatMap.h>
#include <formatters.h>
#include <ihttpRequest.h>
#include <maybeString.h>
#include <slicer/common.h>
#include <slicer/modelPartsTypes.h>
#include <utility>
#include <vector>

namespace ba = boost::algorithm;
using namespace std::literals;

#define CGI_CONST(NAME) static constexpr std::string_view NAME(#NAME)

namespace IceSpider {
	static const auto slash_pred = boost::algorithm::is_any_of("/");
	constexpr std::string_view amp("&");
	constexpr std::string_view semi("; ");
	constexpr std::string_view HEADER_PREFIX("HTTP_");
	CGI_CONST(HTTPS);
	CGI_CONST(REDIRECT_URL);
	CGI_CONST(SCRIPT_NAME);
	CGI_CONST(QUERY_STRING);
	CGI_CONST(HTTP_COOKIE);
	CGI_CONST(REQUEST_METHOD);

	template<typename in, typename out>
	inline void
	mapVars(const std::string_view & vn, const in & envmap, out & map, const std::string_view & sp)
	{
		auto qs = envmap.find(vn);
		if (qs != envmap.end()) {
			XWwwFormUrlEncoded::iterateVars(
					qs->second,
					[&map](auto && k, auto && v) {
						map.insert({std::forward<decltype(k)>(k), std::forward<decltype(v)>(v)});
					},
					sp);
		}
	}

	template<typename Ex, typename Map, typename... Ks>
	const std::string_view &
	findFirstOrElse(const Map & map, const std::string_view & k, const Ks &... ks)
	{
		if (const auto i = map.find(k); i != map.end()) {
			return i->second;
		}
		if constexpr (sizeof...(ks)) {
			return findFirstOrElse<Ex>(map, ks...);
		}
		throw Ex();
	}

	CgiRequestBase::CgiRequestBase(Core * c, const EnvArray envs, const EnvArray extra) : IHttpRequest(c)
	{
		for (const auto & envdata : {envs, extra}) {
			for (const std::string_view e : envdata) {
				if (const auto eq = e.find('='); eq != std::string_view::npos) {
					envmap.insert({e.substr(0, eq), e.substr(eq + 1)});
				}
			}
		}

		if (auto path = findFirstOrElse<Http400_BadRequest>(envmap, REDIRECT_URL, SCRIPT_NAME).substr(1);
				// NOLINTNEXTLINE(clang-analyzer-cplusplus.NewDeleteLeaks)
				!path.empty()) {
			ba::split(pathElements, path, slash_pred, ba::token_compress_off);
		}

		mapVars(QUERY_STRING, envmap, qsmap, amp);
		mapVars(HTTP_COOKIE, envmap, cookiemap, semi);
		for (auto header = envmap.lower_bound(HEADER_PREFIX);
				header != envmap.end() && ba::starts_with(header->first, HEADER_PREFIX); header++) {
			hdrmap.insert({header->first.substr(HEADER_PREFIX.length()), header->second});
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

	bool
	CgiRequestBase::isSecure() const
	{
		return envmap.contains(HTTPS);
	}

	OptionalString
	CgiRequestBase::getHeaderParam(const std::string_view & key) const
	{
		return optionalLookup(key, hdrmap);
	}

	void
	CgiRequestBase::response(short statusCode, const std::string_view & statusMsg) const
	{
		StatusFmt::write(getOutputStream(), statusCode, statusMsg);
	}

	void
	CgiRequestBase::setHeader(const std::string_view & header, const std::string_view & value) const
	{
		HdrFmt::write(getOutputStream(), header, value);
	}
}
