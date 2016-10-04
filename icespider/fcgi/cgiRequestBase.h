#ifndef ICESPIDER_CGI_CGIREQUESTBASE_H
#define ICESPIDER_CGI_CGIREQUESTBASE_H

#include <core.h>
#include <ihttpRequest.h>
#include <map>
#include <tuple>

namespace IceSpider {
	class CgiRequestBase : public IHttpRequest {
		protected:
			struct cmp_str {
				bool operator()(char const *a, char const *b) const;
			};

			typedef std::tuple<char *, char *> Env;
			typedef std::map<const char *, Env, cmp_str> VarMap;

			CgiRequestBase(Core * c, char ** env);
			void addenv(char *);
			void initialize();

		public:
			const PathElements & getRequestPath() const override;
			HttpMethod getRequestMethod() const override;
			OptionalString getQueryStringParam(const std::string & key) const override;
			OptionalString getHeaderParam(const std::string & key) const override;
			OptionalString getEnv(const std::string & key) const override;

		private:
			static OptionalString optionalLookup(const std::string & key, const VarMap &);

			VarMap envmap;
			VarMap qsmap;
			PathElements pathElements;
	};
}

#endif

