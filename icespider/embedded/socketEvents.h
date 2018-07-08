#ifndef ICESPIDER_EMBEDDED_SOCKETEVENTS_H
#define ICESPIDER_EMBEDDED_SOCKETEVENTS_H

#include <tuple>
#include <future>

namespace IceSpider::Embedded {
	enum class FDSetChange {
		NoChange,
		AddNew,
		Remove,
	};
	typedef std::tuple<FDSetChange> SocketEventResult;
	typedef std::future<SocketEventResult> SocketEventResultFuture;
	typedef std::tuple<int, SocketEventResultFuture> FdSocketEventResultFuture;
}

#endif

