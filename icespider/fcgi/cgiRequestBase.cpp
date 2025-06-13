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

namespace ba = boost::algorithm;
using namespace std::literals;

#define CGI_CONST(NAME) static constexpr std::string_view NAME(#NAME)

namespace IceSpider {
	namespace {
		const auto SLASH_PRED = boost::algorithm::is_any_of("/");
		constexpr std::string_view AMP("&");
		constexpr std::string_view SEMI("; ");
		constexpr std::string_view HEADER_PREFIX("HTTP_");
		CGI_CONST(HTTPS);
		CGI_CONST(REDIRECT_URL);
		CGI_CONST(SCRIPT_NAME);
		CGI_CONST(QUERY_STRING);
		CGI_CONST(HTTP_COOKIE);
		CGI_CONST(REQUEST_METHOD);

		template<typename In, typename Out>
		inline void
		mapVars(const std::string_view key, const In & envmap, Out & map, const std::string_view separators)
		{
			auto qs = envmap.find(key);
			if (qs != envmap.end()) {
				XWwwFormUrlEncoded::iterateVars(
						qs->second,
						[&map](auto && key, auto && value) {
							map.insert({std::forward<decltype(key)>(key), std::forward<decltype(value)>(value)});
						},
						separators);
			}
		}

		template<typename Ex, typename Map, typename... Ks>
		std::string_view
		findFirstOrElse(const Map & map, const std::string_view key, const Ks &... ks)
		{
			if (const auto iter = map.find(key); iter != map.end()) {
				return iter->second;
			}
			if constexpr (sizeof...(ks)) {
				return findFirstOrElse<Ex>(map, ks...);
			}
			throw Ex();
		}

		template<typename Fmt, typename Map>
		void
		dumpMap(std::ostream & strm, const std::string_view name, const Map & map)
		{
			strm << name << '\n';
			for (const auto & [key, value] : map) {
				Fmt::write(strm, key, value);
			}
		}
	}

	CgiRequestBase::CgiRequestBase(Core * core, const EnvArray envs, const EnvArray extra) : IHttpRequest(core)
	{
		for (const auto & envdata : {envs, extra}) {
			for (const std::string_view env : envdata) {
				if (const auto equalPos = env.find('='); equalPos != std::string_view::npos) {
					envmap.emplace(env.substr(0, equalPos), env.substr(equalPos + 1));
				}
			}
		}

		if (auto path = findFirstOrElse<Http400BadRequest>(envmap, REDIRECT_URL, SCRIPT_NAME).substr(1);
				// NOLINTNEXTLINE(clang-analyzer-cplusplus.NewDeleteLeaks)
				!path.empty()) {
			ba::split(pathElements, path, SLASH_PRED, ba::token_compress_off);
		}

		mapVars(QUERY_STRING, envmap, qsmap, AMP);
		mapVars(HTTP_COOKIE, envmap, cookiemap, SEMI);
		for (auto header = envmap.lower_bound(HEADER_PREFIX);
				header != envmap.end() && ba::starts_with(header->first, HEADER_PREFIX); header++) {
			hdrmap.insert({header->first.substr(HEADER_PREFIX.length()), header->second});
		}
	}

	AdHocFormatter(VarFmt, "\t%?: [%?]\n");
	AdHocFormatter(PathFmt, "\t[%?]\n");

	std::ostream &
	CgiRequestBase::dump(std::ostream & strm) const
	{
		dumpMap<VarFmt>(strm, "Environment dump"sv, envmap);
		strm << "Path dump" << '\n';
		for (const auto & element : pathElements) {
			PathFmt::write(strm, element);
		}
		dumpMap<VarFmt>(strm, "Query string dump"sv, qsmap);
		dumpMap<VarFmt>(strm, "Cookie dump"sv, cookiemap);
		return strm;
	}

	template<typename MapType>
	OptionalString
	CgiRequestBase::optionalLookup(const std::string_view key, const MapType & map)
	{
		auto iter = map.find(key);
		if (iter == map.end()) {
			return {};
		}
		return iter->second;
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
			auto iter = envmap.find(REQUEST_METHOD);
			if (iter == envmap.end()) {
				throw IceSpider::Http400BadRequest();
			}
			return Slicer::ModelPartForEnum<HttpMethod>::lookup(iter->second);
		}
		catch (const Slicer::InvalidEnumerationSymbol &) {
			throw IceSpider::Http405MethodNotAllowed();
		}
	}

	OptionalString
	CgiRequestBase::getQueryStringParamStr(const std::string_view key) const
	{
		return optionalLookup(key, qsmap);
	}

	OptionalString
	CgiRequestBase::getCookieParamStr(const std::string_view key) const
	{
		return optionalLookup(key, cookiemap);
	}

	OptionalString
	CgiRequestBase::getEnvStr(const std::string_view key) const
	{
		return optionalLookup(key, envmap);
	}

	bool
	CgiRequestBase::isSecure() const
	{
		return envmap.contains(HTTPS);
	}

	OptionalString
	CgiRequestBase::getHeaderParamStr(const std::string_view key) const
	{
		return optionalLookup(key, hdrmap);
	}

	void
	CgiRequestBase::response(short statusCode, const std::string_view statusMsg) const
	{
		StatusFmt::write(getOutputStream(), statusCode, statusMsg);
	}

	void
	CgiRequestBase::setHeader(const std::string_view header, const std::string_view value) const
	{
		HdrFmt::write(getOutputStream(), header, value);
	}
}
