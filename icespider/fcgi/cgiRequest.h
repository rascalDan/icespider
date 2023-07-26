#pragma once

#include "cgiRequestBase.h"
#include <iosfwd>

namespace IceSpider {
	class Core;

	class CgiRequest : public CgiRequestBase {
	public:
		CgiRequest(Core * c, int argc, char ** argv, char ** env);

		std::istream & getInputStream() const override;
		std::ostream & getOutputStream() const override;
	};
}
