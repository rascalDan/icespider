#ifndef ICESPIDER_EMBEDDED_H
#define ICESPIDER_EMBEDDED_H

#include <visibility.h>
#include <memory>
#include <array>
#include <vector>
#include </usr/include/semaphore.h>
#include <blockingconcurrentqueue.h>
#include "socketHandler.h"

namespace IceSpider::Embedded {
	class DLL_PUBLIC Listener {
		public:
			typedef moodycamel::BlockingConcurrentQueue<SocketHandler::Work> WorkQueue;

			Listener();
			Listener(unsigned short portno);
			~Listener();

			int listen(unsigned short portno);
			void unlisten(int fd);

			void run();

			template<typename T, typename ... P> inline int create(const P & ... p)
			{
				auto s = std::make_unique<T>(p...);
				topSock = std::max(s->fd + 1, topSock);
				return (sockets[s->fd] = std::move(s))->fd;
			}

			WorkQueue work;

		private:
			inline void add(int fd);
			inline void remove(int fd);

			void worker();

			typedef std::unique_ptr<SocketHandler> SocketPtr;
			typedef std::array<SocketPtr, 1024> Sockets;
			int topSock;
			Sockets sockets;
			fd_set rfds, wfds, efds;
	};
};

#endif

