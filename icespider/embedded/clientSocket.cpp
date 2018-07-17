#include "clientSocket.h"
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/epoll.h>

namespace IceSpider::Embedded {
	static socklen_t clientlen = sizeof(struct sockaddr_in);

	ClientSocket::ClientSocket(int pfd) :
		SocketHandler(accept4(pfd, (struct sockaddr *) &clientaddr, &clientlen, SOCK_NONBLOCK)),
		buf(BUFSIZ),
		rec(0),
		state(State::reading_headers)
	{
	}

	int ClientSocket::read(Listener *)
	{
			switch (state) {
				case State::reading_headers:
					return read_headers();
					break;
				case State::streaming_input:
					return stream_input();
					break;
			}
			return -1;
	}

	int ClientSocket::read_headers()
	{
		auto readBytes = [this]() {
			if (buf.size() == rec) {
				buf.resize(rec * 2);
			}
			return ::read(fd, &buf.at(rec), buf.size() - rec);
		};
		int r;
		while ((r = readBytes()) > 0) {
			buf[rec + r] = 0;
			auto end = strstr(&buf.at(rec > 3 ? rec - 2 : 0), "\r\n");
			if (end) {
				auto w = ::write(fd, "HTTP/1.1 204 No content\r\n\r\n", 27);
				(void)w;
				rec = 0;
				buf.erase(buf.begin(), buf.begin() + (end - &buf.front()));
			}
			else {
				rec += r;
			}
		}
		if (r < 1 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
			return EPOLLET | EPOLLRDHUP | EPOLLONESHOT | EPOLLIN | EPOLLERR | EPOLLHUP;
		}
		return -1;
	}

	int ClientSocket::stream_input()
	{
		return 0;
	}

	int ClientSocket::write(Listener *)
	{
		return 0;
	}
}

