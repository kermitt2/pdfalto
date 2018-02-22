//========================================================================
//
// ConstantsUtils.cc
//
// Contain all constants useful for the pdftoxml tool. It's the initialization.
//
// author: Sophie Andrieu
// 05-2006
// Xerox Research Centre Europe
//
//========================================================================
/* $Author: dejean $
$Date: 2012/04/25 15:06:10 $
$Header: /cvsroot-fuse/pdf2xml/pdf2xml/src/ConstantsUtils.cc,v 1.3 2012/04/25 15:06:10 dejean Exp $
$Id: ConstantsUtils.cc,v 1.3 2012/04/25 15:06:10 dejean Exp $
$Name:  $
$Locker:  $
$Log: ConstantsUtils.cc,v $
Revision 1.3  2012/04/25 15:06:10  dejean
version 2.0 with xpdf3.03

Revision 1.2  2007/12/14 11:01:16  dejean
new command line
new attributes: idx, base

Revision 1.10  2007/12/14 10:07:17  hdejean
v 1.2 : idx  + base as default attribute

Revision 1.9  2007/12/14 09:20:58  hdejean
all

$RCSfile: ConstantsUtils.cc,v $
$Revision: 1.3 $
$Source: /cvsroot-fuse/pdf2xml/pdf2xml/src/ConstantsUtils.cc,v $
$State: Exp $

*/

static const char cvsid[] = "$Revision: 1.3 $";
#include "ConstantsUtils.h"

namespace ConstantsUtils
{
	const char *EXTENSION_XML = ".xml";
	const char *EXTENSION_XML_MAJ = ".XML";
	const char *EXTENSION_VEC = ".vec";
	const char *EXTENSION_PBM = ".pbm";
	const char *EXTENSION_PPM = ".ppm";
	const char *EXTENSION_PDF = ".pdf";
	const char *EXTENSION_PDF_MAJ = ".PDF";
	const char *NAME_OUTLINE = "outline";
	const char *NAME_ANNOT = "annot";
	const char *NAME_DATA_DIR = "_data";

	const char *PDFALTO_VERSION = "2.1";
	const char *PDFALTO_NAME = "pdfalto";

	const char *PDFALTO_CREATOR = "INRIA_ALMANACH";

	const char *MEASUREMENT_UNIT = "pixel";

}
