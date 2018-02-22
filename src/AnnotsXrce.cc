//====================================================================
//
// AnnotsXrce.cc
//
// author: Sophie Andrieu
//
// 09-2006
//
// Xerox Research Centre Europe
//
//====================================================================

#include "AnnotsXrce.h"
#include <stdlib.h>
#include "Object.h"
#include "Link.h"
#include "ConstantsXMLALTO.h"
#include "Catalog.h"
#include "UnicodeMap.h"
#include "PDFDocEncoding.h"
#include "GlobalParams.h"
#include "UnicodeTypeTable.h"


using namespace ConstantsXMLALTO;

#include <libxml/xmlmemory.h>
#include <libxml/parser.h>

AnnotsXrce::AnnotsXrce(Object &objA, xmlNodePtr docrootA, Catalog *catalog,double *ctmA, int pageNumA){
	
	idAnnot = 1;
	double x, y;
	
	xmlNodePtr nodeAnnot = NULL;
  	xmlNodePtr nodePopup = NULL;
  	xmlNodePtr nodeContent = NULL;
  	
  	Object objSubtype;
  	Object objContents;
  	Object objPopup;
  	Object objT;
  	Object objQuadPoints;
  	Object objAP;
  	Object objRect;
  	Object rectPoint;
  	Object objAct;
  	GBool current = gFalse;
  	GBool isLink = gFalse;
	Object kid;

	GString *news;
	UnicodeMap *uMap;

	if (!(uMap = globalParams->getTextEncoding())) {
		return;
	}

//	printf("%d\n", objA.arrayGetLength());
  	for (int i = 0 ; i < objA.arrayGetLength() ; ++i){
  		objA.arrayGet(i, &kid);
  		if (kid.isDict()) {
  			Dict *dict;
  			dict = kid.getDict();
			char* tmpor=(char*)malloc(100*sizeof(char));
			// Get the annotation's type
  			if (dict->lookup("Subtype", &objSubtype)->isName()){
  				// It can be 'Highlight' or 'Underline' or 'Link' (Subtype 'Squiggly' or 'StrikeOut' are not supported)
  				if (!strcmp(objSubtype.getName(), "Text") || !strcmp(objSubtype.getName(), "Highlight") || !strcmp(objSubtype.getName(), "Underline") || !strcmp(objSubtype.getName(), "FreeText") || !strcmp(objSubtype.getName(), "Link")){
  					nodeAnnot = xmlNewNode(NULL,(const xmlChar*)TAG_ANNOTATION);
  					nodeAnnot->type = XML_ELEMENT_NODE;
  					xmlNewProp(nodeAnnot,(const xmlChar*)ATTR_SUBTYPE,(const xmlChar*)objSubtype.getName());
  					xmlAddChild(docrootA,nodeAnnot);
  					current = gTrue;
  					sprintf(tmpor,"%d",pageNumA);
  					xmlNewProp(nodeAnnot,(const xmlChar*)ATTR_PAGENUM,(const xmlChar*)tmpor);
  					free(tmpor);
  					isLink = gFalse;
  				}		
  				if (!strcmp(objSubtype.getName(), "Link")){
  					isLink = gTrue;
  				}
				//printf("%s\n",objSubtype.getName());
  			}
  			objSubtype.free();


  			// Get informations about Link annotation
  			if (isLink){
				Link *link = new Link(dict,catalog->getBaseURI());
				//printf("%d \n",link->isOk());
				LinkAction * ac= link->getAction();
				//printf("ac %d \n",ac->isOk());

  				// Get the Action information
  				if (dict->lookup("A", &objAct)->isDict()){
  					xmlNodePtr nodeActionAction;
  					xmlNodePtr nodeActionDEST;
  					if (nodeAnnot){
  	  					nodeActionAction = xmlNewNode(NULL,(const xmlChar*)"ACTION");
  						nodeActionAction->type = XML_ELEMENT_NODE;

  						xmlAddChild(nodeAnnot, nodeActionAction);
					}

  					Dict *dictAction;
					Object objURI;
					Object objGoTo;
					dictAction = objAct.getDict();
					Object objS;
					Object objF;
					// Get the type of link
					if (dictAction->lookup("S", &objS)->isName()){
						if (!strcmp(objS.getName(), "URI")){
							//printf("uri\n");
							xmlNewProp(nodeActionAction,(const xmlChar*)"type",(const xmlChar*)"URI");
							if (dictAction->lookup("URI", &objURI)->isString()){
								if (nodeAnnot){
  	  								nodeActionDEST = xmlNewNode(NULL,(const xmlChar*)"DEST");
  									nodeActionDEST->type = XML_ELEMENT_NODE;
									//to unicode first
								    	news  = toUnicode(objURI.getString(),uMap);
  									xmlNodeSetContent(nodeActionDEST,(const xmlChar*)xmlEncodeEntitiesReentrant(nodeActionDEST->doc,(const xmlChar*)news->getCString()));
  									xmlAddChild(nodeActionAction, nodeActionDEST);
								}
								objURI.free();
							}
						}
						if (!strcmp(objS.getName(), "GoToR")){
							//printf("gotor\n");
							if(link->isOk()){
								xmlNewProp(nodeActionAction,(const xmlChar*)"type",(const xmlChar*)objS.getName());
								// Get the destination to jump to
								LinkAction* action = link->getAction();
								LinkGoToR* goto_link = (LinkGoToR*)action;
  								news  = toUnicode(goto_link->getFileName(),uMap);
  	  							nodeActionDEST = xmlNewNode(NULL,(const xmlChar*)"DEST");
  								nodeActionDEST->type = XML_ELEMENT_NODE;
  								xmlNodeSetContent(nodeActionDEST,(const xmlChar*)xmlEncodeEntitiesReentrant(nodeActionAction->doc,(const xmlChar*)news->getCString()));
  								xmlAddChild(nodeActionAction, nodeActionDEST);
							}else{
								xmlNewProp(nodeActionAction,(const xmlChar*)"type",(const xmlChar*)objS.getName());
  	  							nodeActionDEST = xmlNewNode(NULL,(const xmlChar*)"DEST");
  								nodeActionDEST->type = XML_ELEMENT_NODE;
  								xmlAddChild(nodeActionAction, nodeActionDEST);
							}
						}
						if (!strcmp(objS.getName(), "GoTo")){
							//printf("goto\n");
							xmlNewProp(nodeActionAction,(const xmlChar*)"type",(const xmlChar*)objS.getName());
							// Get the destination to jump to
							nodeActionDEST = NULL;
							LinkAction* action = link->getAction();
							LinkGoTo* goto_link = (LinkGoTo*)action;
							bool newlink = false;
							LinkDest* link_dest = goto_link->getDest();
							GString*  name_dest = goto_link->getNamedDest();
							if (name_dest != NULL && catalog != NULL)
							{
								link_dest = catalog->findDest(name_dest);
								newlink   = true;
							}
							if (link_dest != NULL && link_dest->isOk())
								{
									// find the destination page number (counted from 1)
									int page;
									if (link_dest->isPageRef())
									{
										Ref pref = link_dest->getPageRef();
										page = catalog->findPage(pref.num, pref.gen);
									}
									else
										page = link_dest->getPageNum();

									// other data depend in the link type
									//printf("page %d %d\n",page,link_dest->getKind());
									switch (link_dest->getKind())
									{
									case destXYZ:
										{
  	  									nodeActionDEST = xmlNewNode(NULL,(const xmlChar*)"DEST");
  										nodeActionDEST->type = XML_ELEMENT_NODE;
										//printf("%s\n",goto_link->getNamedDest()->getCString());
  										//news  = toUnicode(goto_link->getNamedDest(),uMap);
  										//xmlNodeSetContent(nodeActionDEST,(const xmlChar*)xmlEncodeEntitiesReentrant(nodeActionDEST->doc,(const xmlChar*)news->getCString()));
  										xmlAddChild(nodeActionAction, nodeActionDEST);
										// find the location on the destination page
										//if (link_dest->getChangeLeft() && link_dest->getChangeTop()){
										// TODO FH 25/01/2006 apply transform matrix of destination page, not current page
										double x,y;
										transform(link_dest->getLeft(), link_dest->getTop(), &x, &y,ctmA);
										char *tmp=(char*)malloc(10*sizeof(char));	
										sprintf(tmp,"%d",page);
										//printf("link %d %g %g\n",page,x,y);
										xmlNewProp(nodeActionDEST,(const xmlChar*)"page",(const xmlChar*)tmp);
										sprintf(tmp,"%g",x);
										xmlNewProp(nodeActionDEST,(const xmlChar*)"x",(const xmlChar*)tmp);
										sprintf(tmp,"%g",y);
										xmlNewProp(nodeActionDEST,(const xmlChar*)"y",(const xmlChar*)tmp);
											//}
										}
										break;

									// link to the page, without a specific location. PDF Data Destruction has hit again!
									case destFit:  case destFitH: case destFitV: case destFitR:
									case destFitB: case destFitBH: case destFitBV:
										char *tmp=(char*)malloc(10*sizeof(char));	
										sprintf(tmp,"%d",page);
										xmlNewProp(nodeActionDEST,(const xmlChar*)"page",(const xmlChar*)tmp);
										xmlNewProp(nodeActionDEST,(const xmlChar*)"x",(const xmlChar*)"0");
										xmlNewProp(nodeActionDEST,(const xmlChar*)"y",(const xmlChar*)"0");
										//printf("p-%d\n",page);
									}

									// must delete the link object if it comes from the catalog
									if (newlink)
										delete link_dest;
								}
						}
					}
					objS.free();
  				}
  				objAct.free();

				// Get the rectangle location link annotation
  				if (dict->lookup("Rect", &objRect)->isArray()){
  					double xMin = 0.;
  					double xMax = 0.;
  					double yMin = 0.;
  					double yMax = 0.;

  					for (int j = 0 ; j < objRect.arrayGetLength() ; ++j){
  						objRect.arrayGet(j, &rectPoint);
  						if (j==0){
  							if (rectPoint.isInt()) xMin = static_cast<double>(rectPoint.getInt());
  							if (rectPoint.isReal()) xMin = rectPoint.getReal();
  						}
  						if (j==1){
  							if (rectPoint.isInt()) yMin = static_cast<double>(rectPoint.getInt());
  							if (rectPoint.isReal()) yMin = rectPoint.getReal();
  						}
  						if (j==2){
  							if (rectPoint.isInt()) xMax = static_cast<double>(rectPoint.getInt());
  							if (rectPoint.isReal()) xMax = rectPoint.getReal();
  						}
  						if (j==3){
  							if (rectPoint.isInt()) yMax = static_cast<double>(rectPoint.getInt());
  							if (rectPoint.isReal()) yMax = rectPoint.getReal();

  							double xMinT,xMaxT,yMinT,yMaxT;
  							xMinT = xMaxT = yMinT = yMaxT = 0.;
  							transform(xMin, yMin, &xMinT, &yMinT, ctmA);
  							transform(xMax, yMax, &xMaxT, &yMaxT, ctmA);

  							xmlNodePtr nodeQuadR;
  							xmlNodePtr nodeQuadrilR;
  							xmlNodePtr nodePointsR1;

  							if (nodeAnnot){
  	  							nodeQuadR = xmlNewNode(NULL,(const xmlChar*)TAG_QUADPOINTS);
  								nodeQuadR->type = XML_ELEMENT_NODE;
  								xmlAddChild(nodeAnnot, nodeQuadR);
  								nodeQuadrilR = xmlNewNode(NULL,(const xmlChar*)TAG_QUADRILATERAL);
  								nodeQuadrilR->type = XML_ELEMENT_NODE;
  								xmlAddChild(nodeQuadR, nodeQuadrilR);

  								char* t = (char*)malloc(10*sizeof(char));

  								nodePointsR1 = xmlNewNode(NULL,(const xmlChar*)TAG_POINT);
  								nodePointsR1->type = XML_ELEMENT_NODE;
  								xmlAddChild(nodeQuadrilR, nodePointsR1);
  								sprintf(t,"%lg",xMinT);
  								xmlNewProp(nodePointsR1,(const xmlChar*)ATTR_X,(const xmlChar*)t);
  								sprintf(t,"%lg",yMinT);
  								xmlNewProp(nodePointsR1,(const xmlChar*)ATTR_Y,(const xmlChar*)t);

  								nodePointsR1 = xmlNewNode(NULL,(const xmlChar*)TAG_POINT);
  								nodePointsR1->type = XML_ELEMENT_NODE;
  								xmlAddChild(nodeQuadrilR, nodePointsR1);
  								sprintf(t,"%lg",xMinT);
  								xmlNewProp(nodePointsR1,(const xmlChar*)ATTR_X,(const xmlChar*)t);
  								sprintf(t,"%lg",yMaxT);
  								xmlNewProp(nodePointsR1,(const xmlChar*)ATTR_Y,(const xmlChar*)t);

  								nodePointsR1 = xmlNewNode(NULL,(const xmlChar*)TAG_POINT);
  								nodePointsR1->type = XML_ELEMENT_NODE;
  								xmlAddChild(nodeQuadrilR, nodePointsR1);
  								sprintf(t,"%lg",xMaxT);
  								xmlNewProp(nodePointsR1,(const xmlChar*)ATTR_X,(const xmlChar*)t);
  								sprintf(t,"%lg",yMinT);
  								xmlNewProp(nodePointsR1,(const xmlChar*)ATTR_Y,(const xmlChar*)t);

  								nodePointsR1 = xmlNewNode(NULL,(const xmlChar*)TAG_POINT);
  								nodePointsR1->type = XML_ELEMENT_NODE;
  								xmlAddChild(nodeQuadrilR, nodePointsR1);
  								sprintf(t,"%lg",xMaxT);
  								xmlNewProp(nodePointsR1,(const xmlChar*)ATTR_X,(const xmlChar*)t);
  								sprintf(t,"%lg",yMaxT);
  								xmlNewProp(nodePointsR1,(const xmlChar*)ATTR_Y,(const xmlChar*)t);

  								free(t);
  							}
  						}
  						rectPoint.free();
  					}
  				}
  				objRect.free();
  			}

			// Add the id attribut into the annotation tag : format is 'p<pageNumber>_a<annotationNumber>
  			if (nodeAnnot && current){
  				char* tmp=(char*)malloc(10*sizeof(char));
  				GString *idValue;
  				idValue = new GString("p");
  				sprintf(tmp,"%d",pageNumA);
  				idValue->append(tmp);
  				idValue->append("_a");
  				sprintf(tmp,"%d",idAnnot);
  				idValue->append(tmp);
  				xmlNewProp(nodeAnnot,(const xmlChar*)"id",(const xmlChar*)idValue->getCString());
  				free(tmp);
  				delete idValue;
  				idAnnot++;
  				current = gFalse;
  			}
            // Get the annotation's author
            if (dict->lookup("T", &objT)->isString()){
                    if (nodeAnnot){
                            xmlNodePtr nodeAUTH;
                            nodeAUTH = xmlNewNode(NULL,(const xmlChar*)TAG_AUTHOR);
                            nodeAUTH->type = XML_ELEMENT_NODE;
                            news  = toUnicode(objT.getString(),uMap);
                            xmlNodeSetContent(nodeAUTH,(const xmlChar*)xmlEncodeEntitiesReentrant(nodeAUTH->doc,(const xmlChar*)news->getCString()));
                            xmlAddChild(nodeAnnot, nodeAUTH);
                    }
            }
            objT.free();

  			// Get the popup object if it exists
    		if (dict->lookup("Popup", &objPopup)->isDict()){
  				if (nodeAnnot){
  	  				nodePopup = xmlNewNode(NULL,(const xmlChar*)TAG_POPUP);
  					nodePopup->type = XML_ELEMENT_NODE;
  					xmlAddChild(nodeAnnot, nodePopup);
  				}

				Dict *dictPopup;
				Object open;
				dictPopup = objPopup.getDict();

				if (dictPopup->lookup("Open", &open)->isBool()){
					if (nodePopup){
						if (open.getBool()){
							xmlNewProp(nodePopup,(const xmlChar*)ATTR_OPEN,(const xmlChar*)"true");
						}else{
							xmlNewProp(nodePopup,(const xmlChar*)ATTR_OPEN,(const xmlChar*)"false");
						}
					}
				}
				open.free();
  			}
  			objPopup.free();

  			// Get the popup's contents
  			if (dict->lookup("RC", &objContents)->isString()){
  				if (nodeAnnot){
  						nodeContent = xmlNewNode(NULL,(const xmlChar*)TAG_RICH_CONTENT);
  						nodeContent->type = XML_ELEMENT_NODE;
						//to unicode first
  						news  = toUnicode(objContents.getString(),uMap);
  						xmlNodeSetContent(nodeContent,(const xmlChar*)xmlEncodeEntitiesReentrant(nodeContent->doc,(const xmlChar*)news->getCString()));
  						xmlAddChild(nodePopup, nodeContent);
  				}
  			}
  			if (dict->lookup("Contents", &objContents)->isString()){
  				if (nodeAnnot){
  					if (nodePopup){
  						nodeContent = xmlNewNode(NULL,(const xmlChar*)TAG_CONTENT);
  						nodeContent->type = XML_ELEMENT_NODE;

						//to unicode first
  						news  = toUnicode(objContents.getString(),uMap);
//  		  				printf("contents: %s\n",news->getCString());
  						xmlNodeSetContent(nodeContent,(const xmlChar*)xmlEncodeEntitiesReentrant(nodeContent->doc,(const xmlChar*)news->getCString()));
  						xmlAddChild(nodePopup, nodeContent);
  					}
  				}
  			}
  			objContents.free();

  			// Get the localization (points series) of the annotation into the page
   			if (dict->lookup("QuadPoints", &objQuadPoints)->isArray()){
  				xmlNodePtr nodeQuad;
  				xmlNodePtr nodeQuadril;
  				xmlNodePtr nodePoints;

  				if (nodeAnnot){
  	  				nodeQuad = xmlNewNode(NULL,(const xmlChar*)TAG_QUADPOINTS);
  					nodeQuad->type = XML_ELEMENT_NODE;
  					xmlAddChild(nodeAnnot, nodeQuad);
  				}
  				Object point;
  				char* temp=(char*)malloc(10*sizeof(char));
   				double xx = 0;
   				double yy = 0;
  				for (int j = 0 ; j < objQuadPoints.arrayGetLength() ; ++j){
  					objQuadPoints.arrayGet(j, &point);

  					if (j%8==0){
  						nodeQuadril = xmlNewNode(NULL,(const xmlChar*)TAG_QUADRILATERAL);
  						nodeQuadril->type = XML_ELEMENT_NODE;
  						xmlAddChild(nodeQuad, nodeQuadril);
  					}
					if (j%2==0){
  						nodePoints = xmlNewNode(NULL,(const xmlChar*)TAG_POINT);
  						nodePoints->type = XML_ELEMENT_NODE;
  						xmlAddChild(nodeQuadril, nodePoints);

  						if (point.isReal()) {
  							xx = point.getReal();
  						}
  					}else{
  						if (point.isReal()) {
  							yy = point.getReal();
  							if (xx!=0){
  								transform(xx, yy, &x, &y, ctmA);
  							}
  							sprintf(temp,"%lg",x);
  							xmlNewProp(nodePoints,(const xmlChar*)ATTR_X,(const xmlChar*)temp);
  							sprintf(temp,"%lg",y);
  							xmlNewProp(nodePoints,(const xmlChar*)ATTR_Y,(const xmlChar*)temp);
  							xx=0;
  						}
  					}
  					point.free();
  				}
  				free(temp);
  			}
  			objQuadPoints.free();

			if (dict->lookup("AP", &objAP)->isDict()){
  				Dict *dictStream;
  				dictStream = objAP.getDict();
  				Object objN;

   				// Annotation with normal appearance
   				if (dictStream->lookupNF("N",&objN)->isRef()){
  					objN.free();
  				}
  			}
  			objAP.free();
  		}
  		kid.free();
  	} // end FOR
}

AnnotsXrce::~AnnotsXrce(){
}


void AnnotsXrce::transform(double x1, double y1, double *x2, double *y2, double *c){
	*x2 = c[0] * x1 + c[2] * y1 + c[4];
    *y2 = c[1] * x1 + c[3] * y1 + c[5]; 
}

GString* AnnotsXrce::toUnicode(GString *s,UnicodeMap *uMap){

	GString *news;
	Unicode *uString;
	int uLen;
	int j;

	if ((s->getChar(0) & 0xff) == 0xfe &&
	(s->getChar(1) & 0xff) == 0xff) {
	  uLen = (s->getLength() - 2) / 2;
	  uString = (Unicode *)gmallocn(uLen, sizeof(Unicode));
	  for (j = 0; j < uLen; ++j) {
		  uString[j] = ((s->getChar(2 + 2*j) & 0xff) << 8) |
			   (s->getChar(3 + 2*j) & 0xff);
	  }
	} else {
		uLen = s->getLength();
		uString = (Unicode *)gmallocn(uLen, sizeof(Unicode));
	  for (j = 0; j < uLen; ++j) {
		  uString[j] = pdfDocEncoding[s->getChar(j) & 0xff];
	  }
	}

	news = new GString();
	dumpFragment(uString,uLen,uMap,news);

	return news;
}

int AnnotsXrce::dumpFragment(Unicode *text, int len, UnicodeMap *uMap,
		GString *s) {
	char lre[8], rle[8], popdf[8], buf[8];
	int lreLen, rleLen, popdfLen, n;
	int nCols, i, j, k;

	nCols = 0;

	// Unicode OK
	if (uMap->isUnicode()) {

		lreLen = uMap->mapUnicode(0x202a, lre, sizeof(lre));
		rleLen = uMap->mapUnicode(0x202b, rle, sizeof(rle));
		popdfLen = uMap->mapUnicode(0x202c, popdf, sizeof(popdf));

		// IF primary direction is Left to Right
		if (1) { //(primaryLR) {

			i = 0;
			while (i < len) {
				// output a left-to-right section
				for (j = i; j < len && !unicodeTypeR(text[j]); ++j)
					;
				for (k = i; k < j; ++k) {
					n = uMap->mapUnicode(text[k], buf, sizeof(buf));
					s->append(buf, n);
					++nCols;
				}
				i = j;
				// output a right-to-left section
				for (j = i; j < len && !unicodeTypeL(text[j]); ++j)
					;
				if (j > i) {
					s->append(rle, rleLen);
					for (k = j - 1; k >= i; --k) {
						n = uMap->mapUnicode(text[k], buf, sizeof(buf));
						s->append(buf, n);
						++nCols;
					}
					s->append(popdf, popdfLen);
					i = j;
				}
			}

		}
		// ELSE primary direction is Right to Left
		else {

			s->append(rle, rleLen);
			i = len - 1;
			while (i >= 0) {
				// output a right-to-left section
				for (j = i; j >= 0 && !unicodeTypeL(text[j]); --j)
					;
				for (k = i; k > j; --k) {
					n = uMap->mapUnicode(text[k], buf, sizeof(buf));
					s->append(buf, n);
					++nCols;
				}
				i = j;
				// output a left-to-right section
				for (j = i; j >= 0 && !unicodeTypeR(text[j]); --j)
					;
				if (j < i) {
					s->append(lre, lreLen);
					for (k = j + 1; k <= i; ++k) {
						n = uMap->mapUnicode(text[k], buf, sizeof(buf));
						s->append(buf, n);
						++nCols;
					}
					s->append(popdf, popdfLen);
					i = j;
				}
			}
			s->append(popdf, popdfLen);
		}
	}
	// Unicode NOT OK
	else {
		for (i = 0; i < len; ++i) {
			n = uMap->mapUnicode(text[i], buf, sizeof(buf));
			s->append(buf, n);
			nCols += n;
		}
	}

	return nCols;
}

