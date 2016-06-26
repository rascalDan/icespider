#ifndef ICESPIDER_CGI_CGIREQUESTBASE_H
#define ICESPIDER_CGI_CGIREQUESTBASE_H

#include <core.h>
#include <ihttpRequest.h>
#include <map>
#include <tuple>

namespace IceSpider {
	class CgiRequestBase : public IceSpider::IHttpRequest {
		protected:
			struct cmp_str {
				bool operator()(char const *a, char const *b) const;
			};

			typedef std::tuple<char *, char *> Env;
			typedef std::map<const char *, Env, cmp_str> VarMap;
			typedef std::vector<std::string> UrlMap;

			CgiRequestBase(IceSpider::Core * c, char ** env);
			void addenv(char *);
			void initialize();

			const std::vector<std::string> & getRequestPath() const override;
			UserIceSpider::HttpMethod getRequestMethod() const override;
			IceUtil::Optional<std::string> getQueryStringParam(const std::string & key) const override;
			IceUtil::Optional<std::string> getHeaderParam(const std::string & key) const override;

			static IceUtil::Optional<std::string> optionalLookup(const std::string & key, const VarMap &);

			VarMap envmap;
			VarMap qsmap;
			UrlMap pathmap;
	};
}

#endif

