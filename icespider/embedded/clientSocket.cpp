#include "clientSocket.h"
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>

namespace IceSpider::Embedded {
	static socklen_t clientlen = sizeof(struct sockaddr_in);

	ClientSocket::ClientSocket(int pfd) :
		SocketHandler(accept(pfd, (struct sockaddr *) &clientaddr, &clientlen)),
		buf(BUFSIZ),
		rec(0),
		state(State::reading_headers)
	{
	}

	FdSocketEventResultFuture ClientSocket::read(Listener * listener)
	{
		Work w([this]() {
			if (buf.size() == rec) {
				buf.resize(rec * 2);
			}
			auto r = ::read(fd, &buf.at(rec), buf.size() - rec);
			if (r < 1) {
				return FDSetChange::Remove;
			}
			switch (state) {
				case State::reading_headers:
					read_headers(r);
					break;
				case State::streaming_input:
					stream_input(r);
					break;
			}
			return FDSetChange::NoChange;
		});
		return returnQueued(listener, fd, std::move(w));
	}

	void ClientSocket::read_headers(int r)
	{
		buf[rec + r] = 0;
		if (strstr(&buf.at(rec > 3 ? rec - 2 : 0), "\r\n")) {
			auto w = ::write(fd, "HTTP/1.1 204 No content\r\n\r\n", 27);
			(void)w;
			rec = 0;
		}
		rec += r;
	}

	void ClientSocket::stream_input(int)
	{
	}
}

