#include "xsltStreamSerializer.h"
#include <chrono>
#include <filesystem>
#include <libxml++/document.h>
#include <libxml++/exceptions/exception.h>
#include <libxml/HTMLtree.h>
#include <libxml/tree.h>
#include <libxml/xmlIO.h>
#include <libxml/xmlstring.h>
#include <libxslt/transform.h>
#include <libxslt/xsltInternals.h>
#include <memory>
#include <ostream>
#include <slicer/modelParts.h>
#include <slicer/serializer.h>
#include <slicer/xml/serializer.h>

namespace IceSpider {
	static int
	xmlstrmclosecallback(void * context)
	{
		static_cast<std::ostream *>(context)->flush();
		return 0;
	}

	static int
	xmlstrmwritecallback(void * context, const char * buffer, int len)
	{
		static_cast<std::ostream *>(context)->write(buffer, len);
		return len;
	}

	XsltStreamSerializer::IceSpiderFactory::IceSpiderFactory(const char * path) :
		stylesheetPath(path), stylesheetWriteTime(std::filesystem::file_time_type::min()), stylesheet(nullptr)
	{
	}

	XsltStreamSerializer::IceSpiderFactory::~IceSpiderFactory()
	{
		xsltFreeStylesheet(stylesheet);
	}

	Slicer::SerializerPtr
	XsltStreamSerializer::IceSpiderFactory::create(std::ostream & strm) const
	{
		auto newMtime = std::filesystem::last_write_time(stylesheetPath);
		if (newMtime != stylesheetWriteTime) {
			if (stylesheet) {
				xsltFreeStylesheet(stylesheet);
			}
			stylesheet = xsltParseStylesheetFile(reinterpret_cast<const unsigned char *>(stylesheetPath.c_str()));
			if (!stylesheet) {
				throw xmlpp::exception("Failed to load stylesheet");
			}
			stylesheetWriteTime = newMtime;
		}
		return std::make_shared<XsltStreamSerializer>(strm, stylesheet);
	}

	XsltStreamSerializer::XsltStreamSerializer(std::ostream & os, xsltStylesheet * ss) :
		Slicer::XmlDocumentSerializer(doc), strm(os), doc(nullptr), stylesheet(ss)
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
		xmlOutputBufferPtr buf = xmlOutputBufferCreateIO(xmlstrmwritecallback, xmlstrmclosecallback, &strm, nullptr);
		if (xmlStrcmp(stylesheet->method, reinterpret_cast<const unsigned char *>("html")) == 0) {
			htmlDocContentDumpFormatOutput(buf, result, reinterpret_cast<const char *>(stylesheet->encoding), 0);
		}
		else {
			xmlNodeDumpOutput(
					buf, result, result->children, 0, 0, reinterpret_cast<const char *>(stylesheet->encoding));
		}
		xmlOutputBufferClose(buf);
		xmlFreeDoc(result);
	}
}
