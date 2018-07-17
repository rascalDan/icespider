#include "embedded.h"
#include "listenSocket.h"
#include <unistd.h>
#include <string.h>
#include <sys/epoll.h>
#include <boost/assert.hpp>
#include <thread>

namespace IceSpider::Embedded {
	Listener::Listener() :
		sockCount(0),
		epollfd(epoll_create1(0))
	{
	}

	Listener::~Listener()
	{
		close(epollfd);
	}

	int Listener::listen(unsigned short portno)
	{
		int fd = create<ListenSocket>(portno);
		add(fd, EPOLLIN | EPOLLET | EPOLLONESHOT);
		return fd;
	}

	void Listener::unlisten(int fd)
	{
		if (sockets[fd] && dynamic_cast<ListenSocket *>(sockets[fd].get())) {
			sockCount--;
			remove(fd);
			sockets[fd].reset();
		}
	}

	void Listener::run()
	{
		std::vector<std::thread> workers;

		for (auto x = std::thread::hardware_concurrency(); x; x--) {
			workers.emplace_back([this]() {
				while (sockCount) {
					std::array<epoll_event, 4> events;
					if (auto s = epoll_wait(epollfd, &events.front(), events.size(), 500); s > 0) {
						for (int n = 0; n < s; n++) {
							auto & sh = sockets[events[n].data.fd];
							int act = 0;
							if (events[n].events & (EPOLLRDHUP | EPOLLERR | EPOLLHUP)) {
								act = sh->except(this);
							}
							else {
								if (events[n].events & EPOLLIN) {
									act = sh->read(this);
								}
								if (events[n].events & EPOLLOUT) {
									act = sh->write(this);
								}
							}
							switch (act) {
								case -1:
									sh.reset();
									sockCount--;
									break;
								case 0:
									break;
								default:
									rearm(events[n].data.fd, act);
									break;
							}
						}
					}
				}
			});
		}

		for (auto & t : workers) {
			t.join();
		}
	}

	void Listener::add(int fd, int flags)
	{
		epoll_event ev;
		ev.data.fd = fd;
		ev.events = flags;
		BOOST_VERIFY(epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &ev) == 0);
	}

	void Listener::rearm(int fd, int flags)
	{
		epoll_event ev;
		ev.data.fd = fd;
		ev.events = flags;
		BOOST_VERIFY(epoll_ctl(epollfd, EPOLL_CTL_MOD, fd, &ev) == 0);
	}

	void Listener::remove(int fd)
	{
		BOOST_VERIFY(epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, NULL) == 0);
	}
}

