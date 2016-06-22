#ifndef ICESPIDER_CGI_CGIREQUEST_H
#define ICESPIDER_CGI_CGIREQUEST_H

#include "cgiRequestBase.h"

namespace IceSpider {
	class CgiRequest : public CgiRequestBase {
		public:
			CgiRequest(IceSpider::Core * c, int argc, char ** argv, char ** env);

			std::istream & getInputStream() const override;
			std::ostream & getOutputStream() const override;
	};
}

#endif

