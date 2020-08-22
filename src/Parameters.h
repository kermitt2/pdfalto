#ifndef PARAMETERS_H_
#define PARAMETERS_H_

#include "gtypes.h"

#if MULTITHREADED
#include "GMutex.h"
#endif

#include <libxml/xmlmemory.h>
#include <libxml/parser.h>

#include "ConstantsXML.h"
using namespace ConstantsXML;

class Parameters;

// The parameters object
extern Parameters *parameters;

/** 
 * Parameters class : parameters listing used into pdfalto as options <br></br>
 */
 
class Parameters {
public:

	/** Construct a new <code>Parameters</code> */ 
	Parameters();
	
	/** Destructor */
	~Parameters();
  
	// getters

	/** Return a boolean which inform if the text is displayed 
	 * @return <code>true</code> if the toText option is selected, <code>false</code> otherwise
	 */
	GBool getDisplayText() { return displayText;};
	
	/** Return a boolean which inform if blocks informations are diplayed
	 * @return <code>true</code> if the blocks option is selected, <code>false</code> otherwise
	 */
	//GBool getDisplayBlocks() { return displayBlocks;};
	
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

	/** PL: Return the boolean that controls whether to include TYPE attributes to String elements to
	 * indicate right-to-left reading order (produces non-valid ALTO)
	 * @return <code>true</code> if the charReadingOrderAttr option is selected, <code>false</code> otherwise
	 */
	GBool getCharReadingOrderAttr() {return charReadingOrderAttr;}

	/** Return a boolean which inform if OCR should be applied to recognize non unicode glyphs
	 * @return <code>true</code> if the ocr option is selected, <code>false</code> otherwise
	 */
	GBool getOcr() {return ocr;}

	/** Return the limit of files to be extracted.
	 * @return <code>true</code> if the readingOrder option is selected, <code>false</code> otherwise
	 */
	int getFilesCountLimit() {return filesCountLimit;}

	/** Return a boolean which inform if line numbers tokens are diplayed
	 * @return <code>true</code> if the noLineNumbers option is selected, <code>false</code> otherwise
	 */
	GBool getNoLineNumbers() { return noLineNumbers;};

	// setters

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
	//void setDisplayBlocks(GBool noblock);
	
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

	/** PL: Modifiy the boolean that controls whether to include TYPE attributes to String elements to indicate
	 * right-to-left reading order (produces non-valid ALTO)
	 * @param charReadingOrderAttr <code>true</code> if the charReadingOrderAttr option is selected, <code>false</code> otherwise
	 */
	void setCharReadingOrderAttr(GBool charReadingOrderAttrs);

	/** Modifiy the boolean which inform ocr should be applied or not
	 * @param readingOrder <code>true</code> if the readingOrder option is selected, <code>false</code> otherwise
	 */
	void setOcr(GBool ocrA);

	void setFilesCountLimit(int count);

	/** Modify the boolean which inform if line numbers must be diplayed
	 * @param noLineNumberAttrs <code>true</code> if the noLineNumbers option is selected, <code>false</code> otherwise
	 */	
	void setNoLineNumbers(GBool noLineNumberAttrs);
	
	void saveToXML(const char *fileName,int firstPage,int lastPage);
	
private:

	/** The value of the noImage option */
	GBool displayImage;
	/** The value of the noText option */
	GBool displayText;
	/** The value of the blocks option */
	//GBool displayBlocks;
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
	/** PL: The value of the charReadingOrderAttr option */
	GBool charReadingOrderAttr;
	/** The value of ocr option */
	GBool ocr;
	/** the count limit of files */
	int filesCountLimit;
  	/** PL: the value of the noLineNumbers option*/
  	GBool noLineNumbers;
};

#endif /*PARAMETERS_H_*/

