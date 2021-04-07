#include "ConstantsXML.h"

namespace ConstantsXML {
	// All tags ALTO XML dialect
	const char *TAG_ALTO = "alto";

	const char *ALTO_URI = "http://www.loc.gov/standards/alto/ns-v3#";
	const char *ALTO_LOCATION = "http://www.loc.gov/standards/alto/v3/alto.xsd";

	const char *TAG_DESCRIPTION = "Description";
    const char *TAG_MEASUREMENTUNIT = "MeasurementUnit";
    const char *TAG_SOURCE_IMAGE_INFO = "sourceImageInformation";
    const char *TAG_PDFFILENAME = "fileName";

    const char *TAG_OCRPROCESSING = "OCRProcessing";
    const char *ATTR_VALUEID_OCRPROCESSING = "IdOcr";

    const char *TAG_OCRPROCESSINGSTEP = "ocrProcessingStep";
    const char *TAG_PROCESSINGDATE = "processingDateTime";
    const char *TAG_PROCESSINGSOFTWARE = "processingSoftware";
    const char *TAG_SOFTWARE_CREATOR = "softwareCreator";
    const char *TAG_SOFTWARE_NAME = "softwareName";
    const char *TAG_SOFTWARE_VERSION = "softwareVersion";

	const char *TAG_STYLES = "Styles";
	const char *TAG_LAYOUT = "Layout";

	const char *TAG_PAGE = "Page";
	const char *TAG_IMAGE = "Illustration";
	const char *TAG_SVG = "svg";
	const char *TAG_BLOCK = "TextBlock";
	const char *TAG_TEXT = "TextLine";

    const char *TAG_PRINTSPACE = "PrintSpace";

	const char *TAG_TOKEN = "String";
	const char *ATTR_STYLEREFS = "STYLEREFS";

    const char *ATTR_TOKEN_CONTENT = "CONTENT";

    const char *TAG_SPACING = "SP";

    const char *TAG_TEXTSTYLE = "TextStyle";
    const char *ATTR_FONTFAMILY = "FONTFAMILY";
    const char *ATTR_FONTSIZE = "FONTSIZE";
    const char *ATTR_FONTTYPE = "FONTTYPE";
    const char *ATTR_FONTWIDTH = "FONTWIDTH";
    const char *ATTR_FONTCOLOR = "FONTCOLOR";
    const char *ATTR_FONTSTYLE = "FONTSTYLE";

	const char *ATTR_D = "d";
	
	const char *VERSION = "1.0";
	const char *ENCODING_UTF8 = "UTF-8";
	
	// All attributs
	const char *ATTR_WIDTH = "WIDTH";
	const char *ATTR_HEIGHT = "HEIGHT";
	const char *ATTR_PHYSICAL_IMG_NR = "PHYSICAL_IMG_NR";
	const char *ATTR_HREF = "FILEID";
	const char *ATTR_X = "HPOS";
	const char *ATTR_Y = "VPOS";
	
	const char *ATTR_ROTATION = "ROTATION";
	const char *ATTR_ANGLE = "angle";

	const char *ATTR_ID = "ID";
	// This attribute gives the reading order : right->left (value 0) or left->right (value 1)
	const char *ATTR_TYPE = "TYPE";
	
	const char *sTRUE = "true";
	const char *sFALSE = "false";

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
	const char *ATTR_EVENODD = "evenodd";
	const char *ATTR_CLIPZONE = "clipZone";
	const char *ATTR_IDCLIPZONE = "idClipZone";

	const char *ATTR_ID_ITEM_PARENT = "idItemParent";
	const char *ATTR_NB_PAGES = "nbPages";
	const char *ATTR_LOCATION = "xsi:schemaLocation";

	// SVG XML TAGS
	//const char *ATTR_NUMFORMAT = "%g";
	const char *ATTR_NUMFORMAT = "%1.4f";
	const char *ATTR_SID = "sid";
	const char *ATTR_SVGID = "id";
	const char *ATTR_SVG_X = "x";
	const char *ATTR_SVG_Y = "y";
	const char *ATTR_SVG_WIDTH = "width";
	const char *ATTR_SVG_HEIGHT = "height";

	const char *TAG_GROUP = "g";
	const char *TAG_CLIPPATH = "clipPath";
	const char *TAG_PATH = "path";
	const char *TAG_M = "M";
	const char *TAG_L = "L";
	const char *TAG_C = "C";


    // METADATA XML TAGS
	const char *TAG_METADATA = "METADATA";
	const char *TAG_PDFFILE_METADATA = "PDF_METADATA";
	const char *TAG_PROCESS = "PROCESS";
	const char *TAG_CREATIONDATE = "CREATIONDATE";
	const char *TAG_COMMENT = "COMMENT";


	// OUTLINE XML TAGS
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



	// ANNOTATIONS XML TAGS
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
	const char *TAG_ACTION = "ACTION";
	const char *TAG_DEST = "DEST";

	// Attributs for annotations file
	const char *ATTR_SUBTYPE = "subtype";
	const char *ATTR_PAGENUM = "pagenum";
	const char *ATTR_OPEN = "open";

	// annotations in XML file
	const char *ATTR_GOTOLINK = "goto";
	const char *ATTR_GOTORLINK = "gotor";
	const char *ATTR_ANNOTS_X = "x";
	const char *ATTR_ANNOTS_Y = "y";
	const char *ATTR_ANNOTS_PAGE = "page";
	const char *ATTR_ANNOTS_TYPE = "type";
	const char *ATTR_URILINK = "uri";
	const char *ATTR_HIGHLIGHT = "highlight";
	const char *ATTR_UNDERLINE = "underline";

	//PAGE BOXES
//	const char *TAG_MEDIABOX = "MEDIABOX";
//	const char *TAG_BLEEDBOX = "BLEEDBOX";
//	const char *TAG_CROPBOX = "CROPBOX";
//	const char *TAG_ARTBOX = "ARTBOX";
//	const char *TAG_TRIMBOX = "TRIMBOX";

	const char *TAG_PAR_CONF = "CONFIGURATION";
	const char *TAG_PAR_TOOL = "TOOL";
	const char *TAG_PAR_NAME = "NAME";
	const char *TAG_PAR_VER = "VERSION";
	const char *TAG_PAR_DESC = "DESCRIPTION";
	const char *TAG_PAR_PARAM = "PARAM";
	const char *TAG_PAR_ATTRNAME = "NAME";


}

