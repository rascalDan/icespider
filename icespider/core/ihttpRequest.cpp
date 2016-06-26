#include "ihttpRequest.h"
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

	Slicer::SerializerPtr
	IHttpRequest::getSerializer() const
	{
		return Slicer::StreamSerializerFactory::createNew(
			"application/json", getOutputStream());
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
			<< statusCode << " " << statusMsg << "\r\n"
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

