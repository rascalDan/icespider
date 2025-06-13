#pragma once

#include "cgiRequestBase.h"
#include <fcgiapp.h>
#include <fcgio.h>
#include <iosfwd>

namespace IceSpider {
	class Core;

	class FcgiRequest : public CgiRequestBase {
	public:
		FcgiRequest(Core * core, FCGX_Request * req);

		std::istream & getInputStream() const override;
		std::ostream & getOutputStream() const override;

	private:
		fcgi_streambuf inputbuf;
		mutable std::istream input;
		fcgi_streambuf outputbuf;
		mutable std::ostream output;
	};
}
