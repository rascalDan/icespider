#ifndef ICESPIDER_CORE_XSLTSTREAMSERIALIZER_H
#define ICESPIDER_CORE_XSLTSTREAMSERIALIZER_H

#include <slicer/xml/serializer.h>
#include <visibility.h>
#include <libxslt/transform.h>
#include <filesystem>

namespace IceSpider {
	class DLL_PUBLIC XsltStreamSerializer : public Slicer::XmlDocumentSerializer {
		public:
			class IceSpiderFactory : public Slicer::StreamSerializerFactory {
				public:
					IceSpiderFactory(const char *);
					~IceSpiderFactory();

					Slicer::SerializerPtr create(std::ostream &) const override;

				private:
					const std::filesystem::path stylesheetPath;
					mutable std::filesystem::file_time_type stylesheetWriteTime;
					mutable xsltStylesheet * stylesheet;
			};

			XsltStreamSerializer(std::ostream &, xsltStylesheet *);
			~XsltStreamSerializer();

			void Serialize(Slicer::ModelPartForRootPtr mp) override;

		protected:
			std::ostream & strm;
			xmlpp::Document * doc;
			xsltStylesheet * stylesheet;
	};
}

#endif

