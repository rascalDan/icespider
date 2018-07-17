#include "listenSocket.h"
#include "embedded.h"
#include "clientSocket.h"
#include <string.h>
#include <sys/epoll.h>

namespace IceSpider::Embedded {
	ListenSocket::ListenSocket(unsigned short portno) :
		SocketHandler(socket(AF_INET, SOCK_STREAM, 0))
	{
		int optval = 1;
		setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval , sizeof(int));

		memset(&serveraddr, 0, sizeof(serveraddr));
		serveraddr.sin_family = AF_INET;
		serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
		serveraddr.sin_port = htons(portno);

		if (bind(fd, (struct sockaddr *) &serveraddr, sizeof(serveraddr)) < 0) {
			throw std::runtime_error("ERROR on binding");
		}

		if (listen(fd, 5) < 0) {
			throw std::runtime_error("ERROR on listen");
		}
	}

	int ListenSocket::read(Listener * listener)
	{
		int newfd = listener->create<ClientSocket>(fd);
		listener->add(newfd, EPOLLET | EPOLLRDHUP | EPOLLONESHOT | EPOLLIN | EPOLLERR | EPOLLHUP);
		return EPOLLIN | EPOLLET | EPOLLONESHOT;
	}

	int ListenSocket::write(Listener *)
	{
		// Log a warning about this nonsense?
		return 0;
	}
}

