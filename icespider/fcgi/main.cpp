#include "cgiRequest.h"
#include "fcgiRequest.h"
#include <core.h>
#include <fcgiapp.h>
#include <http.h>
#include <visibility.h>

using namespace IceSpider;

DLL_PUBLIC
int
main(int argc, char ** argv, char ** env)
{
	CoreWithDefaultRouter core;
	if (!FCGX_IsCGI()) {
		FCGX_Request request;

		FCGX_Init();
		FCGX_InitRequest(&request, 0, 0);

		while (FCGX_Accept_r(&request) == 0) {
			FcgiRequest req(&core, &request);
			core.process(&req);
			FCGX_Finish_r(&request);
		}
	}
	else {
		CgiRequest req(&core, argc, argv, env);
		core.process(&req);
	}
	return 0;
}
