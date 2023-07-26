#pragma once

#include "cgiRequestBase.h"
#include <fcgiapp.h>
#include <fcgio.h>
#include <iosfwd>

namespace IceSpider {
	class Core;

	class FcgiRequest : public CgiRequestBase {
	public:
		FcgiRequest(Core * c, FCGX_Request * r);

		std::istream & getInputStream() const override;
		std::ostream & getOutputStream() const override;

		fcgi_streambuf inputbuf;
		mutable std::istream input;
		fcgi_streambuf outputbuf;
		mutable std::ostream output;
	};
}
