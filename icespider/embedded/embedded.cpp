#include "embedded.h"
#include "listenSocket.h"

namespace IceSpider::Embedded {
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

