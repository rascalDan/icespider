#ifndef ICESPIDER_EMBEDDED_CLIENTSOCKET_H
#define ICESPIDER_EMBEDDED_CLIENTSOCKET_H

#include "socketHandler.h"
#include <vector>
#include <netinet/in.h>

namespace IceSpider::Embedded {
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

}

#endif

