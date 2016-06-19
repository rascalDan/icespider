#include "ihttpRequest.h"

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
}

