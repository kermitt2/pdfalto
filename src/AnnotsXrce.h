//====================================================================
//
// AnnotsXrce.h
//
// author: Sophie Andrieu
//
// 09-2006
//
// Xerox Research Centre Europe
//
//====================================================================

#ifndef ANNOTSXRCE_H_
#define ANNOTSXRCE_H_

#include <stdio.h>
#include <stdlib.h>
#include "GString.h"
#include "Object.h"
#include "Catalog.h"
#include "UnicodeMap.h"
#include "PDFDocEncoding.h"
#include "GlobalParams.h"
#include "UnicodeTypeTable.h"

#include <libxml/xmlmemory.h>
#include <libxml/parser.h>

/** 
 * AnnotsXrce class <br></br>
 * This class is used to recover text markup annotations (Highlight and Underline)<br></br>
 * Xerox Research Centre Europe <br></br>
 * @date 09-2006
 * @author Sophie Andrieu
 */
class AnnotsXrce {

public:

  /** Construct a new <code>AnnotsXrce</code> 
   * @param objA The current annotation's object  
   * @param docrootA The document XML root which will store annotations
   * @param ctmA The matrix values used to get the transformation
   * @param pageNumA The numero of page where this annotation is located
   */
	AnnotsXrce(Object &objA, xmlNodePtr docrootA, Catalog *catalog,double *ctmA, int pageNumA);

  /** Destructor 
   */
	~AnnotsXrce();

  /** Get the id of the current annotation
   * @return The id of the current annotation */	
	int getIdAnnot(){return idAnnot;}

  /** Modify the id of the current annotation
   * @param idAnnotA The new id of the annotation */	
	void setIdAnnot(int idAnnotA){idAnnot=idAnnotA;}

	AnnotsXrce **getList();

  /** Transform the values with the matrix page
   * @param x1 The new x value after transformation
   * @param y1 The new y value after transformation
   * @param x2 The original x value
   * @param y2 The original y value
   * @param ctmA The matrix values used to get the transformation
   */		
	void transform(double x1, double y1, double *x2, double *y2, double *ctmA);
	
	GString*  toUnicode(GString *s,UnicodeMap *uMap);

	int dumpFragment(Unicode *text, int len, UnicodeMap *uMap,GString *s);

private:

	int idAnnot;

};

#endif 

