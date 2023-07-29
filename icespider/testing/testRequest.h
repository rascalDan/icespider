#pragma once

#include <functional>
#include <http.h>
#include <ihttpRequest.h>
#include <iosfwd>
#include <map>
#include <string>
#include <string_view>
#include <visibility.h>

namespace IceSpider {
	class Core;

	class DLL_PUBLIC TestRequest : public IHttpRequest {
	public:
		using MapVars = std::map<std::string, std::string, std::less<>>;

		TestRequest(const Core * c, HttpMethod m, const std::string_view p);

		const PathElements & getRequestPath() const override;
		PathElements & getRequestPath() override;
		HttpMethod getRequestMethod() const override;
		OptionalString getEnvStr(const std::string_view key) const override;
		OptionalString getQueryStringParamStr(const std::string_view key) const override;
		OptionalString getCookieParamStr(const std::string_view key) const override;
		OptionalString getHeaderParamStr(const std::string_view key) const override;
		bool isSecure() const override;
		void setQueryStringParam(const std::string_view, const OptionalString &);
		void setHeaderParam(const std::string_view, const OptionalString &);
		void setCookieParam(const std::string_view, const OptionalString &);
		void setEnv(const std::string_view, const OptionalString &);
		std::istream & getInputStream() const override;
		std::ostream & getOutputStream() const override;
		void response(short statusCode, const std::string_view statusMsg) const override;
		void setHeader(const std::string_view header, const std::string_view value) const override;
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
		OptionalString get(const std::string_view, const MapVars &) const;
		void set(const std::string_view, const OptionalString &, MapVars &);

	private:
		MapVars responseHeaders;
	};
}
