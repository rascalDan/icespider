#include "fcgiRequest.h"

namespace IceSpider {
	FcgiRequest::FcgiRequest(Core * core, FCGX_Request * req) :
		CgiRequestBase(core, EnvNTL {req->envp}), inputbuf(req->in), input(&inputbuf), outputbuf(req->out),
		output(&outputbuf)
	{
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
