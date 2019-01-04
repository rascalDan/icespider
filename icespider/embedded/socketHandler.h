#ifndef ICESPIDER_EMBEDDED_SOCKETHANDLER_H
#define ICESPIDER_EMBEDDED_SOCKETHANDLER_H

#include <visibility.h>

namespace IceSpider::Embedded {
	class Listener;

	class DLL_PUBLIC SocketHandler {
		public:
			SocketHandler(int f);
			virtual ~SocketHandler();

			virtual int read(Listener *) = 0;
			virtual int write(Listener *) = 0;
			virtual int except(Listener *);

			const int fd;
	};
}

#endif

