#ifndef ICESPIDER_EMBEDDED_SOCKETHANDLER_H
#define ICESPIDER_EMBEDDED_SOCKETHANDLER_H

namespace IceSpider::Embedded {
	class Listener;

	class SocketHandler {
		public:
			SocketHandler(int f);
			~SocketHandler();

			virtual int read(Listener *) = 0;
			virtual int write(Listener *) = 0;
			virtual int except(Listener *);

			const int fd;
	};
}

#endif

