#ifndef ICESPIDER_EMBEDDED_H
#define ICESPIDER_EMBEDDED_H

#include <visibility.h>
#include <memory>
#include <array>
#include <vector>
#include "socketHandler.h"

namespace IceSpider::Embedded {
	class DLL_PUBLIC Listener {
		public:
			Listener();
			Listener(unsigned short portno);
			~Listener();

			int listen(unsigned short portno);
			void unlisten(int fd);

			void run();

			template<typename T, typename ... P> inline int create(const P & ... p)
			{
				auto s = std::make_unique<T>(p...);
				sockCount++;
				return (sockets[s->fd] = std::move(s))->fd;
			}

			void add(int fd, int flags);
			void rearm(int fd, int flags);
			void remove(int fd);

		private:
			typedef std::unique_ptr<SocketHandler> SocketPtr;
			typedef std::array<SocketPtr, 1024> Sockets;
			int sockCount, epollfd;
			Sockets sockets;
	};
};

#endif

