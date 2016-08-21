<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">
  <xsl:output encoding="utf-8" method="html" media-type="text/html" indent="yes"/>
  <xsl:template match="/SomeModel">
    <html>
      <head>
        <title>Some Model</title>
      </head>
      <body>
        <p><b>value</b>: <xsl:value-of select="value"/></p>
      </body>
    </html>
  </xsl:template>
</xsl:stylesheet>
