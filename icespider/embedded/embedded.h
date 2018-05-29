#ifndef ICESPIDER_EMBEDDED_H
#define ICESPIDER_EMBEDDED_H

#include <sys/select.h>
#include <netinet/in.h>
#include <visibility.h>
#include <memory>
#include <array>
#include <vector>
#include <future>
#include </usr/include/semaphore.h>
#include <blockingconcurrentqueue.h>

namespace IceSpider::Embedded {
	class Listener;

	enum class FDSetChange {
		NoChange,
		AddNew,
		Remove,
	};
	typedef std::tuple<FDSetChange> SocketEventResult;
	typedef std::future<SocketEventResult> SocketEventResultFuture;
	typedef std::tuple<int, SocketEventResultFuture> FdSocketEventResultFuture;

	class SocketHandler {
		public:
			typedef std::packaged_task<SocketEventResult()> Work;

			SocketHandler(int f);
			~SocketHandler();

			static inline FdSocketEventResultFuture returnNow(int, const SocketEventResult &&);
			static inline FdSocketEventResultFuture returnQueued(Listener *, int, Work &&);

			virtual FdSocketEventResultFuture read(Listener *) = 0;
			virtual FdSocketEventResultFuture except(Listener *);

			const int fd;
	};

	class ClientSocket : public SocketHandler {
		public:
			ClientSocket(int fd);

			FdSocketEventResultFuture read(Listener * listener) override;

		private:
			inline void read_headers(int bytes);
			inline void stream_input(int bytes);

			enum class State {
				reading_headers,
				streaming_input,
			};

			struct sockaddr_in clientaddr;
			std::vector<char> buf;
			std::size_t rec;
			State state;
	};

	class ListenSocket : public SocketHandler {
		public:
			ListenSocket(unsigned short portno);

			FdSocketEventResultFuture read(Listener * listener) override;

		private:
			struct sockaddr_in serveraddr;
	};

	class DLL_PUBLIC Listener {
		public:
			typedef moodycamel::BlockingConcurrentQueue<SocketHandler::Work> WorkQueue;

			Listener();
			Listener(unsigned short portno);
			~Listener();

			int listen(unsigned short portno);
			void unlisten(int fd);

			void run();

			template<typename T, typename ... P> inline int create(const P & ... p);

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

