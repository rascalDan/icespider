#include "cgiRequestBase.h"
#include "xwwwFormUrlEncoded.h"
#include <boost/algorithm/string/case_conv.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <util.h>
#include <slicer/modelPartsTypes.h>
#include <slicer/common.h>
#include <formatters.h>

namespace ba = boost::algorithm;
using namespace std::literals;

#define CGI_CONST(NAME) static const std::string_view NAME(#NAME)

namespace IceSpider {
	static const std::string HEADER_PREFIX("HTTP_");
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

	inline
	std::string
	operator*(const std::string_view & t)
	{
		return std::string(t);
	}

	template<typename in, typename out>
	inline
	void
	mapVars(const std::string_view & vn, const in & envmap, out & map, const std::string_view & sp) {
		auto qs = envmap.find(vn);
		if (qs != envmap.end()) {
			XWwwFormUrlEncoded::iterateVars(qs->second, [&map](auto k, auto v) {
				map.insert({ k, v });
			}, sp);
		}
	}

	template<typename Ex, typename Map>
	const std::string_view &
	findFirstOrElse(const Map &, const std::string & msg)
	{
		throw Ex(msg);
	}

	template<typename Ex, typename Map, typename ... Ks>
	const std::string_view &
	findFirstOrElse(const Map & map, const std::string & msg, const std::string_view & k, const Ks & ... ks)
	{
		auto i = map.find(k);
		if (i == map.end()) {
			return findFirstOrElse<Ex>(map, msg, ks...);
		}
		return i->second;
	}

	void
	CgiRequestBase::initialize()
	{
		namespace ba = boost::algorithm;
		auto path = findFirstOrElse<std::runtime_error>(envmap, "Couldn't determine request path"s,
				REDIRECT_URL,
				SCRIPT_NAME).substr(1);
		if (!path.empty()) {
			ba::split(pathElements, path, ba::is_any_of("/"), ba::token_compress_off);
		}

		mapVars(QUERY_STRING, envmap, qsmap, "&"sv);
		mapVars(HTTP_COOKIE, envmap, cookiemap, "; "sv);
	}

	AdHocFormatter(VarFmt, "\t%?: [%?]\n");
	AdHocFormatter(PathFmt, "\t[%?]\n");
	std::ostream &
	CgiRequestBase::dump(std::ostream & s) const
	{
		s << "Environment dump" << std::endl;
		for (const auto & e : envmap) {
			VarFmt::write(s, e.first, e.second);
		}
		s << "Path dump" << std::endl;
		for (const auto & e : pathElements) {
			PathFmt::write(s, e);
		}
		s << "Query string dump" << std::endl;
		for (const auto & v : qsmap) {
			VarFmt::write(s, v.first, v.second);
		}
		s << "Cookie dump" << std::endl;
		for (const auto & c : cookiemap) {
			VarFmt::write(s, c.first, c.second);
		}
		return s;
	}

	OptionalString
	CgiRequestBase::optionalLookup(const std::string_view & key, const VarMap & vm)
	{
		auto i = vm.find(key);
		if (i == vm.end()) {
			return IceUtil::None;
		}
		return *i->second;
	}

	OptionalString
	CgiRequestBase::optionalLookup(const std::string & key, const StringMap & vm)
	{
		auto i = vm.find(key);
		if (i == vm.end()) {
			return IceUtil::None;
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
			return Slicer::ModelPartForEnum<HttpMethod>::lookup(*i->second);
		}
		catch (const Slicer::InvalidEnumerationSymbol &) {
			throw IceSpider::Http405_MethodNotAllowed();
		}
	}

	OptionalString
	CgiRequestBase::getQueryStringParam(const std::string & key) const
	{
		return optionalLookup(key, qsmap);
	}

	OptionalString
	CgiRequestBase::getCookieParam(const std::string & key) const
	{
		return optionalLookup(key, cookiemap);
	}

	OptionalString
	CgiRequestBase::getEnv(const std::string & key) const
	{
		return optionalLookup(key, envmap);
	}

	OptionalString
	CgiRequestBase::getHeaderParam(const std::string & key) const
	{
		return optionalLookup(HEADER_PREFIX + boost::algorithm::to_upper_copy(key), envmap);
	}

	void
	CgiRequestBase::setQueryStringParam(const std::string & key, const OptionalString & val)
	{
		if (val)
			qsmap[key] = val;
		else
			qsmap.erase(key);
	}

	void
	CgiRequestBase::setHeaderParam(const std::string &, const OptionalString &)
	{
		throw std::runtime_error("Changing the CGI environment is not supported.");
	}

	void
	CgiRequestBase::setCookieParam(const std::string & key, const OptionalString & val)
	{
		if (val)
			cookiemap[key] = val;
		else
			cookiemap.erase(key);
	}

	void
	CgiRequestBase::setEnv(const std::string &, const OptionalString &)
	{
		throw std::runtime_error("Changing the CGI environment is not supported.");
	}

	void CgiRequestBase::response(short statusCode, const std::string & statusMsg) const
	{
		StatusFmt::write(getOutputStream(), statusCode, statusMsg);
	}

	void
	CgiRequestBase::setHeader(const std::string & header, const std::string & value) const
	{
		HdrFmt::write(getOutputStream(), header, value);
	}
}

