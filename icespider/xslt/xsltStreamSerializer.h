#ifndef ICESPIDER_CORE_XSLTSTREAMSERIALIZER_H
#define ICESPIDER_CORE_XSLTSTREAMSERIALIZER_H

#include <c++11Helpers.h>
#include <filesystem>
#include <iosfwd>
#include <libxslt/xsltInternals.h>
#include <slicer/modelParts.h>
#include <slicer/serializer.h>
#include <slicer/xml/serializer.h>
#include <visibility.h>
namespace xmlpp {
	class Document;
}

namespace IceSpider {
	class DLL_PUBLIC XsltStreamSerializer : public Slicer::XmlDocumentSerializer {
	public:
		class IceSpiderFactory : public Slicer::StreamSerializerFactory {
		public:
			explicit IceSpiderFactory(const char *);
			SPECIAL_MEMBERS_MOVE_RO(IceSpiderFactory);
			~IceSpiderFactory() override;

			Slicer::SerializerPtr create(std::ostream &) const override;

		private:
			const std::filesystem::path stylesheetPath;
			mutable std::filesystem::file_time_type stylesheetWriteTime;
			mutable xsltStylesheet * stylesheet;
		};

		XsltStreamSerializer(std::ostream &, xsltStylesheet *);
		SPECIAL_MEMBERS_DELETE(XsltStreamSerializer);
		~XsltStreamSerializer() override;

		void Serialize(Slicer::ModelPartForRootPtr mp) override;

	protected:
		std::ostream & strm;
		xmlpp::Document * doc;
		xsltStylesheet * stylesheet;
	};
}

#endif
