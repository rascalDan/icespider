#ifndef ICESPIDER_EMBEDDED_SOCKETHANDLER_H
#define ICESPIDER_EMBEDDED_SOCKETHANDLER_H

#include <future>
#include "socketEvents.h"

namespace IceSpider::Embedded {
	class Listener;

	class SocketHandler {
		public:
			typedef std::packaged_task<SocketEventResult()> Work;

			SocketHandler(int f);
			~SocketHandler();

			static FdSocketEventResultFuture returnNow(int, const SocketEventResult &&);
			static FdSocketEventResultFuture returnQueued(Listener *, int, Work &&);

			virtual FdSocketEventResultFuture read(Listener *) = 0;
			virtual FdSocketEventResultFuture except(Listener *);

			const int fd;
	};
}

#endif

