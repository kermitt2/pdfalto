#ifndef CONSTANTSXMLALTO_H_
#define CONSTANTSXMLALTO_H_

/**
  * Contain all constants for tag XML, attributs XML and other informations
  * which are defined in the output file of pdfalto
  */
namespace ConstantsXML {
	// All tags ALTO XML dialect
	extern const char *TAG_ALTO;
	extern const char *ALTO_URI;
	extern const char *ALTO_LOCATION;
	extern const char *TAG_DESCRIPTION;
	extern const char *TAG_MEASUREMENTUNIT;
	extern const char *TAG_SOURCE_IMAGE_INFO;
	extern const char *TAG_PDFFILENAME;
	extern const char *TAG_OCRPROCESSING;
	extern const char *ATTR_VALUEID_OCRPROCESSING;
	extern const char *TAG_OCRPROCESSINGSTEP;
	extern const char *TAG_PROCESSINGDATE;
	extern const char *TAG_PROCESSINGSOFTWARE;
	extern const char *TAG_SOFTWARE_CREATOR;
	extern const char *TAG_SOFTWARE_NAME;
	extern const char *TAG_SOFTWARE_VERSION;
	extern const char *TAG_STYLES;
	extern const char *TAG_LAYOUT;
	extern const char *TAG_PAGE;
	extern const char *TAG_IMAGE;
	extern const char *TAG_SVG;
	extern const char *TAG_BLOCK;
	extern const char *TAG_TEXT;
	extern const char *TAG_PRINTSPACE;
	extern const char *TAG_TOKEN;
	extern const char *ATTR_STYLEREFS;
	extern const char *ATTR_TOKEN_CONTENT;
	extern const char *TAG_SPACING;
	extern const char *TAG_TEXTSTYLE;
	extern const char *ATTR_D;
	extern const char *VERSION;
	extern const char *ENCODING_UTF8;
	// All attributs
	extern const char *ATTR_FONTFAMILY;
	extern const char *ATTR_FONTSIZE;
	extern const char *ATTR_FONTTYPE;
	extern const char *ATTR_FONTWIDTH;
	extern const char *ATTR_FONTCOLOR;
	extern const char *ATTR_FONTSTYLE;
	extern const char *ATTR_WIDTH;
	extern const char *ATTR_HEIGHT;
	extern const char *ATTR_PHYSICAL_IMG_NR;
	extern const char *ATTR_HREF;
	extern const char *ATTR_X;
	extern const char *ATTR_Y;
	extern const char *ATTR_ROTATION;
	extern const char *ATTR_ANGLE;
	extern const char *ATTR_ID;
	// This attribute gives the reading order : right->left (value 0) or left->right (value 1)
	extern const char *ATTR_TYPE;
	extern const char *sTRUE;
	extern const char *sFALSE;
	// Attributs about details informations
	extern const char *ATTR_LEADING;
	extern const char *ATTR_RENDER;
	extern const char *ATTR_RISE;
	extern const char *ATTR_HORIZ_SCALING;
	extern const char *ATTR_WORD_SPACE;
	extern const char *ATTR_CHAR_SPACE;
	extern const char *ATTR_BASE;
	extern const char *ATTR_ANGLE_SKEWING_Y;
	extern const char *ATTR_ANGLE_SKEWING_X;
	extern const char *ATTR_STYLE;
	extern const char *ATTR_EVENODD;
	extern const char *ATTR_CLIPZONE;
	extern const char *ATTR_IDCLIPZONE;
	extern const char *ATTR_ID_ITEM_PARENT;
	extern const char *ATTR_NB_PAGES;
	extern const char *ATTR_LOCATION;

	// SVG XML TAGS
	extern const char *ATTR_NUMFORMAT;
	extern const char *ATTR_SID;
	extern const char *ATTR_SVGID;
	extern const char *ATTR_SVG_X;
	extern const char *ATTR_SVG_Y;
	extern const char *ATTR_SVG_WIDTH;
	extern const char *ATTR_SVG_HEIGHT;
	extern const char *TAG_CLIPPATH;
	extern const char *TAG_GROUP;
	extern const char *TAG_PATH;
	extern const char *TAG_M;
	extern const char *TAG_L;
	extern const char *TAG_C;

	// METADATA XML TAGS
	extern const char *TAG_METADATA;
	extern const char *TAG_PDFFILE_METADATA;
	extern const char *TAG_PROCESS;
	extern const char *TAG_CREATIONDATE;
	extern const char *TAG_COMMENT;

	// OUTLINE XML TAGS
	// Tag XML for file Outline
	extern const char *TAG_TOCITEMS;
	extern const char *TAG_TOCITEMLIST;
	extern const char *TAG_ITEM;
	extern const char *TAG_STRING;
	extern const char *TAG_LINK;
	// Attributs for file Outline
	extern const char *ATTR_LEVEL;
	extern const char *ATTR_PAGE;
	extern const char *ATTR_TOP;
	extern const char *ATTR_BOTTOM;
	extern const char *ATTR_LEFT;
	extern const char *ATTR_RIGHT;

	// ANNOTATIONS XML TAGS
	// Tag XML for annotations file
	extern const char *TAG_ANNOTATIONS;
	extern const char *TAG_ANNOTATION;
	extern const char *TAG_POPUP;
	extern const char *TAG_AUTHOR;
	extern const char *TAG_CONTENT;
	extern const char *TAG_RICH_CONTENT;
	extern const char *TAG_QUADPOINTS;
	extern const char *TAG_QUADRILATERAL;
	extern const char *TAG_POINT;
	extern const char *TAG_ACTION;
	extern const char *TAG_DEST;
	// Attributs for annotations file
	extern const char *ATTR_SUBTYPE;
	extern const char *ATTR_PAGENUM;
	extern const char *ATTR_OPEN;
	// annotations in XML file
	extern const char *ATTR_GOTOLINK;
	extern const char *ATTR_GOTORLINK;
	extern const char *ATTR_ANNOTS_X;
	extern const char *ATTR_ANNOTS_Y;
	extern const char *ATTR_ANNOTS_PAGE;
	extern const char *ATTR_ANNOTS_TYPE;
	extern const char *ATTR_URILINK;
	extern const char *ATTR_HIGHLIGHT;
	extern const char *ATTR_UNDERLINE;

	//PAGE BOXES
//	extern const char *TAG_MEDIABOX;
//	extern const char *TAG_BLEEDBOX;
//	extern const char *TAG_CROPBOX;
//	extern const char *TAG_ARTBOX;
//	extern const char *TAG_TRIMBOX;

	extern const char *TAG_PAR_CONF;
	extern const char *TAG_PAR_TOOL;
	extern const char *TAG_PAR_NAME;
	extern const char *TAG_PAR_VER;
	extern const char *TAG_PAR_DESC;
	extern const char *TAG_PAR_PARAM;
	extern const char *TAG_PAR_ATTRNAME;

}

#endif
