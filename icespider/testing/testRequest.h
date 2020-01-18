#ifndef ICESPIDER_TESTING_TESTREQUEST_H
#define ICESPIDER_TESTING_TESTREQUEST_H

#include <ihttpRequest.h>
#include <visibility.h>

namespace IceSpider {
	class DLL_PUBLIC TestRequest : public IHttpRequest {
		public:
			typedef std::map<std::string, std::string, std::less<>> MapVars;

			TestRequest(const Core * c, HttpMethod m, const std::string_view & p);

			const PathElements & getRequestPath() const override;
			PathElements & getRequestPath() override;
			HttpMethod getRequestMethod() const override;
			OptionalString getEnv(const std::string_view & key) const override;
			OptionalString getQueryStringParam(const std::string_view & key) const override;
			OptionalString getCookieParam(const std::string_view & key) const override;
			OptionalString getHeaderParam(const std::string_view & key) const override;
			bool isSecure() const override;
			void setQueryStringParam(const std::string_view &, const OptionalString &);
			void setHeaderParam(const std::string_view &, const OptionalString &);
			void setCookieParam(const std::string_view &, const OptionalString &);
			void setEnv(const std::string_view &, const OptionalString &);
			std::istream & getInputStream() const override;
			std::ostream & getOutputStream() const override;
			void response(short statusCode, const std::string_view & statusMsg) const override;
			void setHeader(const std::string_view & header, const std::string_view & value) const override;
			std::ostream & dump(std::ostream & s) const override;

			const MapVars & getResponseHeaders();

			PathElements url;
			MapVars qs;
			MapVars cookies;
			MapVars hdr;
			MapVars env;
			mutable std::stringstream input;
			mutable std::stringstream output;
			const HttpMethod method;

		protected:
			OptionalString get(const std::string_view &, const MapVars &) const;
			void set(const std::string_view &, const OptionalString &, MapVars &);

		private:
			MapVars responseHeaders;
	};
}

#endif

