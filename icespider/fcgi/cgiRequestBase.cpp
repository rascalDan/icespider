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

namespace std {
	ostream & operator<<(ostream & s, const IceSpider::CgiRequestBase::Env & e) {
		return s.write(std::get<0>(e), std::get<1>(e) - std::get<0>(e));
	}
}

#define CGI_CONST(NAME) static const std::string_view NAME(#NAME)

namespace IceSpider {
	static const std::string HEADER_PREFIX("HTTP_");
	CGI_CONST(REDIRECT_URL);
	CGI_CONST(SCRIPT_NAME);
	CGI_CONST(QUERY_STRING);
	CGI_CONST(HTTP_COOKIE);
	CGI_CONST(REQUEST_METHOD);

	CgiRequestBase::CgiRequestBase(Core * c, char ** env) :
		IHttpRequest(c)
	{
		for(char * const * e = env; *e; ++e) {
			addenv(*e);
		}
	}

	void
	CgiRequestBase::addenv(char * e)
	{
		if (auto eq = strchr(e, '=')) {
			*eq++ = '\0';
			envmap.insert({ e, Env(eq, strchr(eq, '\0')) });
		}
	}

	std::string
	operator*(const CgiRequestBase::Env & t)
	{
		return std::string(std::get<0>(t), std::get<1>(t));
	}

	void
	CgiRequestBase::initialize()
	{
		namespace ba = boost::algorithm;
		auto path = (optionalLookup(REDIRECT_URL, envmap) /
			[this]() { return optionalLookup(SCRIPT_NAME, envmap); } /
			[this]() -> std::string { throw std::runtime_error("Couldn't determine request path"); })
			.substr(1);
		if (!path.empty()) {
			ba::split(pathElements, path, ba::is_any_of("/"), ba::token_compress_off);
		}

		auto qs = envmap.find(QUERY_STRING);
		if (qs != envmap.end()) {
			XWwwFormUrlEncoded::iterateVars(*qs->second, [this](auto k, auto v) {
				qsmap.insert({ k, v });
			}, "&");
		}
		auto cs = envmap.find(HTTP_COOKIE);
		if (cs != envmap.end()) {
			XWwwFormUrlEncoded::iterateVars(*cs->second, [this](auto k, auto v) {
				cookiemap.insert({ k, v });
			}, "; ");
		}
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

