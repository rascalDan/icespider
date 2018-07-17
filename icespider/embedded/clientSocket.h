#ifndef ICESPIDER_EMBEDDED_CLIENTSOCKET_H
#define ICESPIDER_EMBEDDED_CLIENTSOCKET_H

#include "socketHandler.h"
#include <vector>
#include <netinet/in.h>

namespace IceSpider::Embedded {
	class ClientSocket : public SocketHandler {
		public:
			ClientSocket(int fd);

			int read(Listener * listener) override;
			int write(Listener * listener) override;

		private:
			inline int read_headers();
			inline int stream_input();

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

