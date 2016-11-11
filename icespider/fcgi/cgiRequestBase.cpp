#include "cgiRequestBase.h"
#include "xwwwFormUrlEncoded.h"
#include <boost/algorithm/string/case_conv.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <util.h>
#include <slicer/modelPartsTypes.h>
#include <slicer/common.h>

namespace ba = boost::algorithm;

namespace IceSpider {
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
		auto path = (optionalLookup("REDIRECT_URL", envmap) /
			[this]() { return optionalLookup("SCRIPT_NAME", envmap); } /
			[this]() -> std::string { throw std::runtime_error("Couldn't determine request path"); })
			.substr(1);
		if (!path.empty()) {
			ba::split(pathElements, path, ba::is_any_of("/"), ba::token_compress_off);
		}

		auto qs = envmap.find("QUERY_STRING");
		if (qs != envmap.end()) {
			XWwwFormUrlEncoded::iterateVars(*qs->second, [this](auto k, auto v) {
				qsmap.insert({ k, v });
			}, "&");
		}
		auto cs = envmap.find("HTTP_COOKIE");
		if (cs != envmap.end()) {
			XWwwFormUrlEncoded::iterateVars(*cs->second, [this](auto k, auto v) {
				cookiemap.insert({ k, v });
			}, "; ");
		}
	}

	OptionalString
	CgiRequestBase::optionalLookup(const std::string & key, const VarMap & vm)
	{
		auto i = vm.find(key.c_str());
		if (i == vm.end()) {
			return IceUtil::None;
		}
		return *i->second;
	}

	OptionalString
	CgiRequestBase::optionalLookup(const std::string & key, const StringMap & vm)
	{
		auto i = vm.find(key.c_str());
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

	HttpMethod
	CgiRequestBase::getRequestMethod() const
	{
		try {
			auto i = envmap.find("REQUEST_METHOD");
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
		return optionalLookup(("HTTP_" + boost::algorithm::to_upper_copy(key)).c_str(), envmap);
	}

	void CgiRequestBase::response(short statusCode, const std::string & statusMsg) const
	{
		getOutputStream()
			<< "Status: " << statusCode << " " << statusMsg << "\r\n"
			<< "\r\n";
	}

	void
	CgiRequestBase::setHeader(const std::string & header, const std::string & value) const
	{
		getOutputStream() << header << ": " << value << "\r\n";
	}

	bool
	CgiRequestBase::cmp_str::operator()(char const * a, char const * b) const
	{
		return std::strcmp(a, b) < 0;
	}
}

