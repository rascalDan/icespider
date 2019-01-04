#ifndef ICESPIDER_EMBEDDED_CLIENTSOCKET_H
#define ICESPIDER_EMBEDDED_CLIENTSOCKET_H

#include "socketHandler.h"
#include <string>
#include <netinet/in.h>
#include <core.h>

namespace IceSpider::Embedded {
	class DLL_PUBLIC ClientSocket : public SocketHandler {
		public:
			ClientSocket(int fd);

			int read(Listener * listener) override;
			int write(Listener * listener) override;

		protected:
			ClientSocket(int fd, bool unused);
			inline int read_headers();
			inline int stream_input();
			void process_request(const std::string_view & hdrs);
			void parse_request(std::string_view hdrs);

			enum class State {
				reading_headers,
				streaming_input,
			};

			struct sockaddr_in clientaddr;
			std::string buf;
			std::size_t rec;
			State state;

			::IceSpider::HttpMethod method;
			std::string_view url, version;
			typedef std::map<std::string_view, std::string_view> Headers;
			Headers headers;
	};

}

#endif

