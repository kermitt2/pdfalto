//========================================================================
//
// Parameters.h
//
// Specifics parameters to pdftoxml
//
// author: Sophie Andrieu
// Xerox Research Centre Europe
//
//========================================================================

#ifndef PARAMETERS_H_
#define PARAMETERS_H_

#include "gtypes.h"

#if MULTITHREADED
#include "GMutex.h"
#endif

#include <libxml/xmlmemory.h>
#include <libxml/parser.h>

#include "ConstantsXMLAlto.h"
using namespace ConstantsXMLALTO;

class Parameters;

// The parameters object
extern Parameters *parameters;

/** 
 * Parameters class : parameters listing used into pdftoxml as options <br></br>
 * Xerox Research Centre Europe <br></br>
 * @date 05-2006
 * @author Sophie Andrieu
 * @version xpdf 3.01
 */
 
class Parameters {
public:

	/** Construct a new <code>Parameters</code> */ 
	Parameters();
	
	/** Destructor */
	~Parameters();
  
	/** Return a boolean which inform if the text is displayed 
	 * @return <code>true</code> if the toText option is selected, <code>false</code> otherwise
	 */
	GBool getDisplayText() { return displayText;};
	
	/** Return a boolean which inform if blocks informations are diplayed
	 * @return <code>true</code> if the blocks option is selected, <code>false</code> otherwise
	 */
	GBool getDisplayBlocks() { return displayBlocks;};
	
	/** Return a boolean which inform if the images are displayed
	 * @return <code>true</code> if the noImage option is not selected, <code>false</code> otherwise
	 */
	GBool getDisplayImage() {return displayImage;};
	
	/** Return a boolean which inform if the bookmark is displayed
	 * @return <code>true</code> if the outline option is selected, <code>false</code> otherwise
	 */
	GBool getDisplayOutline() {return displayOutline;}
	
	/** Return a boolean which inform if all pages are cutted or not
	 * @return <code>true</code> if the cutPages option is selected, <code>false</code> otherwise
	 */
	GBool getCutAllPages() {return cutAllPages;}
	
	/** Return a boolean which inform if the fonts names are normalized or not
	 * @return <code>true</code> if the fullFontName option is selected, <code>false</code> otherwise
	 */
	GBool getFullFontName() {return fullFontName;}

	/** Return a boolean which inform if the images inline are included in the stream or not
	 * @return <code>true</code> if the noImageInline option is selected, <code>false</code> otherwise
	 */
	GBool getImageInline() {return imageInline;}
	
	/** PL: Return a boolean which inform if the blocks should follow the reading order rather than the 
	 *  PDF content stream one
	 * @return <code>true</code> if the readingOrder option is selected, <code>false</code> otherwise
	 */
	GBool getReadingOrder() {return readingOrder;}

	/** Modify the boolean which inform if the images are displayed
	 * @param noImage <code>true</code> if the noImage option is not selected, <code>false</code> otherwise
	 */
	void setDisplayImage(GBool noImage);
	
	/** Modify the boolean which inform if the text is displayed 
	 * @param notext <code>true</code> if the toText option is selected, <code>false</code> otherwise
	 */	
	void setDisplayText(GBool notext);
	
	/** Modify the boolean which inform if blocks informations are diplayed
	 * @param noblock <code>true</code> if the blocks option is selected, <code>false</code> otherwise
	 */	
	void setDisplayBlocks(GBool noblock);
	
	/** Modify the boolean which inform if the bookmark is displayed
	 * @param outline <code>true</code> if the outline option is selected, <code>false</code> otherwise
	 */	
	void setDisplayOutline(GBool outline);
	
	/** Modifiy the boolean which inform if all pages are cutted or not
	 * @param cutPages <code>true</code> if the cutPages option is selected, <code>false</code> otherwise
	 */	
	void setCutAllPages(GBool cutPages);
	
	/** Modifiy the boolean which inform if the fonts names are normalized or not
	 * @param fullFontName <code>true</code> if the fullFontName option is selected, <code>false</code> otherwise
	 */	
	void setFullFontName(GBool fullFontName);
	
	/** Modifiy the boolean which inform if the images inline are included in the stream or not
	 * @param imageInline <code>true</code> if the noImageInline option is selected, <code>false</code> otherwise
	 */	
	void setImageInline(GBool imageInline);

	/** PL: Modifiy the boolean which inform if the blocks should follow the reading order
	 * @param readingOrder <code>true</code> if the readingOrder option is selected, <code>false</code> otherwise
	 */	
	void setReadingOrder(GBool readingOrders);
	
	void saveToXML(const char *fileName,int firstPage,int lastPage);
	
private:

	/** The value of the noImage option */
	GBool displayImage;
	/** The value of the noText option */
	GBool displayText;
	/** The value of the blocks option */
	GBool displayBlocks;
	/** The value of the outline option */
	GBool displayOutline;
	/** The value of the cutPages option */
	GBool cutAllPages;
	/** The value of the fullFontName option */
	GBool fullFontName;
	/** The value of the noImageInline option */
	GBool imageInline;
	/** PL: The value of the readingOrder option */
	GBool readingOrder;
  
};

#endif /*PARAMETERS_H_*/

