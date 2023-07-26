#include "cgiRequest.h"
#include <iostream>
#include <span>

namespace IceSpider {
	CgiRequest::CgiRequest(Core * c, int argc, char ** argv, char ** env) :
		CgiRequestBase(c, EnvNTL {env}, EnvArray {argv, static_cast<size_t>(argc)})
	{
		initialize();
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
