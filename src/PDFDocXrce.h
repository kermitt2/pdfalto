//====================================================================
//
// PDFDocXrce.h
//
// author: Sophie Andrieu
//
// 09-2006
//
// Xerox Research Centre Europe
//
//====================================================================

#ifndef PDFDOCXRCE_H_
#define PDFDOCXRCE_H_

#include <stdio.h>
#include "XRef.h"
#include "Catalog.h"
#include "Page.h"
#include "PDFDoc.h"

#include <libxml/xmlmemory.h>
#include <libxml/parser.h>

class GString;
class BaseStream;
class OutputDev;
class Links;
class LinkAction;
class LinkDest;
class Outline;

/** 
 * PDFDocXrce class <br></br>
 * This class extend the PDFDoc class<br></br>
 * Xerox Research Centre Europe <br></br>
 * @date 09-2006
 * @author Sophie Andrieu
 */
class PDFDocXrce: public PDFDoc {
	
public:

  /** Construct a new <code>PDFDocXrce</code> 
   * @param fileNameA The file name  
   * @param ownerPassword The owner password
   * @param userPassword The user password
   */
	PDFDocXrce(GString *fileNameA, GString *ownerPassword = NULL, GString *userPassword = NULL);

  /** Destructor 
   */
	virtual ~PDFDocXrce();

  /** Display all pages
   * @param out The state description
   * @param docrootA The document XML root which will store annotations
   * @param firstPage The first page to display
   * @param lastPage The last page to display
   * @param hDPI The h DPI value
   * @param vDPI The vDPI value
   * @param rotate The value of the rotation
   * @param useMediaBox To know if media box is used
   * @param crop To know if crop box is used
   * @param doLinks To know if links will be extracted
   * @param abortCheckCbk
   * @param abortCheckCbkData
   */
  	void displayPages(OutputDev *out, xmlNodePtr docrootA, int firstPage, int lastPage,
		    double hDPI, double vDPI, int rotate,
		    GBool useMediaBox, GBool crop, GBool doLinks,
		    GBool (*abortCheckCbk)(void *data) = NULL,
		    void *abortCheckCbkData = NULL);

};

#endif
