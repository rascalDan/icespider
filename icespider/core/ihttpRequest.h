#pragma once

#include "exceptions.h"
#include "util.h"
#include <Ice/Current.h>
#include <boost/lexical_cast.hpp>
#include <c++11Helpers.h>
#include <charconv>
#include <ctime>
#include <http.h>
#include <iosfwd>
#include <map>
#include <optional>
#include <slicer/modelParts.h>
#include <slicer/serializer.h>
#include <slicer/slicer.h>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>
#include <visibility.h>

namespace IceSpider {
	class Core;
	class IRouteHandler;

	struct Accept {
		std::optional<std::string_view> group, type;
		float q {1.0F};
	};

	using Accepted = std::vector<Accept>;
	using PathElements = std::vector<std::string_view>;
	using OptionalString = std::optional<std::string_view>;
	using ContentTypeSerializer = std::pair<MimeType, Slicer::SerializerPtr>;

	class DLL_PUBLIC IHttpRequest {
	public:
		explicit IHttpRequest(const Core *);
		virtual ~IHttpRequest() = default;
		SPECIAL_MEMBERS_DEFAULT_MOVE_NO_COPY(IHttpRequest);

		[[nodiscard]] Ice::Context getContext() const;
		[[nodiscard]] virtual const PathElements & getRequestPath() const = 0;
		[[nodiscard]] virtual PathElements & getRequestPath() = 0;
		[[nodiscard]] virtual HttpMethod getRequestMethod() const = 0;

		[[nodiscard]] std::string_view getURLParamStr(const unsigned int) const;
		[[nodiscard]] virtual OptionalString getQueryStringParamStr(const std::string_view) const = 0;
		[[nodiscard]] virtual OptionalString getHeaderParamStr(const std::string_view) const = 0;
		[[nodiscard]] virtual OptionalString getCookieParamStr(const std::string_view) const = 0;
		[[nodiscard]] virtual OptionalString getEnvStr(const std::string_view) const = 0;
		[[nodiscard]] virtual bool isSecure() const = 0;
		[[nodiscard]] static Accepted parseAccept(std::string_view);
		[[nodiscard]] virtual Slicer::DeserializerPtr getDeserializer() const;
		[[nodiscard]] virtual ContentTypeSerializer getSerializer(const IRouteHandler *) const;
		[[nodiscard]] virtual std::istream & getInputStream() const = 0;
		[[nodiscard]] virtual std::ostream & getOutputStream() const = 0;
		virtual void setHeader(const std::string_view, const std::string_view) const = 0;

		virtual std::ostream & dump(std::ostream & s) const = 0;

		template<typename T, typename K>
		[[nodiscard]] inline std::optional<T>
		getFrom(const K key, OptionalString (IHttpRequest::*src)(const K) const) const
		{
			if (auto v = (this->*src)(key)) {
				return convert<T>(*v);
			}
			else {
				return std::nullopt;
			}
		}

		template<typename T>
		[[nodiscard]] T
		getURLParam(unsigned int n) const
		{
			return convert<T>(getURLParamStr(n));
		}

		template<typename T>
		[[nodiscard]] std::optional<T>
		getBody() const
		{
			return Slicer::DeserializeAnyWith<T>(getDeserializer());
		}

		template<typename T>
		[[nodiscard]] std::optional<T>
		getBodyParam(const std::optional<IceSpider::StringMap> & map, const std::string_view key) const
		{
			if (!map) {
				return {};
			}

			if (const auto i = map->find(key); i != map->end()) {
				return convert<T>(i->second);
			}
			return {};
		}

		void responseRedirect(const std::string_view url, const OptionalString & = {}) const;
		void setCookie(const std::string_view, const std::string_view, const OptionalString & = {},
				const OptionalString & = {}, bool = false, std::optional<time_t> = {});

		template<typename T>
		void
		setCookie(const std::string_view n, const T & v, const OptionalString & d, const OptionalString & p, bool s,
				std::optional<time_t> e)
		{
			if constexpr (std::is_constructible_v<std::string_view, T>) {
				setCookie(n, std::string_view(v), d, p, s, e);
			}
			else if constexpr (requires { std::to_string(v); }) {
				setCookie(n, std::to_string(v), d, p, s, e);
			}
			else {
				setCookie(n, boost::lexical_cast<std::string>(v), d, p, s, e);
			}
		}

		template<typename T>
		[[nodiscard]] std::optional<T>
		getQueryStringParam(const std::string_view key) const
		{
			return getFrom<T, std::string_view>(key, &IHttpRequest::getQueryStringParamStr);
		}

		template<typename T>
		[[nodiscard]] std::optional<T>
		getHeaderParam(const std::string_view key) const
		{
			return getFrom<T, std::string_view>(key, &IHttpRequest::getHeaderParamStr);
		}

		template<typename T>
		[[nodiscard]] std::optional<T>
		getCookieParam(const std::string_view key) const
		{
			return getFrom<T, std::string_view>(key, &IHttpRequest::getCookieParamStr);
		}

		virtual void response(short, const std::string_view) const = 0;

		template<typename T>
		void
		response(const IRouteHandler * route, const T & t) const
		{
			modelPartResponse(route, Slicer::ModelPart::CreateRootFor(t));
		}

		void modelPartResponse(const IRouteHandler * route, const Slicer::ModelPartForRootPtr &) const;

		const Core * core;
	};
}
