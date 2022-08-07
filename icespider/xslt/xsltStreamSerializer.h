#ifndef ICESPIDER_CORE_XSLTSTREAMSERIALIZER_H
#define ICESPIDER_CORE_XSLTSTREAMSERIALIZER_H

#include <c++11Helpers.h>
#include <filesystem>
#include <iosfwd>
#include <libxslt/xsltInternals.h>
#include <memory>
#include <slicer/modelParts.h>
#include <slicer/serializer.h>
#include <slicer/xml/serializer.h>
#include <visibility.h>

namespace IceSpider {
	class DLL_PUBLIC XsltStreamSerializer : public Slicer::XmlDocumentSerializer {
	public:
		class IceSpiderFactory : public Slicer::StreamSerializerFactory {
		public:
			explicit IceSpiderFactory(const char *);

			Slicer::SerializerPtr create(std::ostream &) const override;

		private:
			std::filesystem::path stylesheetPath;
			mutable std::filesystem::file_time_type stylesheetWriteTime;
			mutable std::unique_ptr<xsltStylesheet, decltype(&xsltFreeStylesheet)> stylesheet;
		};

		XsltStreamSerializer(std::ostream &, xsltStylesheet *);

		void Serialize(Slicer::ModelPartForRootPtr mp) override;

	protected:
		std::ostream & strm;
		xsltStylesheet * stylesheet;
	};
}

#endif
