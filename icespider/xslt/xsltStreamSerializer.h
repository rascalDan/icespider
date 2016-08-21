#ifndef ICESPIDER_CORE_XSLTSTREAMSERIALIZER_H
#define ICESPIDER_CORE_XSLTSTREAMSERIALIZER_H

#include <slicer/xml/serializer.h>
#include <visibility.h>
#include <libxslt/transform.h>

namespace IceSpider {
	class DLL_PUBLIC XsltStreamSerializer : public Slicer::XmlDocumentSerializer {
		public:
			class IceSpiderFactory : public Slicer::StreamSerializerFactory {
				public:
					IceSpiderFactory(const char *);
					~IceSpiderFactory();

					XsltStreamSerializer * create(std::ostream &) const override;

					xsltStylesheet * stylesheet;
			};

			XsltStreamSerializer(std::ostream &, xsltStylesheet *);
			~XsltStreamSerializer();

			void Serialize(Slicer::ModelPartPtr mp) override;

		protected:
			std::ostream & strm;
			xmlpp::Document * doc;
			xsltStylesheet * stylesheet;
	};
}

#endif

