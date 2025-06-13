#include "cgiRequest.h"
#include <iostream>

namespace IceSpider {
	CgiRequest::CgiRequest(Core * core, int argc, char ** argv, char ** env) :
		CgiRequestBase(core, EnvNTL {env}, EnvArray {argv, static_cast<size_t>(argc)})
	{
	}

	std::istream &
	CgiRequest::getInputStream() const
	{
		return std::cin;
	}

	std::ostream &
	CgiRequest::getOutputStream() const
	{
		return std::cout;
	}
}
