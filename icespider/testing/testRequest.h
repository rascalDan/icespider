#ifndef ICESPIDER_TESTING_TESTREQUEST_H
#define ICESPIDER_TESTING_TESTREQUEST_H

#include <ihttpRequest.h>
#include <visibility.h>

namespace IceSpider {
	class DLL_PUBLIC TestRequest : public IHttpRequest {
		public:
			typedef std::map<std::string, std::string> MapVars;

			TestRequest(const Core * c, HttpMethod m, const std::string & p);

			const PathElements & getRequestPath() const override;
			PathElements & getRequestPath() override;
			HttpMethod getRequestMethod() const override;
			OptionalString getEnv(const std::string & key) const override;
			OptionalString getQueryStringParam(const std::string & key) const override;
			OptionalString getCookieParam(const std::string & key) const override;
			OptionalString getHeaderParam(const std::string & key) const override;
			void setQueryStringParam(const std::string &, const OptionalString &) override;
			void setHeaderParam(const std::string &, const OptionalString &) override;
			void setCookieParam(const std::string &, const OptionalString &) override;
			void setEnv(const std::string &, const OptionalString &) override;
			std::istream & getInputStream() const override;
			std::ostream & getOutputStream() const override;
			void response(short statusCode, const std::string & statusMsg) const override;
			void setHeader(const std::string & header, const std::string & value) const override;

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
			OptionalString get(const std::string &, const MapVars &) const;
			void set(const std::string &, const OptionalString &, MapVars &);

		private:
			MapVars responseHeaders;
	};
}

#endif

