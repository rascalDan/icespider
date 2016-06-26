#include "cgiRequest.h"

namespace IceSpider {
	CgiRequest::CgiRequest(Core * c, int argc, char ** argv, char ** env) :
		CgiRequestBase(c, env)
	{
		for (; argc > 0;) {
			addenv(argv[--argc]);
		}
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

