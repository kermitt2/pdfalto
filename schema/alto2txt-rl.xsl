<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="2.0" 
		xmlns:alto="http://www.loc.gov/standards/alto/ns-v3#">
    
    <xsl:output encoding="UTF-8" method="text" omit-xml-declaration="yes" indent="no"/>

    <xsl:template match="alto:SP">
        <xsl:text> </xsl:text>
    </xsl:template>

    <xsl:template match="alto:TextLine">
        <xsl:text>&#xA;</xsl:text>
        <xsl:variable name="w1" select="./alto:String[position()=1]/@CONTENT" />
        <xsl:choose>        
            <xsl:when test="matches($w1,'[&#x0590;-&#x083F;]|[&#x08A0;-&#x08FF;]|[&#xFB1D;-&#xFDFF;]|[&#xFE70;-&#xFEFF;]')">
                <xsl:for-each select="node()">
                    <xsl:sort select="position()" data-type="number" order="descending"/>
                    <xsl:apply-templates select="."/>
                </xsl:for-each>
            </xsl:when>
            <xsl:otherwise>    
                <xsl:apply-templates />
            </xsl:otherwise>
        </xsl:choose>
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

