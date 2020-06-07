#ifndef ICESPIDER_CGI_CGIREQUESTBASE_H
#define ICESPIDER_CGI_CGIREQUESTBASE_H

#include <core.h>
#include <ihttpRequest.h>
#include <map>
#include <string_view>

namespace IceSpider {
	class CgiRequestBase : public IHttpRequest {
	protected:
		CgiRequestBase(Core * c, const char * const * const env);
		void addenv(const char * const);
		void initialize();

	public:
		using VarMap = std::map<std::string_view, const std::string_view>;
		using HdrMap = std::map<std::string_view, const std::string_view, Slicer::case_less>;

		[[nodiscard]] const PathElements & getRequestPath() const override;
		[[nodiscard]] PathElements & getRequestPath() override;
		[[nodiscard]] HttpMethod getRequestMethod() const override;
		[[nodiscard]] OptionalString getQueryStringParam(const std::string_view & key) const override;
		[[nodiscard]] OptionalString getHeaderParam(const std::string_view & key) const override;
		[[nodiscard]] OptionalString getCookieParam(const std::string_view & key) const override;
		[[nodiscard]] OptionalString getEnv(const std::string_view & key) const override;
		[[nodiscard]] bool isSecure() const override;

		void response(short, const std::string_view &) const override;
		void setHeader(const std::string_view &, const std::string_view &) const override;

		std::ostream & dump(std::ostream & s) const override;

	private:
		template<typename MapType> static OptionalString optionalLookup(const std::string_view & key, const MapType &);

		VarMap envmap;
		StringMap qsmap;
		StringMap cookiemap;
		HdrMap hdrmap;
		PathElements pathElements;
	};
}

#endif
