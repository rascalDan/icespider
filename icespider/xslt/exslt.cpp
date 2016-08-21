#include <libexslt/exslt.h>
#include <libxslt/transform.h>

static void initLibXml() __attribute__((constructor(102)));
void initLibXml()
{
	xmlInitParser();
	exsltRegisterAll();
}

static void cleanupLibXml() __attribute__((destructor(102)));
void cleanupLibXml()
{
	xsltCleanupGlobals();
	xmlCleanupParser();
}

