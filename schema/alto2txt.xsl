<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0" 
		xmlns:alto="http://www.loc.gov/standards/alto/ns-v3#">
    
    <xsl:output encoding="UTF-8" method="text" omit-xml-declaration="yes" indent="no"/>

    <xsl:template match="alto:SP">
        <xsl:text> </xsl:text>
    </xsl:template>

    <xsl:template match="alto:TextLine">
        <xsl:text>&#xA;</xsl:text>
        <xsl:apply-templates />
    </xsl:template>
    
    <xsl:template match="alto:TextBlock">
        <xsl:text>&#xA;</xsl:text>
        <xsl:apply-templates />
    </xsl:template>
    
    <xsl:template match="alto:Page">
        <xsl:text>&#xA;</xsl:text>
        <xsl:apply-templates />
    </xsl:template>
    
    <xsl:template match="alto:String">
        <xsl:value-of disable-output-escaping="yes" select="@CONTENT"/>
    </xsl:template>

    <xsl:template match="@*|node()">
        <xsl:apply-templates select="@*|node()" />
    </xsl:template>

</xsl:stylesheet>

