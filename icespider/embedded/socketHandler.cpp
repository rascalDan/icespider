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

	FdSocketEventResultFuture SocketHandler::returnNow(int fd, const SocketEventResult && ser)
	{
		std::promise<SocketEventResult> p;
		p.set_value(ser);
		return { fd, p.get_future() };
	}

	FdSocketEventResultFuture SocketHandler::returnQueued(Listener * listener, int fd, Work && work)
	{
		auto f = work.get_future();
		BOOST_VERIFY_MSG(listener->work.try_enqueue(std::move(work)), "try_enqueue");
		return { fd, std::move(f) };
	}

	FdSocketEventResultFuture SocketHandler::except(Listener *)
	{
		return returnNow(fd, FDSetChange::Remove);
	}
}

