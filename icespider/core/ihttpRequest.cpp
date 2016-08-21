#include "ihttpRequest.h"
#include "irouteHandler.h"
#include "util.h"
#include <boost/lexical_cast.hpp>

namespace IceSpider {
	IHttpRequest::IHttpRequest(const Core * c) :
		core(c)
	{
	}

	Ice::Context
	IHttpRequest::getContext() const
	{
		return Ice::Context();
	}

	Slicer::DeserializerPtr
	IHttpRequest::getDeserializer() const
	{
		return Slicer::StreamDeserializerFactory::createNew(
			getHeaderParam("Content-Type") / []() -> std::string {
				throw std::runtime_error("Content-Type must be specified to deserialize payload");
			}, getInputStream());
	}

	ContentTypeSerializer
	IHttpRequest::getSerializer(const IRouteHandler * handler) const
	{
		auto acceptHdr = getHeaderParam("Accept");
		if (acceptHdr) {
			auto accept = acceptHdr->c_str();
			std::vector<AcceptPtr> accepts;
			accepts.reserve(5);
			char grp[BUFSIZ], type[BUFSIZ];
			float pri = 0.0f;
			int chars, v;
			while ((v = sscanf(accept, " %[^ /] / %[^ ;,] %n , %n", grp, type, &chars, &chars)) == 2) {
				accept += chars;
				chars = 0;
				auto a = new Accept();
				if ((v = sscanf(accept, " ; q = %f %n , %n", &pri, &chars, &chars)) == 1) {
					a->q = pri;
				}
				if (strcmp(grp, "*")) {
					a->group = grp;
				}
				if (strcmp(type, "*")) {
					a->type = type;
				}
				accept += chars;
				accepts.push_back(a);
			}
			std::stable_sort(accepts.begin(), accepts.end(), [](const auto & a, const auto & b) { return a->q > b->q; });
			auto & strm = getOutputStream();
			for(auto & a : accepts) {
				ContentTypeSerializer serializer = handler->getSerializer(a, strm);
				if (serializer.second) {
					return serializer;
				}
			}
			return ContentTypeSerializer();
		}
		else {
			return handler->defaultSerializer(getOutputStream());
		}
	}

	const std::string &
	IHttpRequest::getURLParam(unsigned int idx) const
	{
		auto & url = getRequestPath();
		if (idx >= url.size()) {
			throw std::runtime_error("Bad Url parameter index");
		}
		return url[idx];
	}

	template <typename T>
	inline IceUtil::Optional<T> optionalLexicalCast(const IceUtil::Optional<std::string> & p)
	{
		if (p) return boost::lexical_cast<T>(*p);
		return IceUtil::Optional<T>();
	}

	void IHttpRequest::response(short statusCode, const std::string & statusMsg) const
	{
		getOutputStream()
			<< "Status: " << statusCode << " " << statusMsg << "\r\n"
			<< "\r\n";
	}


#define getParams(T) \
	template<> T IHttpRequest::getURLParam<T>(unsigned int idx) const { \
		return boost::lexical_cast<T>(getURLParam(idx)); } \
	template<> IceUtil::Optional<T> IHttpRequest::getQueryStringParam<T>(const std::string & key) const { \
		return optionalLexicalCast<T>(getQueryStringParam(key)); } \
	template<> IceUtil::Optional<T> IHttpRequest::getHeaderParam<T>(const std::string & key) const { \
		return optionalLexicalCast<T>(getHeaderParam(key)); }
	template<> std::string IHttpRequest::getURLParam<std::string>(unsigned int idx) const {
		return getURLParam(idx); }
	template<> IceUtil::Optional<std::string> IHttpRequest::getQueryStringParam<std::string>(const std::string & key) const { \
		return getQueryStringParam(key); }
	template<> IceUtil::Optional<std::string> IHttpRequest::getHeaderParam<std::string>(const std::string & key) const { \
		return getHeaderParam(key); }

	getParams(bool);
	getParams(Ice::Short);
	getParams(Ice::Int);
	getParams(Ice::Long);
	getParams(Ice::Float);
	getParams(Ice::Double);
}

