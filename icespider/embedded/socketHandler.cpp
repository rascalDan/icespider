#include "socketHandler.h"
#include "embedded.h"
#include <boost/assert.hpp>
#include <unistd.h>

namespace IceSpider::Embedded {
	SocketHandler::SocketHandler(int f) :
		fd(f)
	{
		if (fd < 0) {
			throw std::runtime_error("Invalid socket");
		}
	}

	SocketHandler::~SocketHandler()
	{
		close(fd);
	}

	int SocketHandler::except(Listener *)
	{
		return -1;
	}
}

