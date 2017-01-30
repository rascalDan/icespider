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

			CgiRequestBase(Core * c, char ** env);
			void addenv(char *);
			void initialize();

		public:
			typedef std::tuple<char *, char *> Env;
			typedef std::map<const char *, Env, cmp_str> VarMap;

			const PathElements & getRequestPath() const override;
			PathElements & getRequestPath() override;
			HttpMethod getRequestMethod() const override;
			OptionalString getQueryStringParam(const std::string & key) const override;
			OptionalString getHeaderParam(const std::string & key) const override;
			OptionalString getCookieParam(const std::string & key) const override;
			OptionalString getEnv(const std::string & key) const override;
			void setQueryStringParam(const std::string &, const OptionalString &) override;
			void setHeaderParam(const std::string &, const OptionalString &) override;
			void setCookieParam(const std::string &, const OptionalString &) override;
			void setEnv(const std::string &, const OptionalString &) override;

			void response(short, const std::string &) const override;
			void setHeader(const std::string &, const std::string &) const override;

		private:
			static OptionalString optionalLookup(const std::string & key, const VarMap &);
			static OptionalString optionalLookup(const std::string & key, const StringMap &);

			VarMap envmap;
			StringMap qsmap;
			StringMap cookiemap;
			PathElements pathElements;
	};
}

#endif

