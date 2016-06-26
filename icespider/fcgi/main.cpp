#include <visibility.h>
#include "fcgiRequest.h"
#include "cgiRequest.h"

using namespace IceSpider;

DLL_PUBLIC
int
main(int argc, char ** argv, char ** env)
{
	Core core(argc, argv);
	if (!FCGX_IsCGI()) {
		FCGX_Request request;

		FCGX_Init();
		FCGX_InitRequest(&request, 0, 0);

		while (FCGX_Accept_r(&request) == 0) {
			FcgiRequest r(&core, &request);
			core.process(&r);
			FCGX_Finish_r(&request);
		}
	}
	else {
		CgiRequest r(&core, argc, argv, env);
		core.process(&r);
	}
	return 0;
}

