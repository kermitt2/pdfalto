#include "PDFDocXrce.h"
#include <stdio.h>
#include "XRef.h"
#include "Catalog.h"
#include "GfxState.h"
#include "OutputDev.h"
#include "Page.h"
#include "AnnotsXrce.h"

#include <libxml/xmlmemory.h>
#include <libxml/parser.h>

PDFDocXrce::PDFDocXrce(GString *fileNameA, GString *ownerPassword, GString *userPassword): PDFDoc(fileNameA, ownerPassword, userPassword) {
}
	 	
void PDFDocXrce::displayPages(OutputDev *out, xmlNodePtr docrootA, int firstPage, int lastPage,
			  double hDPI, double vDPI, int rotate,
			  GBool useMediaBox, GBool crop, GBool doLinks,
			  GBool (*abortCheckCbk)(void *data),
			  void *abortCheckCbkData){
			  
	PDFDoc::displayPages(out, firstPage, lastPage, hDPI, vDPI, rotate, useMediaBox, crop, doLinks); 

	int pageNum;
	Page *currentPage;
	GfxState *state;
  	AnnotsXrce *an;
  	Object objAnnot;
  	double ctm[6];
  	
  	for (pageNum = firstPage; pageNum <= lastPage; ++pageNum) { 		
  		if (docrootA != NULL){			
  			currentPage = getCatalog()->getPage(pageNum);	  		
  			int i;

  			if (rotate >= 360) {
    			rotate -= 360;
  			} else if (rotate < 0) {
    			rotate += 360;
  			}
  			
  			// We recover the state matrix page.
  			// Use the CropBox (not the MediaBox) so annotation coordinates are referenced to the
  			// same page frame as the text (rendered with useMediaBox=false). Otherwise, on pages
  			// whose CropBox is inset from the MediaBox (bleed/trim margins), annotation boxes are
  			// shifted by the CropBox origin and can land on the wrong line.
  			state = new GfxState(hDPI, vDPI, currentPage->getCropBox(), rotate, out->upsideDown());
  			for (i = 0; i < 6; ++i) {
    			ctm[i] = state->getCTM()[i];
  			}
  			delete state;

  			currentPage->getAnnots(&objAnnot);
  	  
			  // Annotation's objects list
    		if (objAnnot.isArray()){
  	  			an = new AnnotsXrce(objAnnot, docrootA, getCatalog(), ctm, pageNum);  
  	  			delete an;		
  			}
  			objAnnot.free();
		  }			
  	}
}

PDFDocXrce::~PDFDocXrce(){
}


