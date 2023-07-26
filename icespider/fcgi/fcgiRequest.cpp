#include "fcgiRequest.h"

namespace IceSpider {
	FcgiRequest::FcgiRequest(Core * c, FCGX_Request * r) :
		CgiRequestBase(c, EnvNTL {r->envp}), inputbuf(r->in), input(&inputbuf), outputbuf(r->out), output(&outputbuf)
	{
		initialize();
	}

	std::istream &
	FcgiRequest::getInputStream() const
	{
		return input;
	}

	std::ostream &
	FcgiRequest::getOutputStream() const
	{
		return output;
	}
}
