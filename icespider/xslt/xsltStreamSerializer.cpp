#include "xsltStreamSerializer.h"
#include <libxslt/xsltInternals.h>
#include <libxml/HTMLtree.h>
#include <factory.impl.h>
#include <boost/filesystem/convenience.hpp>

namespace IceSpider {
	static int xmlstrmclosecallback(void * context)
	{
		((std::ostream*)context)->flush();
		return 0;
	}

	static int xmlstrmwritecallback(void * context, const char * buffer, int len)
	{
		((std::ostream*)context)->write(buffer, len);
		return len;
	}

	XsltStreamSerializer::IceSpiderFactory::IceSpiderFactory(const char * path) :
		stylesheetPath(path),
		stylesheetWriteTime(0),
		stylesheet(nullptr)
	{
	}

	XsltStreamSerializer::IceSpiderFactory::~IceSpiderFactory()
	{
		xsltFreeStylesheet(stylesheet);
	}

	Slicer::SerializerPtr
	XsltStreamSerializer::IceSpiderFactory::create(std::ostream & strm) const
	{
		auto newMtime = boost::filesystem::last_write_time(stylesheetPath);
		if (newMtime != stylesheetWriteTime) {
			if (stylesheet) {
				xsltFreeStylesheet(stylesheet);
			}
			stylesheet = xsltParseStylesheetFile(BAD_CAST stylesheetPath.c_str());
			if (!stylesheet) {
				throw xmlpp::exception("Failed to load stylesheet");
			}
			stylesheetWriteTime = newMtime;
		}
		return std::make_shared<XsltStreamSerializer>(strm, stylesheet);
	}

	XsltStreamSerializer::XsltStreamSerializer(std::ostream & os, xsltStylesheet * ss) :
		Slicer::XmlDocumentSerializer(doc),
		strm(os),
		doc(nullptr),
		stylesheet(ss)
	{
	}

	XsltStreamSerializer::~XsltStreamSerializer()
	{
		delete doc;
	}

	void
	XsltStreamSerializer::Serialize(Slicer::ModelPartForRootPtr mp)
	{
		Slicer::XmlDocumentSerializer::Serialize(mp);
		auto result = xsltApplyStylesheet(stylesheet, doc->cobj(), nullptr);
		if (!result) {
			throw xmlpp::exception("Failed to apply XSL transform");
		}
		xmlOutputBufferPtr buf = xmlOutputBufferCreateIO(xmlstrmwritecallback, xmlstrmclosecallback, &strm, NULL);
		if (xmlStrcmp(stylesheet->method, BAD_CAST "html") == 0) {
			htmlDocContentDumpFormatOutput(buf, result, (const char *) stylesheet->encoding, 0);
		}
		else {
			xmlNodeDumpOutput(buf, result, result->children, 0, 0, (const char *) stylesheet->encoding);
		}
		xmlOutputBufferClose(buf);
		xmlFreeDoc(result);
	}
}

