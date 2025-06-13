#pragma once

#include <case_less.h>
#include <flatMap.h>
#include <http.h>
#include <ihttpRequest.h>
#include <iosfwd>
#include <maybeString.h>
#include <span>
#include <string_view>

// IWYU pragma: no_forward_declare AdHoc::case_less

namespace IceSpider {
	class Core;

	class CgiRequestBase : public IHttpRequest {
	protected:
		using EnvArray = std::span<const char * const>;
		// Null terminated list, bsv will handle this and is convertible to span
		using EnvNTL = std::basic_string_view<const char * const>;

		CgiRequestBase(Core * core, EnvArray envs, EnvArray extra = {});

	public:
		using VarMap = FlatMap<std::string_view, std::string_view>;
		using HdrMap = FlatMap<std::string_view, std::string_view, AdHoc::case_less>;
		using StrMap = FlatMap<MaybeString, MaybeString>;

		[[nodiscard]] const PathElements & getRequestPath() const override;
		[[nodiscard]] PathElements & getRequestPath() override;
		[[nodiscard]] HttpMethod getRequestMethod() const override;
		[[nodiscard]] OptionalString getQueryStringParamStr(std::string_view key) const override;
		[[nodiscard]] OptionalString getHeaderParamStr(std::string_view key) const override;
		[[nodiscard]] OptionalString getCookieParamStr(std::string_view key) const override;
		[[nodiscard]] OptionalString getEnvStr(std::string_view key) const override;
		[[nodiscard]] bool isSecure() const override;

		void response(short, std::string_view) const override;
		void setHeader(std::string_view, std::string_view) const override;

		std::ostream & dump(std::ostream & strm) const override;

	private:
		template<typename MapType> static OptionalString optionalLookup(std::string_view key, const MapType &);

		VarMap envmap {40};
		StrMap qsmap;
		StrMap cookiemap;
		HdrMap hdrmap {15};
		PathElements pathElements;
	};
}
