#ifndef ICESPIDER_CGI_CGIREQUESTBASE_H
#define ICESPIDER_CGI_CGIREQUESTBASE_H

#include "http.h"
#include <case_less.h>
#include <flatMap.h>
#include <ihttpRequest.h>
#include <iosfwd>
#include <maybeString.h>
#include <string_view>
// IWYU pragma: no_forward_declare AdHoc::case_less

namespace IceSpider {
	class Core;

	class CgiRequestBase : public IHttpRequest {
	protected:
		CgiRequestBase(Core * c, const char * const * const env);
		void addenv(const std::string_view);
		void initialize();

	public:
		using VarMap = flatmap<std::string_view, std::string_view>;
		using HdrMap = flatmap<std::string_view, std::string_view, AdHoc::case_less>;
		using StrMap = flatmap<MaybeString, MaybeString>;

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

		VarMap envmap {40};
		StrMap qsmap;
		StrMap cookiemap;
		HdrMap hdrmap {15};
		PathElements pathElements;
	};
}

#endif
