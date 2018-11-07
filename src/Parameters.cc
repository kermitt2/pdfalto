#include "Parameters.h"
#include <stdio.h>

#if MULTITHREADED
#  define lockGlobalParams            gLockMutex(&mutex)
#  define unlockGlobalParams          gUnlockMutex(&mutex)
#else
#  define lockGlobalParams
#  define unlockGlobalParams
#endif

Parameters *parameters = NULL;

Parameters::Parameters() {}

Parameters::~Parameters() {}

void Parameters::setFilesCountLimit(int count) {
	lockGlobalParams;
	filesCountLimit = count;
	unlockGlobalParams;
}


void Parameters::setDisplayImage(GBool image) {
  lockGlobalParams;
  displayImage = image;
  unlockGlobalParams;
}

void Parameters::setDisplayText(GBool text) {
  lockGlobalParams;
  displayText = text;
  unlockGlobalParams;
}

void Parameters::setDisplayBlocks(GBool block) {
  lockGlobalParams;
  displayBlocks = block;
  unlockGlobalParams;
}

void Parameters::setDisplayOutline(GBool outl) {
  lockGlobalParams;
  displayOutline = outl;
  unlockGlobalParams;
}

void Parameters::setCutAllPages(GBool cutPages) {
  lockGlobalParams;
  cutAllPages = cutPages;
  unlockGlobalParams;
}

void Parameters::setFullFontName(GBool fullFontsNames) {
  lockGlobalParams;
  fullFontName = fullFontsNames;
  unlockGlobalParams;
}

void Parameters::setImageInline(GBool imagesInline) {
  lockGlobalParams;
  imageInline = imagesInline;
  unlockGlobalParams;
}

// PL
void Parameters::setReadingOrder(GBool readingOrders) {
  lockGlobalParams;
  readingOrder = readingOrders;
  unlockGlobalParams;
}

void Parameters::setCharCount(int charCountA) {
	lockGlobalParams;
	charCount = charCountA;
	unlockGlobalParams;
}

void Parameters::saveToXML(const char *fileName,int firstPage,int lastPage){
	char* tmp;
  	tmp=(char*)malloc(10*sizeof(char));
  	
  	xmlNodePtr confNode = NULL;
  	xmlNodePtr tool = NULL;
  	xmlNodePtr name = NULL;
  	xmlNodePtr version = NULL;
  	xmlNodePtr desc = NULL;
  	xmlNodePtr param = NULL;
  	xmlDocPtr confDoc =NULL;
  	
  	
	confDoc = xmlNewDoc((const xmlChar*)VERSION);
	confNode = xmlNewNode(NULL,(const xmlChar*)TAG_PAR_CONF);
	tool = xmlNewNode(NULL,(const xmlChar*)TAG_PAR_TOOL);
	name = xmlNewNode(NULL,(const xmlChar*)TAG_PAR_NAME);
	version =  xmlNewNode(NULL,(const xmlChar*)TAG_PAR_VER);
	desc = xmlNewNode(NULL,(const xmlChar*)TAG_PAR_DESC);
	
	xmlDocSetRootElement(confDoc,confNode);
	xmlAddChild(confNode,tool);
	xmlAddChild(tool,name);
	xmlAddChild(tool,version);
	xmlAddChild(tool,desc);
	
// * -f <int> : first page to convert<br/>
// * -l <int> : last page to convert<br/>
// * -verbose : display pdf attributes<br/>
// * -noText : do not extract textual objects<br/>
// * -noImage : do not extract images (Bitmap and Vectorial)<br/>
// * -noImageInline : do not include images inline in the stream<br/>
// * -outline : create an outline file xml<br/>
// * -annots : create an annotaitons file xml<br/>
// * -cutPages : cut all pages in separately files<br/>
// * -blocks : add blocks informations whithin the structure<br/>
// * -readingOrder : blocks follow the reading order<br/>	
// * -fullFontName : fonts names are not normalized<br/>
// * -nsURI : add the specified namespace URI<br/>
// * -q : don't print any messages or errors<br/>
// * -v : print copyright and version information<br/>
// * -h : print usage information<br/>
// * -help : print usage information<br/>
// * --help : print usage information<br/>
// * -? : print usage information<br/>	
	
	
	param = xmlNewNode(NULL,(const xmlChar*)TAG_PAR_PARAM);
	xmlNewProp(param,(const xmlChar*)"name",(const xmlChar*)"first page");
	xmlNewProp(param,(const xmlChar*)"form",(const xmlChar*)"-f");
	xmlNewProp(param,(const xmlChar*)"default",(const xmlChar*)"1");
	xmlNewProp(param,(const xmlChar*)"type",(const xmlChar*)"int");
	xmlNewProp(param,(const xmlChar*)"help",(const xmlChar*)"first page to convert");
	sprintf(tmp,"%d",firstPage);
	xmlNodeSetContent(param,(const xmlChar*)tmp);
	xmlAddChild(confNode,param);

	param = xmlNewNode(NULL,(const xmlChar*)TAG_PAR_PARAM);
	xmlNewProp(param,(const xmlChar*)"name",(const xmlChar*)"last page");
	xmlNewProp(param,(const xmlChar*)"form",(const xmlChar*)"-l");
	xmlNewProp(param,(const xmlChar*)"default",(const xmlChar*)"10000");
	xmlNewProp(param,(const xmlChar*)"type",(const xmlChar*)"int");
	xmlNewProp(param,(const xmlChar*)"help",(const xmlChar*)"last page to convert");
	if (lastPage == 0){lastPage=10000;}
	sprintf(tmp,"%d",lastPage);
	xmlNodeSetContent(param,(const xmlChar*)tmp);
	xmlAddChild(confNode,param);

	param = xmlNewNode(NULL,(const xmlChar*)TAG_PAR_PARAM);
	xmlNewProp(param,(const xmlChar*)"name",(const xmlChar*)"noText");
	xmlNewProp(param,(const xmlChar*)"form",(const xmlChar*)"-noText");
	xmlNewProp(param,(const xmlChar*)"default",(const xmlChar*)"False");
	xmlNewProp(param,(const xmlChar*)"help",(const xmlChar*)"do not extract text");
	if (!getDisplayText()){sprintf(tmp,"True");}
	else {sprintf(tmp,"False");}
	xmlNodeSetContent(param,(const xmlChar*)tmp);
	xmlAddChild(confNode,param);

	param = xmlNewNode(NULL,(const xmlChar*)TAG_PAR_PARAM);
	xmlNewProp(param,(const xmlChar*)"name",(const xmlChar*)"noImage");
	xmlNewProp(param,(const xmlChar*)"form",(const xmlChar*)"-noImage");
	xmlNewProp(param,(const xmlChar*)"default",(const xmlChar*)"False");
	xmlNewProp(param,(const xmlChar*)"help",(const xmlChar*)"do not extract images");
	if (!getDisplayImage()){sprintf(tmp,"True");}
	else  {sprintf(tmp,"False");}
	xmlNodeSetContent(param,(const xmlChar*)tmp);
	xmlAddChild(confNode,param);

//	getDisplayText() 
//	GBool getDisplayBlocks() 
//	GBool getDisplayImage() 
//	GBool getDisplayOutline() 
//	GBool getCutAllPages() 
//	GBool getFullFontName() 
//	GBool getImageInline() 
	
	xmlSaveFile(fileName,confDoc);
  	xmlFreeDoc(confDoc);
	free(tmp);
}
