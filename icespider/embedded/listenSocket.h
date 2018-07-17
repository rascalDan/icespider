#ifndef ICESPIDER_EMBEDDED_LISTENSOCKET_H
#define ICESPIDER_EMBEDDED_LISTENSOCKET_H

#include "socketHandler.h"
#include <netinet/in.h>

namespace IceSpider::Embedded {
	class ListenSocket : public SocketHandler {
		public:
			ListenSocket(unsigned short portno);

			int read(Listener * listener) override;
			int write(Listener * listener) override;

		private:
			struct sockaddr_in serveraddr;
	};
}

#endif

