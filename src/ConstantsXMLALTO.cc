//========================================================================
//
// ConstantsXMLALTO.h
//
// Contain all constants for tag XML, attributs XML and other informations
// which are defined in the output file of pdftoxml.
//
// author: Achraf Azhar
// 12-2017
//
//========================================================================

#include "ConstantsXMLALTO.h"

namespace ConstantsXMLALTO
{
	// All tags ALTO XML dialect
	const char *TAG_ALTO = "alto";

	const char *ALTO_URI = "http://www.loc.gov/standards/alto/ns-v3#";

	const char *XLINK_PREFIX = "xlink";

	const char *XLINK_URI = "http://www.w3.org/1999/xlink";

	const char *XSI_PREFIX = "xsi";

	const char *XSI_URI = "http://www.w3.org/2001/XMLSchema-instance";

    const char *SCHEMA_LOCATION_ATTR_NAME = "schemaLocation";

    const char *SCHEMA_LOCATION_URI = "http://www.loc.gov/standards/alto/alto-v2.0.xsd";

	const char *TAG_DESCRIPTION = "Description";
    const char *TAG_MEASUREMENTUNIT = "MeasurementUnit";
    const char *TAG_SOURCE_IMAGE_INFO = "sourceImageInformation";
    const char *TAG_PDFFILENAME = "fileName";

    const char *TAG_OCRPROCESSING = "OCRProcessing";
    const char *ATTR_NAMEID_OCRPROCESSING = "ID";
    const char *ATTR_VALUEID_OCRPROCESSING = "IdOcr";

    const char *TAG_OCRPROCESSINGSTEP = "ocrProcessingStep";
    const char *TAG_PROCESSINGDATE = "processingDateTime";
    const char *TAG_PROCESSINGSOFTWARE = "processingSoftware";
    const char *TAG_SOFTWARE_CREATOR = "softwareCreator";
    const char *TAG_SOFTWARE_NAME = "softwareName";
    const char *TAG_SOFTWARE_VERSION = "softwareVersion";


	const char *TAG_STYLES = "Styles";
	const char *TAG_LAYOUT = "Layout";




	const char *TAG_METADATA = "METADATA";
	const char *TAG_PDFFILE_METADATA = "PDF_METADATA";
	const char *TAG_PROCESS = "PROCESS";
	const char *TAG_CREATIONDATE = "CREATIONDATE";
	const char *TAG_COMMENT = "COMMENT";


	const char *TAG_PAGE = "Page";
	const char *TAG_IMAGE = "IMAGE";
	const char *TAG_VECTORIALIMAGES = "VECTORIALIMAGES";
	const char *TAG_BLOCK = "TextBlock";
	const char *TAG_TEXT = "TextLine";

    const char *TAG_PRINTSPACE = "PrintSpace";

	const char *TAG_TOKEN = "String";
    const char *ATTR_TOKEN_CONTENT = "CONTENT";

    const char *TAG_SPACING = "SP";

	const char *TAG_VECTORIALINSTRUCTIONS = "VECTORIALINSTRUCTIONS";
	const char *TAG_GROUP = "GROUP";
	const char *TAG_CLIP = "CLIP";
	const char *TAG_M = "M";
	const char *TAG_L = "L";
	const char *TAG_C = "C";
	
	const char *TAG_PAR_CONF = "CONFIGURATION";
	const char *TAG_PAR_TOOL = "TOOL";
	const char *TAG_PAR_NAME = "NAME";
	const char *TAG_PAR_VER = "VERSION";
	const char *TAG_PAR_DESC = "DESCRIPTION";
	const char *TAG_PAR_PARAM = "PARAM";
	const char *TAG_PAR_ATTRNAME = "NAME";
	

	
	// Tag xi:include
	const char *XI_URI = "http://www.w3.org/2001/XInclude";
	const char *XI_PREFIX = "xi";
	const char *XI_INCLUDE = "include";
	
	const char *VERSION = "1.0";
	const char *ENCODING_UTF8 = "UTF-8";
	
	// Metadata attributes
	const char *ATTR_VALUE = "value";
	const char *ATTR_NAME = "name";
	const char *ATTR_CMD = "cmd";
	
	// All attributs
	const char *ATTR_FORMAT = "format";
	const char *ATTR_WIDTH = "WIDTH";
	const char *ATTR_HEIGHT = "HEIGHT";
	const char *ATTR_PHYSICAL_IMG_NR = "PHYSICAL_IMG_NR";
	const char *ATTR_HREF = "href";
	const char *ATTR_X = "HPOS";
	const char *ATTR_Y = "VPOS";
	
	const char *ATTR_ROTATION = "rotation";
	const char *ATTR_ANGLE = "angle";
	
	// Attributs about font informations
	const char *ATTR_FONT_NAME = "font-name";
	const char *ATTR_FONT_SIZE = "font-size";
	const char *ATTR_FONT_COLOR = "font-color";
	const char *ATTR_BOLD = "bold";
	const char *ATTR_ITALIC = "italic";
	const char *ATTR_ALL_CAP = "all-cap";
	const char *ATTR_SMALL_CAP = "small-cap";
	const char *ATTR_SYMBOLIC = "symbolic";
	const char *ATTR_SERIF = "serif";
	const char *ATTR_FIXED_WIDTH = "fixed-width";

	// Attributs about details informations
	const char *ATTR_LEADING = "leading";
	const char *ATTR_RENDER = "render";
	const char *ATTR_RISE = "rise";
	const char *ATTR_HORIZ_SCALING = "horiz-scaling";
	const char *ATTR_WORD_SPACE = "word-space";
	const char *ATTR_CHAR_SPACE = "char-space";
	const char *ATTR_BASE = "base";
	const char *ATTR_ANGLE_SKEWING_Y = "angle-skewing-y";
	const char *ATTR_ANGLE_SKEWING_X = "angle-skewing-x";

	const char *ATTR_STYLE = "style";
	const char *ATTR_CLOSED = "closed";
	const char *ATTR_EVENODD = "evenodd";
	const char *ATTR_CLIPZONE = "clipZone";
	const char *ATTR_IDCLIPZONE = "idClipZone";
	
	const char *ATTR_INLINE = "inline";
	const char *ATTR_MASK = "mask";
	
	const char *ATTR_ID = "ID";
	const char *ATTR_ID_ITEM_PARENT = "idItemParent";
	const char *ATTR_NB_PAGES = "nbPages";
	
	const char *ATTR_X1 = "x1";
	const char *ATTR_Y1 = "y1";
	const char *ATTR_X2 = "x2";
	const char *ATTR_Y2 = "y2";
	const char *ATTR_X3 = "x3";
	const char *ATTR_Y3 = "y3";
	
	// This attribute gives the reading order : right->left (value 0) or left->right (value 1)
	const char *ATTR_TYPE = "type";
	
	const char *YES = "yes";
	const char *NO = "no";
	
	const char *sTRUE = "true";
	const char *sFALSE = "false";

	
	// Tag XML for file Outline
	const char *TAG_TOCITEMS = "TOCITEMS";
	const char *TAG_TOCITEMLIST = "TOCITEMLIST";
	const char *TAG_ITEM = "ITEM";
	const char *TAG_STRING = "STRING";
	const char *TAG_LINK = "LINK";
	
	// Attributs for file Outline
	const char *ATTR_LEVEL = "level";
	const char *ATTR_PAGE = "page";
	const char *ATTR_TOP = "top";
	const char *ATTR_BOTTOM = "bottom";
	const char *ATTR_LEFT = "left";
	const char *ATTR_RIGHT = "right";
	
	// Tag XML for annotations file	
	const char *TAG_ANNOTATIONS = "ANNOTATIONS";
	const char *TAG_ANNOTATION = "ANNOTATION";
	const char *TAG_POPUP = "POPUP";
	const char *TAG_AUTHOR = "AUTHOR";
	const char *TAG_CONTENT = "CONTENT";
	const char *TAG_RICH_CONTENT = "CONTENT";
	const char *TAG_QUADPOINTS = "QUADPOINTS";
	const char *TAG_QUADRILATERAL = "QUADRILATERAL";
	const char *TAG_POINT = "POINT";

	// Attributs for annotations file
	const char *ATTR_SUBTYPE = "subtype";
	const char *ATTR_PAGENUM = "pagenum";
	const char *ATTR_OPEN = "open";
	
	// annotations in XML file
	const char *ATTR_GOTOLINK = "goto";
	const char *ATTR_URILINK = "href";
	const char *ATTR_HIGHLIGHT = "highlight";
	const char *ATTR_UNDERLINE = "underline";


	// Attributs for number format
	const char *ATTR_NUMFORMAT = "%g";
	const char *ATTR_SID = "sid";
	
	//PAGE BOXES
	const char *TAG_MEDIABOX = "MEDIABOX";
	const char *TAG_BLEEDBOX = "BLEEDBOX";
	const char *TAG_CROPBOX = "CROPBOX";
	const char *TAG_ARTBOX = "ARTBOX";
	const char *TAG_TRIMBOX = "TRIMBOX";

	
}

