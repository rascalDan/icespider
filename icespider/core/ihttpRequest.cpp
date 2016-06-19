#include "ihttpRequest.h"
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
			getHeaderParam("Content-Type"), getInputStream());
	}

	Slicer::SerializerPtr
	IHttpRequest::getSerializer() const
	{
		return Slicer::StreamSerializerFactory::createNew(
			"application/json", getOutputStream());
	}

#define getParams(T) \
	template<> T IHttpRequest::getURLParam<T>(const std::string & key) const { \
		return boost::lexical_cast<T>(getURLParam(key)); } \
	template<> T IHttpRequest::getQueryStringParam<T>(const std::string & key) const { \
		return boost::lexical_cast<T>(getQueryStringParam(key)); } \
	template<> T IHttpRequest::getHeaderParam<T>(const std::string & key) const { \
		return boost::lexical_cast<T>(getHeaderParam(key)); }
	template<> std::string IHttpRequest::getURLParam<std::string>(const std::string & key) const { \
		return getURLParam(key); }
	template<> std::string IHttpRequest::getQueryStringParam<std::string>(const std::string & key) const { \
		return getQueryStringParam(key); }
	template<> std::string IHttpRequest::getHeaderParam<std::string>(const std::string & key) const { \
		return getHeaderParam(key); }

	getParams(bool);
	getParams(Ice::Short);
	getParams(Ice::Int);
	getParams(Ice::Long);
	getParams(Ice::Float);
	getParams(Ice::Double);
}

