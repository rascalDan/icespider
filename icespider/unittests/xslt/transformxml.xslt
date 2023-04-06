<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">
  <xsl:output encoding="utf-8" method="xml" media-type="text/xml" indent="yes"/>
  <xsl:template match="/SomeModel">
    <M>
      <xsl:attribute name="v">
        <xsl:value-of select="value"/>
      </xsl:attribute>
    </M>
  </xsl:template>
</xsl:stylesheet>
