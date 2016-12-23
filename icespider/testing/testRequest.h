#ifndef ICESPIDER_TESTING_TESTREQUEST_H
#define ICESPIDER_TESTING_TESTREQUEST_H

#include <ihttpRequest.h>
#include <visibility.h>

namespace IceSpider {
	class DLL_PUBLIC TestRequest : public IHttpRequest {
		public:
			typedef std::map<std::string, std::string> MapVars;
			typedef std::vector<std::string> UrlVars;

			TestRequest(const Core * c, HttpMethod m, const std::string & p);

			const std::vector<std::string> & getRequestPath() const override;
			HttpMethod getRequestMethod() const override;
			IceUtil::Optional<std::string> getEnv(const std::string & key) const override;
			IceUtil::Optional<std::string> getQueryStringParam(const std::string & key) const override;
			IceUtil::Optional<std::string> getCookieParam(const std::string & key) const override;
			IceUtil::Optional<std::string> getHeaderParam(const std::string & key) const override;
			std::istream & getInputStream() const override;
			std::ostream & getOutputStream() const override;
			void response(short statusCode, const std::string & statusMsg) const override;
			void setHeader(const std::string & header, const std::string & value) const override;

			const MapVars & getResponseHeaders();

			UrlVars url;
			MapVars qs;
			MapVars cookies;
			MapVars hdr;
			MapVars env;
			mutable std::stringstream input;
			mutable std::stringstream output;
			const HttpMethod method;

		private:
			MapVars responseHeaders;
	};
}

#endif

