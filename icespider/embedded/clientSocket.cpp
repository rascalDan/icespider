#include "clientSocket.h"
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <slicer/modelPartsTypes.h>

namespace IceSpider::Embedded {
	static socklen_t clientlen = sizeof(struct sockaddr_in);
	static const std::string_view crlf("\r\n");
	static const std::string_view clnsp(": ");

	ClientSocket::ClientSocket(int pfd) :
		ClientSocket(accept4(pfd, (struct sockaddr *) &clientaddr, &clientlen, SOCK_NONBLOCK), false)
	{
	}

	ClientSocket::ClientSocket(int fd, bool) :
		SocketHandler(fd),
		buf(BUFSIZ, '\0'),
		rec(0),
		state(State::reading_headers)
	{
	}

	int ClientSocket::read(Listener *)
	{
			switch (state) {
				case State::reading_headers:
					return read_headers();
					break;
				case State::streaming_input:
					return stream_input();
					break;
			}
			return -1;
	}

	int ClientSocket::read_headers()
	{
		auto readBytes = [this]() {
			if (buf.size() == rec) {
				buf.resize(rec * 2);
			}
			return ::read(fd, &buf.at(rec), buf.size() - rec);
		};
		int r;
		while ((r = readBytes()) > 0) {
			// Recommended ending
			if (auto end = buf.find("\r\n\r\n", rec > 4 ? rec - 4 : 0, 4); end != std::string::npos) {
				process_request(std::string_view(buf).substr(0, end));
				rec = 0;
				buf.erase(0, end + 3);
			}
			// Alternative ending
			else if (auto end = buf.find("\r\r", rec > 1 ? rec - 2 : 0, 2); end != std::string::npos) {
				process_request(std::string_view(buf).substr(0, end));
				rec = 0;
				buf.erase(0, end + 1);
			}
			else {
				rec += r;
			}
		}
		if (r < 1 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
			return EPOLLET | EPOLLRDHUP | EPOLLONESHOT | EPOLLIN | EPOLLERR | EPOLLHUP;
		}
		return -1;
	}

	void ClientSocket::process_request(const std::string_view & hdrs)
	{
		parse_request(hdrs);
		auto w = ::write(fd, "HTTP/1.1 204 No content\r\n\r\n", 27);
		(void)w;
		(void)hdrs;
	}

	void ClientSocket::parse_request(std::string_view hdrs)
	{
		auto consumeUpto = [&hdrs](auto endAt, auto skip) {
			if (auto end = hdrs.find_first_of(endAt); end != std::string_view::npos) {
				auto rtn = hdrs.substr(0, end);
				if (auto skipTo = hdrs.find_first_not_of(skip, end); skipTo != std::string_view::npos) {
					end = skipTo;
				}
				hdrs.remove_prefix(end);
				return rtn;
			}
			else {
				auto rtn = hdrs;
				hdrs.remove_prefix(hdrs.length());
				return rtn;
			}
		};
		method = Slicer::ModelPartForEnum<::IceSpider::HttpMethod>::lookup(
				//TODO: string copy
				std::string(consumeUpto(' ', ' ')));
		url = consumeUpto(' ', ' ');
		version = consumeUpto(crlf, crlf);
		while (!hdrs.empty()) {
			auto n = consumeUpto(':', clnsp);
			auto v = consumeUpto(crlf, crlf);
			headers.insert_or_assign(n, v);
		}
	}

	int ClientSocket::stream_input()
	{
		return 0;
	}

	int ClientSocket::write(Listener *)
	{
		return 0;
	}
}

