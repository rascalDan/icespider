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

		TestRequest(const Core * core, HttpMethod method, std::string_view path);

		const PathElements & getRequestPath() const override;
		PathElements & getRequestPath() override;
		HttpMethod getRequestMethod() const override;
		OptionalString getEnvStr(std::string_view key) const override;
		OptionalString getQueryStringParamStr(std::string_view key) const override;
		OptionalString getCookieParamStr(std::string_view key) const override;
		OptionalString getHeaderParamStr(std::string_view key) const override;
		bool isSecure() const override;
		void setQueryStringParam(std::string_view, const OptionalString &);
		void setHeaderParam(std::string_view, const OptionalString &);
		void setCookieParam(std::string_view, const OptionalString &);
		void setEnv(std::string_view, const OptionalString &);
		std::istream & getInputStream() const override;
		std::ostream & getOutputStream() const override;
		void response(short statusCode, std::string_view statusMsg) const override;
		void setHeader(std::string_view header, std::string_view value) const override;
		std::ostream & dump(std::ostream & strm) const override;

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
		static OptionalString get(std::string_view, const MapVars &);
		static void set(std::string_view, const OptionalString &, MapVars &);

	private:
		MapVars responseHeaders;
	};
}
