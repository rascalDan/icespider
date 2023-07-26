#include <libexslt/exslt.h>
#include <libxml/parser.h>
#include <libxslt/xslt.h>

static void initLibXml() __attribute__((constructor(102)));

void
initLibXml()
{
	xmlInitParser();
	exsltRegisterAll();
}

// LCOV_EXCL_START lcov actually misses destructor functions
static void cleanupLibXml() __attribute__((destructor(102)));

void
cleanupLibXml()
{
	xsltCleanupGlobals();
	xmlCleanupParser();
}

// LCOV_EXCL_STOP
