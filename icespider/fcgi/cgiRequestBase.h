#ifndef ICESPIDER_CGI_CGIREQUESTBASE_H
#define ICESPIDER_CGI_CGIREQUESTBASE_H

#include <core.h>
#include <ihttpRequest.h>
#include <map>
#include <tuple>
#include <string_view>

namespace IceSpider {
	class CgiRequestBase : public IHttpRequest {
		protected:
			CgiRequestBase(Core * c, const char * const * const env);
			void addenv(const char * const);
			void initialize();

		public:
			typedef std::map<std::string_view, const std::string_view> VarMap;

			const PathElements & getRequestPath() const override;
			PathElements & getRequestPath() override;
			HttpMethod getRequestMethod() const override;
			OptionalString getQueryStringParam(const std::string_view & key) const override;
			OptionalString getHeaderParam(const std::string_view & key) const override;
			OptionalString getCookieParam(const std::string_view & key) const override;
			OptionalString getEnv(const std::string_view & key) const override;

			void response(short, const std::string_view &) const override;
			void setHeader(const std::string_view &, const std::string_view &) const override;

			std::ostream & dump(std::ostream & s) const override;

		private:
			static OptionalString optionalLookup(const std::string_view & key, const VarMap &);
			static OptionalString optionalLookup(const std::string_view & key, const StringMap &);

			VarMap envmap;
			StringMap qsmap;
			StringMap cookiemap;
			PathElements pathElements;
	};
}

#endif

