#include <fcgio.h>

int
main(void)
{
	if (!FCGX_IsCGI()) {
		FCGX_Request request;

		FCGX_Init();
		FCGX_InitRequest(&request, 0, 0);

		while (FCGX_Accept_r(&request) == 0) {
			// app.process(IO, &IO, IO);
			FCGX_Finish_r(&request);
		}
		return 0;
	}
	else {
		return 1;
	}
}

