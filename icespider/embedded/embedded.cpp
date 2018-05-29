#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "embedded.h"
#include <lockHelpers.h>
#include <functional>
#include <future>
#include <boost/assert.hpp>

static socklen_t clientlen = sizeof(struct sockaddr_in);

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

	ListenSocket::ListenSocket(unsigned short portno) :
		SocketHandler(socket(AF_INET, SOCK_STREAM, 0))
	{
		int optval = 1;
		setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval , sizeof(int));

		bzero(&serveraddr, sizeof(serveraddr));
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

	FdSocketEventResultFuture ListenSocket::read(Listener * listener)
	{
		return returnNow(listener->create<ClientSocket>(fd), FDSetChange::AddNew);
	}

	Listener::Listener() :
		work(1024),
		topSock(0)
	{
		FD_ZERO(&rfds);
		FD_ZERO(&wfds);
		FD_ZERO(&efds);
	}

	Listener::~Listener()
	{
	}

	int Listener::listen(unsigned short portno)
	{
		int fd = create<ListenSocket>(portno);
		add(fd);
		return fd;
	}

	void Listener::unlisten(int fd)
	{
		if (sockets[fd] && dynamic_cast<ListenSocket *>(sockets[fd].get())) {
			remove(fd);
		}
	}

	void Listener::worker()
	{
		while (topSock > 0) {
			try {
				SocketHandler::Work w;
				if (work.wait_dequeue_timed(w, 500000)) {
					w();
				};
			}
			catch (std::exception & e) {
			}
		}
	}

	void Listener::run()
	{
		std::vector<std::thread> workers;
		std::vector<FdSocketEventResultFuture> pending;

		for (auto x = std::thread::hardware_concurrency(); x; x--) {
			workers.emplace_back(&Listener::worker, this);
		}

		while (topSock > 0) {
			auto r = rfds, w = wfds, e = efds;
			struct timeval to = { 0, pending.empty() ? 500000 : 50 };
			if (auto s = select(topSock, &r, &w, &e, &to) > 0) {
				pending.reserve(pending.size() + s);

				for (int fd = 0; fd < topSock; fd++) {
					if (FD_ISSET(fd, &r)) {
						pending.emplace_back(sockets[fd]->read(this));
					}
					else if (FD_ISSET(fd, &e)) {
						pending.emplace_back(sockets[fd]->except(this));
					}
				}
			}

			for (auto p = pending.begin(); p != pending.end(); ) {
				auto & [ fd, ser ] = *p;
				if (ser.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
					auto [ op ] = ser.get();
					switch (op) {
						case FDSetChange::NoChange:
							FD_SET(fd, &rfds);
							break;
						case FDSetChange::AddNew:
							add(fd);
							break;
						case FDSetChange::Remove:
							remove(fd);
							break;
					}
					p = pending.erase(p);
				}
				else {
					FD_CLR(fd, &rfds);
					p++;
				}
			}
		}

		for (auto & t : workers) {
			t.join();
		}
	}

	template<typename T, typename ... P> inline int Listener::create(const P & ... p)
	{
		auto s = std::make_unique<T>(p...);
		topSock = std::max(s->fd + 1, topSock);
		return (sockets[s->fd] = std::move(s))->fd;
	}

	void Listener::add(int fd)
	{
		FD_SET(fd, &rfds);
		FD_SET(fd, &efds);
	}

	void Listener::remove(int fd)
	{
		FD_CLR(fd, &rfds);
		FD_CLR(fd, &wfds);
		FD_CLR(fd, &efds);
		sockets[fd].reset();
		while (topSock > 0 && !sockets[topSock - 1]) {
			--topSock;
		}
	}
}

