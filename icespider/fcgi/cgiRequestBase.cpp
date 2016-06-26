#include "cgiRequestBase.h"
#include <boost/algorithm/string/case_conv.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <util.h>

namespace ba = boost::algorithm;

namespace IceSpider {
	CgiRequestBase::CgiRequestBase(IceSpider::Core * c, char ** env) :
		IceSpider::IHttpRequest(c)
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

	void
	CgiRequestBase::initialize()
	{
		namespace ba = boost::algorithm;
		auto path = (optionalLookup("REDIRECT_URL", envmap) /
			[this]() { return optionalLookup("SCRIPT_NAME", envmap); } /
			[this]() -> std::string { throw std::runtime_error("Couldn't determine request path"); })
			.substr(1);
		if (!path.empty()) {
			ba::split(pathmap, path, ba::is_any_of("/"), ba::token_compress_off);
		}

		auto qs = envmap.find("QUERY_STRING");
		if (qs != envmap.end()) {
			auto start = std::get<0>(qs->second);
			auto end = std::get<1>(qs->second);
			while (start < end) {
				auto amp = orelse(strchr(start, '&'), end);
				auto eq = orelse(strchr(start, '='), end);
				if (eq < amp) {
					*eq = '\0';
					*amp = '\0';
					qsmap.insert({ eq, Env( eq + 1, amp ) });
				}
				else {
					*eq = '\0';
					*amp = '\0';
					qsmap.insert({ eq, Env( eq + 1, eq + 1 ) });
				}
				start = amp + 1;
			}
		}
	}

	IceUtil::Optional<std::string>
	CgiRequestBase::optionalLookup(const std::string & key, const VarMap & vm)
	{
		auto i = vm.find(key.c_str());
		if (i == vm.end()) {
			return IceUtil::Optional<std::string>();
		}
		return std::string(std::get<0>(i->second), std::get<1>(i->second));
	}

	const std::vector<std::string> &
	CgiRequestBase::getRequestPath() const
	{
		return pathmap;
	}

	UserIceSpider::HttpMethod
	CgiRequestBase::getRequestMethod() const
	{
		return UserIceSpider::HttpMethod::GET;
	}

	IceUtil::Optional<std::string>
	CgiRequestBase::getQueryStringParam(const std::string & key) const
	{
		return optionalLookup(key, qsmap);
	}

	IceUtil::Optional<std::string>
	CgiRequestBase::getHeaderParam(const std::string & key) const
	{
		return optionalLookup(("HTTP_" + boost::algorithm::to_upper_copy(key)).c_str(), envmap);
	}

	bool
	CgiRequestBase::cmp_str::operator()(char const * a, char const * b) const
	{
		return std::strcmp(a, b) < 0;
	}
}

