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
	namespace {
		int
		xmlstrmclosecallback(void * context)
		{
			static_cast<std::ostream *>(context)->flush();
			return 0;
		}

		int
		xmlstrmwritecallback(void * context, const char * buffer, int len)
		{
			static_cast<std::ostream *>(context)->write(buffer, len);
			return len;
		}
	}

	XsltStreamSerializer::IceSpiderFactory::IceSpiderFactory(const char * path) :
		stylesheetPath(path), stylesheetWriteTime(std::filesystem::file_time_type::min()), stylesheet(nullptr, nullptr)
	{
	}

	Slicer::SerializerPtr
	XsltStreamSerializer::IceSpiderFactory::create(std::ostream & strm) const
	{
		auto newMtime = std::filesystem::last_write_time(stylesheetPath);
		if (newMtime != stylesheetWriteTime) {
			stylesheet = {xsltParseStylesheetFile(reinterpret_cast<const unsigned char *>(stylesheetPath.c_str())),
					xsltFreeStylesheet};
			if (!stylesheet) {
				throw xmlpp::exception("Failed to load stylesheet");
			}
			stylesheetWriteTime = newMtime;
		}
		return std::make_shared<XsltStreamSerializer>(strm, stylesheet.get());
	}

	XsltStreamSerializer::XsltStreamSerializer(std::ostream & os, xsltStylesheet * ss) : strm(os), stylesheet(ss) { }

	void
	XsltStreamSerializer::Serialize(Slicer::ModelPartForRootPtr mp)
	{
		Slicer::XmlDocumentSerializer::Serialize(mp);
		const auto result = std::unique_ptr<xmlDoc, decltype(&xmlFreeDoc)> {
				xsltApplyStylesheet(stylesheet, doc.cobj(), nullptr), &xmlFreeDoc};
		if (!result) {
			throw xmlpp::exception("Failed to apply XSL transform");
		}
		auto buf = std::unique_ptr<xmlOutputBuffer, decltype(&xmlOutputBufferClose)> {
				xmlOutputBufferCreateIO(xmlstrmwritecallback, xmlstrmclosecallback, &strm, nullptr),
				&xmlOutputBufferClose};
		if (xmlStrcmp(stylesheet->method, reinterpret_cast<const unsigned char *>("html")) == 0) {
			htmlDocContentDumpFormatOutput(
					buf.get(), result.get(), reinterpret_cast<const char *>(stylesheet->encoding), 0);
		}
		else {
			xmlSaveFormatFileTo(buf.release(), result.get(), reinterpret_cast<const char *>(stylesheet->encoding), 0);
		}
	}
}
