//
// Created by azhar on 20/12/2017.
//

#include <aconf.h>
#ifdef USE_GCC_PRAGMAS
#pragma implementation
#endif

#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include <libxml/uri.h>
#include <time.h>
#include <string>
#include <list>
#include <vector>
#include <stack>
#define _USE_MATH_DEFINES
#include <cmath> // PL: for using std::abs

#include <iostream>
using namespace std;

#include "ConstantsUtils.h"
using namespace ConstantsUtils;

#include "ConstantsXMLALTO.h"
using namespace ConstantsXMLALTO;
#include "AnnotsXrce.h"


#include <stdio.h>
#include <stddef.h>
#include <math.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdarg.h>

#ifndef WIN32
#include <libgen.h>
#endif

#ifdef WIN32
#include <fcntl.h> // for O_BINARY
#include <io.h>    // for setmode
#include <direct.h>  // for _mkdir
#endif

#include "gmem.h"
#include "GList.h"
#include "config.h"
#include "Error.h"
#include "GlobalParams.h"
#include "UnicodeMap.h"
#include "UnicodeTypeTable.h"
#include "GfxState.h"
#include "PDFDoc.h"
#include "Outline.h"
#include "XmlAltoOutputDev.h"
#include "Link.h"
#include "Catalog.h"
#include "Parameters.h"
//#include "Page.h"

// PNG lib
#include "png.h"


#ifdef MACOS
// needed for setting type/creator of MacOS files
#include "ICSupport.h"
#endif

//------------------------------------------------------------------------
// parameters
//------------------------------------------------------------------------

// Inter-character space width which will cause addChar to start a new word.
#define minWordBreakSpace 0.1

// Negative inter-character space width, i.e., overlap, which will
// cause addChar to start a new word.
#define minDupBreakOverlap 0.3

// Maximum distance between baselines of two words on the same line,
// e.g., distance between subscript or superscript and the primary
// baseline, as a fraction of the font size.
#define maxIntraLineDelta 0.5

// Maximum distance value between the baseline of a word in a line
// and the yMin of an other word in a other line
#define maxSpacingWordsBetweenTwoLines 5

// Maximum inter-word spacing, as a fraction of the font size.
#define maxWordSpacing 1.5

// Maximum horizontal spacing which will allow a word to be pulled
// into a block.
#define maxColSpacing 0.3

// Max distance between baselines of two lines within a block, as a
// fraction of the font size.
#define maxLineSpacingDelta 1.5

// Max difference in primary font sizes on two lines in the same
// block.  Delta1 is used when examining new lines above and below the
// current block.
#define maxBlockFontSizeDelta1 0.05

// Max difference in primary,secondary coordinates (as a fraction of
// the font size) allowed for duplicated text (fake boldface, drop
// shadows) which is to be discarded.
#define dupMaxPriDelta 0.1
#define dupMaxSecDelta 0.2

//------------------------------------------------------------------------
// TextFontInfo
//------------------------------------------------------------------------

TextFontInfo::TextFontInfo(GfxState *state) {
    gfxFont = state->getFont();
    //#if TEXTOUT_WORD_LIST
    //fontName = (gfxFont && gfxFont->getOrigName()) ? gfxFont->getOrigName()->copy() : (GString *)NULL;
    fontName = (gfxFont && gfxFont->getName()) ? gfxFont->getName()->copy() : (GString *)NULL;
    //#endif
}

TextFontInfo::~TextFontInfo() {
    //#if TEXTOUT_WORD_LIST
    if (fontName) {
        delete fontName;
    }
    //#endif
}

GBool TextFontInfo::matches(GfxState *state) {
    return state->getFont() == gfxFont;
}


TextFontStyleInfo::~TextFontStyleInfo() {
    if (fontName) {
        delete fontName;
    }
//    if (fontSize) {
//        delete fontSize;
//    }
    if (fontColor) {
        delete fontColor;
    }
}

//inline bool operator==(const TextFontStyleInfo& lhs, const TextFontStyleInfo& rhs){
//    return (((lhs.getFontName())->cmp(rhs.getFontName())  == 0 ) &&  (lhs.getFontSize() == rhs.getFontSize())
//            && (lhs.getFontColor()->cmp(rhs.getFontColor())  == 0 )
//            && (lhs.getFontType() == rhs.getFontType())
//            && (lhs.getFontStyle() == rhs.getFontStyle()));
//}


//------------------------------------------------------------------------
// ImageInline
//------------------------------------------------------------------------

ImageInline::ImageInline(double xPosition, double yPosition, double width,
                         double height, int idWord, int idImage, GString* href, int index) {
    xPositionImage = xPosition;
    yPositionImage = yPosition;
    widthImage = width;
    heightImage = height;
    idWordBefore = idWord;
    idImageCurrent = idImage;
    hrefImage = href;
    idx = index;
}

ImageInline::~ImageInline() {

}

//------------------------------------------------------------------------
// TextWord
//------------------------------------------------------------------------

TextWord::TextWord(GfxState *state, int rotA, int angleDegre,
                   int angleSkewingY, int angleSkewingX, double x0, double y0,
                   int charPosA, TextFontInfo *fontA, double fontSizeA, int idCurrentWord,
                   int index) {
    GfxFont *gfxFont;
    double x, y, ascent, descent;

    rot = rotA;
    angle = angleDegre;
    charPos = charPosA;
    charLen = 0;
    font = fontA;
    italic = gFalse;
    bold = gFalse;
    serif = gFalse;
    symbolic = gFalse;
    angleSkewing_Y = angleSkewingY;
    angleSkewing_X = angleSkewingX;
    idWord = idCurrentWord;
    idx = index;

    base = 0;
    baseYmin = 0;
    fontName = NULL;

    if (state->getFont()) {
        if (state->getFont()->getName()) {
            // PDF reference guide 5.5.3 For a font subset, the PostScript name of the font�the value of the font�s
            //BaseFont entry and the font descriptor�s FontName entry�begins with a tag
            //followed by a plus sign (+). The tag consists of exactly six uppercase letters; the
            //choice of letters is arbitrary, but different subsets in the same PDF file must have
            //different tags. For example, EOODIA+Poetica is the name of a subset of Poetica�, a
            //Type 1 font. (See implementation note 62 in Appendix H.)
            fontName = strdup(state->getFont()->getName()->getCString());
            if (strstr(state->getFont()->getName()->lowerCase()->getCString(), "bold"))
                bold=gTrue;
            if (strstr(state->getFont()->getName()->lowerCase()->getCString(), "italic")|| strstr(state->getFont()->getName()->lowerCase()->getCString(), "oblique"))
                italic=gTrue;
        } else {
            fontName = NULL;
        }
        if (bold == gFalse) {
            bold = state->getFont()->isBold();
        }
        if (italic == gFalse) {
            italic = state->getFont()->isItalic();
        }
        symbolic = state->getFont()->isSymbolic();
        serif = state->getFont()->isSerif();
    }

    horizScaling = state->getHorizScaling();
    wordSpace = state->getWordSpace();
    charSpace = state->getCharSpace();
    rise = state->getRise();
    render = state->getRender();
    leading = state->getLeading();

    fontSize = fontSizeA;

    state->transform(x0, y0, &x, &y);
    if ((gfxFont = font->gfxFont)) {
        ascent = gfxFont->getAscent() * fontSize;
        descent = gfxFont->getDescent() * fontSize;
    } else {
        // this means that the PDF file draws text without a current font,
        // which should never happen
        ascent = 0.95 * fontSize;
        descent = -0.35 * fontSize;
        gfxFont = NULL;
    }

    // Rotation cases
    switch (rot) {
        case 0:
            yMin = y - ascent;
            yMax = y - descent;
            if (yMin == yMax) {
                // this is a sanity check for a case that shouldn't happen -- but
                // if it does happen, we want to avoid dividing by zero later
                yMin = y;
                yMax = y + 1;
            }
            base = y;
            baseYmin = yMin;
            break;

        case 3:
            xMin = x + descent;
            xMax = x + ascent;
            if (xMin == xMax) {
                // this is a sanity check for a case that shouldn't happen -- but
                // if it does happen, we want to avoid dividing by zero later
                xMin = x;
                xMax = x + 1;
            }
            base = x;
            baseYmin = xMin;
            break;

        case 2:
            yMin = y + descent;
            yMax = y + ascent;
            if (yMin == yMax) {
                // this is a sanity check for a case that shouldn't happen -- but
                // if it does happen, we want to avoid dividing by zero later
                yMin = y;
                yMax = y + 1;
            }
            base = y;
            baseYmin = yMin;
            break;

        case 1:
            xMin = x - ascent;
            xMax = x - descent;
            if (xMin == xMax) {
                // this is a sanity check for a case that shouldn't happen -- but
                // if it does happen, we want to avoid dividing by zero later
                xMin = x;
                xMax = x + 1;
            }
            base = x;
            baseYmin = xMin;
            break;
    }

    text = NULL;
    edge = NULL;
    len = size = 0;
    spaceAfter = gFalse;
    next = NULL;

    GfxRGB rgb;

    if ((state->getRender() & 3) == 1) {
        state->getStrokeRGB(&rgb);
    } else {
        state->getFillRGB(&rgb);
    }

    colorR = colToDbl(rgb.r);
    colorG = colToDbl(rgb.g);
    colorB = colToDbl(rgb.b);
}

TextWord::~TextWord() {
    gfree(text);
    gfree(edge);
}

void TextWord::addChar(GfxState *state, double x, double y, double dx,
                       double dy, Unicode u) {

    if (len == size) {
        size += 16;
        text = (Unicode *)grealloc(text, size * sizeof(Unicode));
        edge = (double *)grealloc(edge, (size + 1) * sizeof(double));
    }

    text[len] = u;
    switch (rot) {
        case 0:
            if (len == 0) {
                xMin = x;
            }
            edge[len] = x;
            xMax = edge[len+1] = x + dx;
            break;
        case 3:
            if (len == 0) {
                yMin = y;
            }
            edge[len] = y;
            yMax = edge[len+1] = y + dy;
            break;
        case 2:
            if (len == 0) {
                xMax = x;
            }
            edge[len] = x;
            xMin = edge[len+1] = x + dx;
            break;
        case 1:
            if (len == 0) {
                yMax = y;
            }
            edge[len] = y;
            yMin = edge[len+1] = y + dy;
            break;
    }
    ++len;
}

void TextWord::merge(TextWord *word) {
    int i;

    if (word->xMin < xMin) {
        xMin = word->xMin;
    }
    if (word->yMin < yMin) {
        yMin = word->yMin;
    }
    if (word->xMax > xMax) {
        xMax = word->xMax;
    }
    if (word->yMax > yMax) {
        yMax = word->yMax;
    }
    if (len + word->len > size) {
        size = len + word->len;
        text = (Unicode *)grealloc(text, size * sizeof(Unicode));
        edge = (double *)grealloc(edge, (size + 1) * sizeof(double));
    }
    for (i = 0; i < word->len; ++i) {
        text[len + i] = word->text[i];
        edge[len + i] = word->edge[i];
    }
    edge[len + word->len] = word->edge[word->len];
    len += word->len;
    charLen += word->charLen;
}

inline int TextWord::primaryCmp(TextWord *word) {
    double cmp;

    cmp = 0; // make gcc happy
    switch (rot) {
        case 0:
            cmp = xMin - word->xMin;
            break;
        case 3:
            cmp = yMin - word->yMin;
            break;
        case 2:
            cmp = word->xMax - xMax;
            break;
        case 1:
            cmp = word->yMax - yMax;
            break;
    }
    return cmp< 0 ? -1 : cmp> 0 ?1 : 0;
}

double TextWord::primaryDelta(TextWord *word) {
    double delta;

    delta = 0; // make gcc happy
    switch (rot) {
        case 0:
            delta = word->xMin - xMax;
            break;
        case 3:
            delta = word->yMin - yMax;
            break;
        case 2:
            delta = xMin - word->xMax;
            break;
        case 1:
            delta = yMin - word->yMax;
            break;
    }
    return delta;
}

GBool TextWord::overlap(TextWord *w2){

    return gFalse;
}

int TextWord::cmpYX(const void *p1, const void *p2) {
    TextWord *word1 = *(TextWord **)p1;
    TextWord *word2 = *(TextWord **)p2;
    double cmp;

    cmp = word1->yMin - word2->yMin;
    if (cmp == 0) {
        cmp = word1->xMin - word2->xMin;
    }
    return cmp< 0 ? -1 : cmp> 0 ?1 : 0;
}

GString *TextWord::convtoX(double xcol) const {
    GString *xret=new GString();
    char tmp;
    unsigned int k;
    k = static_cast<int>(xcol);
    k= k/16;
    if ((k>=0)&&(k<10))
        tmp=(char) ('0'+k);
    else
        tmp=(char)('a'+k-10);
    xret->append(tmp);
    k = static_cast<int>(xcol);
    k= k%16;
    if ((k>=0)&&(k<10))
        tmp=(char) ('0'+k);
    else
        tmp=(char)('a'+k-10);
    xret->append(tmp);

    return xret;
}

GString *TextWord::colortoString() const {
    GString *tmp=new GString("#");
    GString *tmpr=convtoX(static_cast<int>(255*colorR));
    GString *tmpg=convtoX(static_cast<int>(255*colorG));
    GString *tmpb=convtoX(static_cast<int>(255*colorB));
    tmp->append(tmpr);
    tmp->append(tmpg);
    tmp->append(tmpb);

    delete tmpr;
    delete tmpg;
    delete tmpb;
    return tmp;
}

const char* TextWord::normalizeFontName(char* fontName) {
    string name (fontName);
    string name2;
    string name3;
    char * cstr;

    size_t position = name.find_first_of('+');
    if (position != string::npos) {
        name2 = name.substr(position+1, name.size()-1);
    }
    else{
        name2 = fontName;
    }
    position = name2.find_first_of('-');
    if (position != string::npos) {
        name3 = name2.substr(0, position);
    }
    else{
        name3 =name2;
    }
    cstr = new char [name3.size()+1];
    strcpy (cstr, name3.c_str());
//        printf("\t%s\t%s\n",fontName,cstr);
    return cstr;

}

//------------------------------------------------------------------------
// TextPage
//------------------------------------------------------------------------

TextPage::TextPage(GBool verboseA, Catalog *catalog, xmlNodePtr node,
                   GString* dir, GString *base, GString *nsURIA) {

    root = node;
    verbose = verboseA;
    rawOrder = 1;

    // PL: to modify block order according to reading order
    if (parameters->getReadingOrder())
        readingOrder = 1;
    else
        readingOrder = 0;
    curWord = NULL;
    charPos = 0;
    curFont = NULL;
    curFontSize = 0;
    nest = 0;
    nTinyChars = 0;
    lastCharOverlap = gFalse;
    idClip = 0;
    stack<int> idStack;
    idCur = 0;
    curstate = NULL;
    myCat = catalog;

    idx = 0; //EG

    if (nsURIA) {
        namespaceURI = new GString(nsURIA);
    } else {
        namespaceURI = NULL;
    }

    rawWords = NULL;
    rawLastWord = NULL;
    fonts = new GList();
    lastFindXMin = lastFindYMin = 0;
    haveLastFind = gFalse;

    if (parameters->getDisplayImage()) {
        RelfileName = new GString(dir);
        ImgfileName = new GString(base);
    }
}

TextPage::~TextPage() {
    clear();
    delete fonts;
    if (namespaceURI) {
        delete namespaceURI;
    }
}

void TextPage::startPage(int pageNum, GfxState *state, GBool cut) {
    clear();
    char *tmp;
    cutter = cut;
    num = pageNum;
    numToken = 1;
    numImage = 1;
    idWORD = 0;
    indiceImage = -1;
    idWORDBefore = -1;

    PDFRectangle *mediaBox = myCat->getPage(pageNum)->getMediaBox();
    PDFRectangle *cropBox = myCat->getPage(pageNum)->getCropBox();
    PDFRectangle *trimBox = myCat->getPage(pageNum)->getTrimBox();
    PDFRectangle *bleedBox = myCat->getPage(pageNum)->getBleedBox();
    PDFRectangle *artBox = myCat->getPage(pageNum)->getArtBox();


    page = xmlNewNode(NULL, (const xmlChar*)TAG_PAGE);
    page->type = XML_ELEMENT_NODE;
    if (state) {
        pageWidth = state->getPageWidth();
        pageHeight = state->getPageHeight();
    } else {
        pageWidth = pageHeight = 0;
    }
    curstate=state;

    tmp = (char*)malloc(20*sizeof(char));

    sprintf(tmp, "%d", pageNum);

    GString *id;
    id = new GString("Page");
    id->append(tmp);
    xmlNewProp(page, (const xmlChar*)ATTR_ID, (const xmlChar*)id->getCString());
    delete id;

    xmlNewProp(page, (const xmlChar*)ATTR_PHYSICAL_IMG_NR, (const xmlChar*)tmp);

    sprintf(tmp, "%g", pageWidth);
    xmlNewProp(page, (const xmlChar*)ATTR_WIDTH, (const xmlChar*)tmp);
    sprintf(tmp, "%g", pageHeight);
    xmlNewProp(page, (const xmlChar*)ATTR_HEIGHT, (const xmlChar*)tmp);

//    xmlNodePtr mediaboxtag = NULL;
//    mediaboxtag = xmlNewNode(NULL, (const xmlChar*)TAG_MEDIABOX);
//    xmlAddChild(page,mediaboxtag);
//    tmp = (char*)malloc(20*sizeof(char));
//    sprintf(tmp, "%g", mediaBox->x1);
//    xmlNewProp(mediaboxtag, (const xmlChar*)ATTR_X1, (const xmlChar*)tmp);
//    sprintf(tmp, "%g", mediaBox->y1);
//    xmlNewProp(mediaboxtag, (const xmlChar*)ATTR_Y1, (const xmlChar*)tmp);
//    sprintf(tmp, "%g", mediaBox->x2);
//    xmlNewProp(mediaboxtag, (const xmlChar*)ATTR_X2, (const xmlChar*)tmp);
//    sprintf(tmp, "%g", mediaBox->y2);
//    xmlNewProp(mediaboxtag, (const xmlChar*)ATTR_Y2, (const xmlChar*)tmp);
//
//    xmlNodePtr cropboxtag = NULL;
//    cropboxtag = xmlNewNode(NULL, (const xmlChar*)TAG_CROPBOX);
//    xmlAddChild(page,cropboxtag);
//    tmp = (char*)malloc(20*sizeof(char));
//    sprintf(tmp, "%g", cropBox->x1);
//    xmlNewProp(cropboxtag, (const xmlChar*)ATTR_X1, (const xmlChar*)tmp);
//    sprintf(tmp, "%g", cropBox->y1);
//    xmlNewProp(cropboxtag, (const xmlChar*)ATTR_Y1, (const xmlChar*)tmp);
//    sprintf(tmp, "%g", cropBox->x2);
//    xmlNewProp(cropboxtag, (const xmlChar*)ATTR_X2, (const xmlChar*)tmp);
//    sprintf(tmp, "%g", cropBox->y2);
//    xmlNewProp(cropboxtag, (const xmlChar*)ATTR_Y2, (const xmlChar*)tmp);
//
//    xmlNodePtr bleedboxtag = NULL;
//    bleedboxtag = xmlNewNode(NULL, (const xmlChar*)TAG_BLEEDBOX);
//    xmlAddChild(page,bleedboxtag);
//    tmp = (char*)malloc(20*sizeof(char));
//    sprintf(tmp, "%g", bleedBox->x1);
//    xmlNewProp(bleedboxtag, (const xmlChar*)ATTR_X1, (const xmlChar*)tmp);
//    sprintf(tmp, "%g", bleedBox->y1);
//    xmlNewProp(bleedboxtag, (const xmlChar*)ATTR_Y1, (const xmlChar*)tmp);
//    sprintf(tmp, "%g", bleedBox->x2);
//    xmlNewProp(bleedboxtag, (const xmlChar*)ATTR_X2, (const xmlChar*)tmp);
//    sprintf(tmp, "%g", bleedBox->y2);
//    xmlNewProp(bleedboxtag, (const xmlChar*)ATTR_Y2, (const xmlChar*)tmp);
//
//    xmlNodePtr artboxtag = NULL;
//    artboxtag = xmlNewNode(NULL, (const xmlChar*)TAG_ARTBOX);
//    xmlAddChild(page,artboxtag);
//    tmp = (char*)malloc(20*sizeof(char));
//    sprintf(tmp, "%g", artBox->x1);
//    xmlNewProp(artboxtag, (const xmlChar*)ATTR_X1, (const xmlChar*)tmp);
//    sprintf(tmp, "%g", artBox->y1);
//    xmlNewProp(artboxtag, (const xmlChar*)ATTR_Y1, (const xmlChar*)tmp);
//    sprintf(tmp, "%g", artBox->x2);
//    xmlNewProp(artboxtag, (const xmlChar*)ATTR_X2, (const xmlChar*)tmp);
//    sprintf(tmp, "%g", artBox->y2);
//    xmlNewProp(artboxtag, (const xmlChar*)ATTR_Y2, (const xmlChar*)tmp);
//
//
//    xmlNodePtr trimboxtag = NULL;
//    trimboxtag = xmlNewNode(NULL, (const xmlChar*)TAG_TRIMBOX);
//    xmlAddChild(page,trimboxtag);
//    tmp = (char*)malloc(20*sizeof(char));
//    sprintf(tmp, "%g", trimBox->x1);
//    xmlNewProp(trimboxtag, (const xmlChar*)ATTR_X1, (const xmlChar*)tmp);
//    sprintf(tmp, "%g", trimBox->y1);
//    xmlNewProp(trimboxtag, (const xmlChar*)ATTR_Y1, (const xmlChar*)tmp);
//    sprintf(tmp, "%g", trimBox->x2);
//    xmlNewProp(trimboxtag, (const xmlChar*)ATTR_X2, (const xmlChar*)tmp);
//    sprintf(tmp, "%g", trimBox->y2);
//    xmlNewProp(trimboxtag, (const xmlChar*)ATTR_Y2, (const xmlChar*)tmp);

    // 26/04/2010: commented
//	if (pageWidth>700 && pageHeight<700) {
//		if ((pageHeight==841 || pageHeight==842) && pageWidth==1224) {
//			xmlNewProp(page, (const xmlChar*)ATTR_FORMAT, (const xmlChar*)"A3");
//		} else {
//			xmlNewProp(page, (const xmlChar*)ATTR_FORMAT,
//					(const xmlChar*)"landscape");
//		}
//	}

    // Cut all pages OK
    if (cutter) {
        docPage = xmlNewDoc((const xmlChar*)VERSION);
        globalParams->setTextEncoding((char*)ENCODING_UTF8);
        docPage->encoding = xmlStrdup((const xmlChar*)ENCODING_UTF8);
        xmlDocSetRootElement(docPage, page);
    } else {
        xmlAddChild(root, page);
    }

    //  	fprintf(stderr, "Page %d\n",pageNum);
    //  	fflush(stderr);

    // New file for vectorials instructions
    vecdoc = xmlNewDoc((const xmlChar*)VERSION);
    globalParams->setTextEncoding((char*)ENCODING_UTF8);
    vecdoc->encoding = xmlStrdup((const xmlChar*)ENCODING_UTF8);
    vecroot = xmlNewNode(NULL, (const xmlChar*)TAG_VECTORIALIMAGES);

    // Add the namespace DS of the vectorial instructions file
    if (namespaceURI) {
        xmlNewNs(vecroot, (const xmlChar*)namespaceURI->getCString(), NULL);
    }

    xmlDocSetRootElement(vecdoc, vecroot);

    // for links
    //  store them in a list
    //  and when dump: for each token look at intersectionwith
    Page *currentPage;
    Object objAnnot;
    double ctm[6];
    Object kid;
    Object objSubtype;

    int i;
    for (i = 0; i < 6; ++i) {
        ctm[i] = state->getCTM()[i];
    }

//  	printf("start annotations\n");
    currentPage = myCat->getPage(num);
    currentPage->getAnnots(&objAnnot);
    //pageLinks = currentPage->getLinks(myCat);
    pageLinks = currentPage->getLinks();

    // Annotation's objects list
    if (objAnnot.isArray()){
        for (int i = 0 ; i < objAnnot.arrayGetLength() ; ++i){
            objAnnot.arrayGet(i, &kid);
            if (kid.isDict()) {
                Dict *dict;
                dict = kid.getDict();
                // Get the annotation's type
                if (dict->lookup("Subtype", &objSubtype)->isName()){
                    // It can be 'Highlight' or 'Underline' or 'Link' (Subtype 'Squiggly' or 'StrikeOut' are not supported)
                    if (!strcmp(objSubtype.getName(), "Highlight")){
                        highlightedObject.push_back(dict);
                    }
                    if(!strcmp(objSubtype.getName(), "Underline")){
                        underlineObject.push_back(dict);
                    }
                }
                objSubtype.free();
            }
        }
    }
    objAnnot.free();

    free(tmp);
}


void TextPage::configuration() {
    if (curWord) {
        endWord();
    }
    if (rawOrder) {
        primaryRot = 0;
        primaryLR = gTrue;
    }
}



void TextPage::endPage(GString *dataDir) {
    if (curWord) {
        endWord();
    }

    if (parameters->getDisplayImage()) {

        xmlNodePtr xiinclude=NULL;
        xmlNsPtr xiNs = NULL;

        GString *relname = new GString(RelfileName);
        relname->append("-");
        relname->append(GString::fromInt(num));
        relname->append(EXTENSION_VEC);

        GString *refname = new GString(ImgfileName);
        refname->append("-");
        refname->append(GString::fromInt(num));
        refname->append(EXTENSION_VEC);

        xiNs=xmlNewNs(NULL, (const xmlChar*)XI_URI, (const xmlChar*)XI_PREFIX);
        if (xiNs) {
            xiinclude = xmlNewNode(xiNs, (const xmlChar*)XI_INCLUDE);
            xiNs=xmlNewNs(xiinclude, (const xmlChar*)XI_URI,
                          (const xmlChar*)XI_PREFIX);
            xmlSetNs(xiinclude, xiNs);
            if (cutter) {
                // Change the relative path of vectorials images when all pages are cutted
                GString *imageName = new GString("image");
                imageName->append("-");
                imageName->append(GString::fromInt(num));
                imageName->append(EXTENSION_VEC);
                // ID: 1850760
                GString *cp;
                cp = refname->copy();
                for (int i=0;i<cp->getLength();i++){
                    if (cp->getChar(i) ==' '){
                        cp->del(i);
                        cp->insert(i,"%20");
                    }
                }
//				printf("%s\n",imageName->getCString());
                xmlNewProp(xiinclude, (const xmlChar*)ATTR_HREF,
                           (const xmlChar*)cp->getCString());
                delete imageName;
                delete cp;
            } else {
                // ID: 1850760
                GString *cp;
                cp = refname->copy();
                for (int i=0;i<cp->getLength();i++){
                    if (cp->getChar(i) ==' '){
                        cp->del(i);
                        cp->insert(i,"%20");
                    }
                }
//				printf("%s\n",cp->getCString());
                xmlNewProp(xiinclude, (const xmlChar*)ATTR_HREF,
                           (const xmlChar*)cp->getCString());
                delete cp;
            }

            if (namespaceURI) {
                xmlNewNs(xiinclude, (const xmlChar*)namespaceURI->getCString(),
                         NULL);
            }
            xmlAddChild(page, xiinclude);
        } else {
            fprintf(stderr, "namespace %s : impossible creation\n", XI_PREFIX);
        }

        // Save the file for example with relname 'p_06.xml_data/image-27.vec'
        if (!xmlSaveFile(relname->getCString(), vecdoc)) {
            //error(errIO,-1, "Couldn't open file '%s'", relname->getCString());
        }

        delete refname;
        delete relname;
    }
    xmlFreeDoc(vecdoc);

    // PL: here we look at the blocks order
    /*if (readingOrder) {
		cout  << endl << "new page" << endl;
		xmlNode *cur_node = NULL;
		// we get the first child of the current page node
		unsigned long nbChildren = xmlChildElementCount(page);
		if (nbChildren > 0) {
			xmlNodePtr firstPageItem = xmlFirstElementChild(page);
			// we get all the block nodes in the XML tree corresponding to the page
			for (cur_node = firstPageItem; cur_node; cur_node = cur_node->next) {
	        	if ( (cur_node->type == XML_ELEMENT_NODE) && (strcmp((const char*)cur_node->name, TAG_BLOCK) ==0) ) {
	        		cout << "node type: Element, name: " << cur_node->name << endl;
					// if a block is located in the layout above another block and
					// there is not overlapping surface on the x axis between the blocks
					// we can swap the blocks
	        		double currentMinX = 0;
					double currentMinY = 0;
					double currentMaxX = 0;
					double currentMaxY = 0;

					xmlChar *attrValue = xmlGetProp(cur_node, (const xmlChar*)ATTR_X);
					if (attrValue != NULL) {
						cout << "X: " << attrValue << endl;
						xmlFree(attrValue);
					}

					attrValue = xmlGetProp(cur_node, (const xmlChar*)ATTR_Y);
					if (attrValue != NULL) {
						cout << "Y:" << attrValue << endl;
						xmlFree(attrValue);
					}

					attrValue = xmlGetProp(cur_node, (const xmlChar*)ATTR_HEIGHT);
					if (attrValue != NULL) {
						cout << "height: " << attrValue << endl;
						xmlFree(attrValue);
					}

					attrValue = xmlGetProp(cur_node, (const xmlChar*)ATTR_WIDTH);
					if (attrValue != NULL) {
						cout << "width: " << attrValue << endl;
						xmlFree(attrValue);
					}

					xmlAttrPtr attX = xmlHasProp(cur_node, (const xmlChar*)ATTR_X);
					xmlAttrPtr attY = xmlHasProp(cur_node, (const xmlChar*)ATTR_Y);
					xmlAttrPtr attHeight = xmlHasProp(cur_node, (const xmlChar*)ATTR_HEIGHT);
					xmlAttrPtr attWidth = xmlHasProp(cur_node, (const xmlChar*)ATTR_WIDTH);
	        	}
	        }
	    }
	}*/

    // IF cutter is ok we build the file name for all pages separately
    // and save all files in the data directory
    if (cutter) {
        dataDirectory = new GString(dataDir);
        GString *pageFile = new GString(dataDirectory);
        pageFile->append("/pageNum-");
        pageFile->append(GString::fromInt(num));
        pageFile->append(EXTENSION_XML);

        if (!xmlSaveFile(pageFile->getCString(), docPage)) {

            //	error(-1, "Couldn't open file '%s'", pageFile->getCString());
        }

        // Add in the principal file XML all pages as a tag xi:include
        xmlNodePtr nodeXiInclude = NULL;
        xmlNsPtr nsXi = xmlNewNs(NULL, (const xmlChar*)XI_URI,
                                 (const xmlChar*)XI_PREFIX);
        if (nsXi) {
            nodeXiInclude = xmlNewNode(nsXi, (const xmlChar*)XI_INCLUDE);
            nsXi = xmlNewNs(nodeXiInclude, (const xmlChar*)XI_URI,
                            (const xmlChar*)XI_PREFIX);
            xmlSetNs(nodeXiInclude, nsXi);
            xmlNewProp(nodeXiInclude, (const xmlChar*)ATTR_HREF,
                       (const xmlChar*)pageFile->getCString());
            if (namespaceURI) {
                xmlNewNs(nodeXiInclude,
                         (const xmlChar*)namespaceURI->getCString(), NULL);
            }
            xmlAddChild(root, nodeXiInclude);
        }
        delete pageFile;
    }
    highlightedObject.clear();
    underlineObject.clear();
}

void TextPage::clear() {
    TextWord *word;

    if (curWord) {
        delete curWord;
        curWord = NULL;
    }
    if (rawOrder) {
        while (rawWords) {
            word = rawWords;
            rawWords = rawWords->next;
            delete word;
        }
    }
    deleteGList(fonts, TextFontInfo);

    curWord = NULL;
    charPos = 0;
    curFont = NULL;
    curFontSize = 0;
    nest = 0;
    nTinyChars = 0;

    rawWords = NULL;
    rawLastWord = NULL;
    fonts = new GList();

    // Clear the vector which contain images inline objects
    int nb = listeImageInline.size();
    for (int i=0; i<nb; i++) {
        delete listeImageInline[i];
    }
    listeImageInline.clear();

}

void TextPage::updateFont(GfxState *state) {

    GfxFont *gfxFont;
    double *fm;
    char *name;
    int code, mCode, letterCode, anyCode;
    double w;
    int i;

    // get the font info object
    curFont = NULL;
    for (i = 0; i < fonts->getLength(); ++i) {
        curFont = (TextFontInfo *)fonts->get(i);

        if (curFont->matches(state)) {
            break;
        }
        curFont = NULL;
    }

    if (!curFont) {
        curFont = new TextFontInfo(state);
        fonts->append(curFont);
    }

    // adjust the font size
    gfxFont = state->getFont();

    curFontSize = state->getTransformedFontSize();
    if (gfxFont && gfxFont->getType() == fontType3) {
        // This is a hack which makes it possible to deal with some Type 3
        // fonts.  The problem is that it's impossible to know what the
        // base coordinate system used in the font is without actually
        // rendering the font.  This code tries to guess by looking at the
        // width of the character 'm' (which breaks if the font is a
        // subset that doesn't contain 'm').
        mCode = letterCode = anyCode = -1;
        for (code = 0; code < 256; ++code) {
            name = ((Gfx8BitFont *)gfxFont)->getCharName(code);
            if (name && name[0] == 'm' && name[1] == '\0') {
                mCode = code;
            }
            if (letterCode < 0 && name && name[1] == '\0' && ((name[0] >= 'A'
                                                               && name[0] <= 'Z') || (name[0] >= 'a' && name[0] <= 'z'))) {
                letterCode = code;
            }
            if (anyCode< 0 && name && ((Gfx8BitFont *)gfxFont)->getWidth(code) > 0) {anyCode = code;
            }
        }
        if (mCode >= 0 &&
            (w = ((Gfx8BitFont *)gfxFont)->getWidth(mCode)) > 0) {
            // 0.6 is a generic average 'm' width -- yes, this is a hack
            curFontSize *= w / 0.6;
        } else if (letterCode >= 0 &&
                   (w = ((Gfx8BitFont *)gfxFont)->getWidth(letterCode)) > 0) {
            // even more of a hack: 0.5 is a generic letter width
            curFontSize *= w / 0.5;
        } else if (anyCode >= 0 &&
                   (w = ((Gfx8BitFont *)gfxFont)->getWidth(anyCode)) > 0) {
            // better than nothing: 0.5 is a generic character width
            curFontSize *= w / 0.5;
        }
        fm = gfxFont->getFontMatrix();
        if (fm[0] != 0) {
            curFontSize *= fabs(fm[3] / fm[0]);
        }
    }
}

void TextPage::beginWord(GfxState *state, double x0, double y0) {

    double *fontm;
    double m[4];
    double m2[4];
    int rot;
    int angle;
    int angleSkewingY = 0;
    int angleSkewingX = 0;
    double tan;

    // This check is needed because Type 3 characters can contain
    // text-drawing operations (when TextPage is being used via
    // {X,Win}SplashOutputDev rather than TextOutputDev).
    if (curWord) {
        ++nest;
        return;
    }

    // Compute the rotation
    state->getFontTransMat(&m[0], &m[1], &m[2], &m[3]);

    if (state->getFont()->getType() == fontType3) {
        fontm = state->getFont()->getFontMatrix();
        m2[0] = fontm[0] * m[0] + fontm[1] * m[2];
        m2[1] = fontm[0] * m[1] + fontm[1] * m[3];
        m2[2] = fontm[2] * m[0] + fontm[3] * m[2];
        m2[3] = fontm[2] * m[1] + fontm[3] * m[3];
        m[0] = m2[0];
        m[1] = m2[1];
        m[2] = m2[2];
        m[3] = m2[3];
    }

    if (fabs(m[0] * m[3]) > fabs(m[1] * m[2])) {
        rot = (m[3] < 0) ? 0 : 2;
    } else {
        rot = (m[2] > 0) ? 3 : 1;
    }

    // Get the tangent
    tan = m[2]/m[0];
    // Get the angle value in radian
    tan = atan(tan);
    // To convert radian angle to degree angle
    tan = 180 * tan / M_PI;

    angle = static_cast<int>(tan);

    // Adjust the angle value
    switch (rot) {
        case 0:
            if (angle>0)
                angle = 360 - angle;
            else
                angle = static_cast<int>(fabs(static_cast<double>(angle)));
            break;
        case 1:
            if (angle>0)
                angle = 180 - angle;
            else
                angle = static_cast<int>(fabs(static_cast<double>(angle)));
            break;
        case 2:
            if (angle>0)
                angle = 180 - angle;
            else
                angle = static_cast<int>(fabs(static_cast<double>(angle))) + 180;
            break;
        case 3:
            if (angle>0)
                angle = 360 - angle;
            else
                angle = static_cast<int>(fabs(static_cast<double>(angle))) + 180;
            break;
    }

    // Recover the skewing angle value
    if (m[1]==0 && m[2]!=0) {
        double angSkew = atan(m[2]);
        angSkew = (180 * angSkew / M_PI) - 90;
        angleSkewingY = static_cast<int>(angSkew);
        if (rot == 0) {
            angle = 0;
        }
    }

    if (m[1]!=0 && m[2]==0) {
        double angSkew = atan(m[1]);
        angSkew = 180 * angSkew / M_PI;
        angleSkewingX = static_cast<int>(angSkew);
        if (rot == 0) {
            angle = 0;
        }
    }

    // Increment the absolute object index
    idx++;

    curWord
            = new TextWord(state, rot, angle, angleSkewingY, angleSkewingX, x0, y0, charPos, curFont, curFontSize, getIdWORD(), getIdx());
}

void TextPage::addChar(GfxState *state, double x, double y, double dx,
                       double dy, CharCode c, int nBytes, Unicode *u, int uLen) {

    double x1, y1, w1, h1, dx2, dy2, base, sp, delta;
    GBool overlap;
    int i;

    if (uLen == 0) {
        endWord();
        return;
    }

    // if the previous char was a space, addChar will have called
    // endWord, so we need to start a new word
    if (!curWord) {
        beginWord(state, x, y);
    }

    // throw away chars that aren't inside the page bounds
    state->transform(x, y, &x1, &y1);
    if (x1 < 0 || x1 > pageWidth ||
        y1 < 0 || y1 > pageHeight)
    {
        charPos += nBytes;
        endWord();
        return;
    }

    // subtract char and word spacing from the dx,dy values // HD why ??
    sp = state->getCharSpace();
    if (c == (CharCode)0x20) {
        sp += state->getWordSpace();
    }
    state->textTransformDelta(sp * state->getHorizScaling(), 0, &dx2, &dy2);
    dx -= dx2; //HD
    dy -= dy2; //HD
    state->transformDelta(dx, dy, &w1, &h1);

    // check the tiny chars limit
    if (!globalParams->getTextKeepTinyChars() && fabs(w1) < 3 && fabs(h1) < 3) {
        if (++nTinyChars > 50000) {
            charPos += nBytes;
            return;
        }
    }

    // break words at space character
    if (uLen == 1 && u[0] == (Unicode)0x20) {
        if (curWord) {
            ++curWord->charLen;
        }
        charPos += nBytes;
        endWord();
        return;
    }


    if (!curWord) {
        beginWord(state, x, y);
    }

    // start a new word if:
    // (1) this character doesn't fall in the right place relative to
    //     the end of the previous word (this places upper and lower
    //     constraints on the position deltas along both the primary
    //     and secondary axes), or
    // (2) this character overlaps the previous one (duplicated text), or
    // (3) the previous character was an overlap (we want each duplicated
    //     character to be in a word by itself at this stage)
    //     characters to be in a word by itself) // HD deleted
    if (curWord && curWord->len > 0) {
        base = sp = delta = 0; // make gcc happy
        switch (curWord->rot) {
            case 0:
                base = y1;
                sp = x1 - curWord->xMax;
                delta = x1 - curWord->edge[curWord->len - 1];
                break;
            case 3:
                base = x1;
                sp = y1 - curWord->yMax;
                delta = y1 - curWord->edge[curWord->len - 1];
                break;
            case 2:
                base = y1;
                sp = curWord->xMin - x1;
                delta = curWord->edge[curWord->len - 1] - x1;
                break;
            case 1:
                base = x1;
                sp = curWord->yMin - y1;
                delta = curWord->edge[curWord->len - 1] - y1;
                break;
        }
        sp -= curWord->charSpace;
        curWord->charSpace = state->getCharSpace();
        overlap = fabs(delta) < dupMaxPriDelta * curWord->fontSize && fabs(base
                                                                           - curWord->base) < dupMaxSecDelta * curWord->fontSize;

        // take into account rotation angle ??
        if ( (overlap || fabs(base - curWord->base) > 1 ||
              sp > minWordBreakSpace * curWord->fontSize ||
              sp < -minDupBreakOverlap * curWord->fontSize)) {
            endWord();
            beginWord(state, x, y);
        }
        lastCharOverlap = overlap;
    } else {
        lastCharOverlap = gFalse;
    }

    if (uLen != 0) {
        if (!curWord) {
            beginWord(state, x, y);
        }
        // page rotation and/or transform matrices can cause text to be
        // drawn in reverse order -- in this case, swap the begin/end
        // coordinates and break text into individual chars
        if ((curWord->rot == 0 && w1 < 0) || (curWord->rot == 3 && h1 < 0)
            || (curWord->rot == 2 && w1 > 0) || (curWord->rot == 1 && h1
                                                                      > 0)) {
            endWord();
            beginWord(state, x + dx, y + dy);
            x1 += w1;
            y1 += h1;
            w1 = -w1;
            h1 = -h1;
        }

        // add the characters to the current word
        w1 /= uLen;
        h1 /= uLen;

        for (i = 0; i < uLen; ++i) {
            curWord->addChar(state, x1 + i*w1, y1 + i*h1, w1, h1, u[i]);
        }
    }

    if (curWord) {
        curWord->charLen += nBytes;
    }
    charPos += nBytes;

}

void TextPage::endWord() {
    // This check is needed because Type 3 characters can contain
    // text-drawing operations (when TextPage is being used via
    // {X,Win}SplashOutputDev rather than TextOutputDev).
    if (nest > 0) {
        --nest;
        return;
    }

    if (curWord) {
        addWord(curWord);
        curWord = NULL;
        idWORD++;
    }

}

void TextPage::addWord(TextWord *word) {
    // throw away zero-length words -- they don't have valid xMin/xMax
    // values, and they're useless anyway
    if (word->len == 0) {
        delete word;
        return;
    }

    if (rawOrder) {
        if (rawLastWord) {
            rawLastWord->next = word;
        } else {
            rawWords = word;
        }
        rawLastWord = word;
    }
}

void TextPage::addAttributTypeReadingOrder(xmlNodePtr node, char* tmp,
                                           TextWord *word) {
    int nbLeft = 0;
    int nbRight = 0;

    // Recover the reading order for each characters of the word
    for (int i = 0; i < word->len; ++i) {
        if (unicodeTypeR(word->text[i])) {
            nbLeft++;
        } else {
            nbRight++;
        }
    }
    // IF there is more character where the reading order is left to right
    // then we add the type attribute with a true value
    if (nbRight<nbLeft) {
        sprintf(tmp, "%d", gTrue);
        xmlNewProp(node, (const xmlChar*)ATTR_TYPE, (const xmlChar*)tmp);
    }
}

void TextPage::addAttributsNodeVerbose(xmlNodePtr node, char* tmp,
                                       TextWord *word) {
    sprintf(tmp, "%d", word->angleSkewing_Y);
    xmlNewProp(node, (const xmlChar*)ATTR_ANGLE_SKEWING_Y, (const xmlChar*)tmp);
    sprintf(tmp, "%d", word->angleSkewing_X);
    xmlNewProp(node, (const xmlChar*)ATTR_ANGLE_SKEWING_X, (const xmlChar*)tmp);
    sprintf(tmp, "%g", word->leading);
    xmlNewProp(node, (const xmlChar*)ATTR_LEADING, (const xmlChar*)tmp);
    sprintf(tmp, "%g", word->render);
    xmlNewProp(node, (const xmlChar*)ATTR_RENDER, (const xmlChar*)tmp);
    sprintf(tmp, "%g", word->rise);
    xmlNewProp(node, (const xmlChar*)ATTR_RISE, (const xmlChar*)tmp);
    sprintf(tmp, "%g", word->horizScaling);
    xmlNewProp(node, (const xmlChar*)ATTR_HORIZ_SCALING, (const xmlChar*)tmp);
    sprintf(tmp, "%g", word->wordSpace);
    xmlNewProp(node, (const xmlChar*)ATTR_WORD_SPACE, (const xmlChar*)tmp);
    sprintf(tmp, "%g", word->charSpace);
    xmlNewProp(node, (const xmlChar*)ATTR_CHAR_SPACE, (const xmlChar*)tmp);
}

void TextPage::addAttributsNode(xmlNodePtr node, TextWord *word, double &xMaxi,
                                double &yMaxi, double &yMinRot, double &yMaxRot, double &xMinRot,
                                double &xMaxRot, TextFontStyleInfo *fontStyleInfo) {

    char *tmp;
    tmp=(char*)malloc(10*sizeof(char));

    if (word->font != NULL) {
        if (word->font->isSymbolic()) {
            xmlNewProp(node, (const xmlChar*)ATTR_SYMBOLIC, (const xmlChar*)YES);
        }
        if (word->font->isSerif()) {
            xmlNewProp(node, (const xmlChar*)ATTR_SERIF, (const xmlChar*)YES);
            fontStyleInfo->setFontType(word->font->isSerif());
        }
        if (word->font->isFixedWidth()) {
            xmlNewProp(node, (const xmlChar*)ATTR_FIXED_WIDTH,
                       (const xmlChar*)YES);
            fontStyleInfo->setFontStyle(word->font->isFixedWidth());
        }
    }

    if (word->isBold())
        xmlNewProp(node, (const xmlChar*)ATTR_BOLD, (const xmlChar*)YES);
    else
        xmlNewProp(node, (const xmlChar*)ATTR_BOLD, (const xmlChar*)NO);

    if (word->isItalic())
        xmlNewProp(node, (const xmlChar*)ATTR_ITALIC, (const xmlChar*)YES);
    else
        xmlNewProp(node, (const xmlChar*)ATTR_ITALIC, (const xmlChar*)NO);

    sprintf(tmp, "%g", word->fontSize);
    xmlNewProp(node, (const xmlChar*)ATTR_FONT_SIZE, (const xmlChar*)tmp);
    fontStyleInfo->setFontSize(word->fontSize);


    xmlNewProp(node, (const xmlChar*)ATTR_FONT_COLOR,
               (const xmlChar*)word->colortoString()->getCString());
    fontStyleInfo->setFontColor(word->colortoString());

    sprintf(tmp, "%d", word->rot);
    xmlNewProp(node, (const xmlChar*)ATTR_ROTATION, (const xmlChar*)tmp);

    sprintf(tmp, "%d", word->angle);
    xmlNewProp(node, (const xmlChar*)ATTR_ANGLE, (const xmlChar*)tmp);

    sprintf(tmp, ATTR_NUMFORMAT, word->xMin);
    xmlNewProp(node, (const xmlChar*)ATTR_X, (const xmlChar*)tmp);

    sprintf(tmp, ATTR_NUMFORMAT, word->yMin);
    xmlNewProp(node, (const xmlChar*)ATTR_Y, (const xmlChar*)tmp);

    sprintf(tmp, ATTR_NUMFORMAT, word->base);
    xmlNewProp(node, (const xmlChar*)ATTR_BASE, (const xmlChar*)tmp);

    sprintf(tmp, ATTR_NUMFORMAT, word->xMax - word->xMin);
    xmlNewProp(node, (const xmlChar*)ATTR_WIDTH, (const xmlChar*)tmp);
    if (word->xMax > xMaxi) {
        xMaxi = word->xMax;
    }
    if (word->xMin < xMinRot) {
        xMinRot = word->xMin;
    }
    if (word->xMax > xMaxRot) {
        xMaxRot = word->xMax;
    }

    sprintf(tmp, ATTR_NUMFORMAT, word->yMax - word->yMin);
    xmlNewProp(node, (const xmlChar*)ATTR_HEIGHT, (const xmlChar*)tmp);
    if (word->yMax > yMaxi) {
        yMaxi = word->yMax;
    }
    if (word->yMin < yMinRot) {
        yMinRot = word->yMin;
    }
    if (word->yMax > yMaxRot) {
        yMaxRot = word->yMax;
    }

    GBool contains = gFalse;

    for(int x = 0; x < fontStyles.size(); x++) {
        if( fontStyleInfo->cmp(fontStyles[x]) ) {
            contains = gTrue;
            break;
        }
    }

    if(!contains) {
        fontStyleInfo->setId(fontStyles.size());
        sprintf(tmp, "font%d", fontStyleInfo->getId());
        //xmlNewProp(node, (const xmlChar*)ATTR_STYLEREFS, (const xmlChar*)tmp);
        fontStyles.push_back(fontStyleInfo);
    }

//        cout << "size :"
//             << fontStyles.size()  << std::endl;

    free(tmp);
}
void TextPage::testLinkedText(xmlNodePtr node,double xMin,double yMin,double xMax,double yMax){
    /*
	 * first test if overlap
	 * then create stuff for ml node:
	 * if uri:  ad @href= value
	 * if goto:  add @hlink = ...??what !! = page and position values
	 */
    GString idvalue = new GString("X");
    Link *link;
    LinkAction* action;

    char* tmp;
    tmp=(char*)malloc(50*sizeof(char));

    for (int j=0;j<pageLinks->getNumLinks();++j){
//		printf("link num:%d\n",j);
        //get rectangle
        link = pageLinks->getLink(j);
        if (link->isOk()){
            GBool isOverlap;
            double x1,y1,x2,y2;
            double xa1,xa2,ya1,ya2;
            double tmpx;
            action = link->getAction();
            if (action->isOk()){
                link->getRect(&x1,&y1,&x2,&y2);
                curstate->transform(x1,y1,&xa1,&ya1);
                curstate->transform(x2,y2,&xa2,&ya2);
                tmpx=ya1;
                ya1=min(ya1,ya2);
                ya2=max(tmpx,ya2);
                //		printf("link %d %g %g %g %g\n",action->getKind(),xa1,ya1,xa2,ya2);
                isOverlap = testOverlap(xa1,ya1,xa2,ya2,xMin,yMin,xMax,yMax);
                // test overlap
                if (isOverlap){
                    switch (action->getKind()){
                        case actionURI:{
                            LinkURI* uri = (LinkURI*)action;
                            if (uri->isOk())
                            {
                                GString* dest = uri->getURI();
                                if (dest != NULL) {
                                    // PL: the uri here must be well formed to avoid not wellformed XML!
                                    xmlChar* name_uri = xmlURIEscape((const xmlChar*)dest->getCString());
                                    //xmlNewProp(node, (const xmlChar*)ATTR_URILINK,(const xmlChar*)dest->getCString());
                                    xmlNewProp(node, (const xmlChar*)ATTR_URILINK, name_uri);
                                    free(name_uri); // PL
                                    return;
                                }
                            }
                            break;
                        }
                        case actionGoTo:{
                            LinkGoTo* goto_link = (LinkGoTo*)action;
                            if (goto_link->isOk())
                            {
                                bool newlink = false;
                                LinkDest* link_dest = goto_link->getDest();
                                GString*  name_dest = goto_link->getNamedDest();
                                if (name_dest != NULL && myCat != NULL)
                                {
                                    link_dest = myCat->findDest(name_dest);
                                    newlink   = true;
                                }
                                if (link_dest != NULL && link_dest->isOk())
                                {
                                    // find the destination page number (counted from 1)
                                    int page;
                                    if (link_dest->isPageRef())
                                    {
                                        Ref pref = link_dest->getPageRef();
                                        page = myCat->findPage(pref.num, pref.gen);
                                    }
                                    else
                                        page = link_dest->getPageNum();

                                    // other data depend in the link type
                                    switch (link_dest->getKind())
                                    {
                                        case destXYZ:
                                        {
                                            // find the location on the destination page
                                            //if (link_dest->getChangeLeft() && link_dest->getChangeTop()){
                                            // TODO FH 25/01/2006 apply transform matrix of destination page, not current page
                                            double x,y;
                                            curstate->transform(link_dest->getLeft(), link_dest->getTop(), &x, &y);
                                            sprintf(tmp,"p-%d %g %g",page,x,y);
                                            xmlNewProp(node, (const xmlChar*)ATTR_GOTOLINK,(const xmlChar*)tmp);
//												printf("link %d %g %g\n",page,x,y);
                                            free(tmp); // PL
                                            return;
                                            //}
                                        }
                                            break;

                                            // link to the page, without a specific location. PDF Data Destruction has hit again!
                                        case destFit:  case destFitH: case destFitV: case destFitR:
                                        case destFitB: case destFitBH: case destFitBV:
                                            sprintf(tmp,"p-%d",page);
                                            xmlNewProp(node, (const xmlChar*)ATTR_GOTOLINK,(const xmlChar*)tmp);
                                            free(tmp); // PL
                                            return;
                                    }

                                    // must delete the link object if it comes from the catalog
                                    if (newlink)
                                        delete link_dest;
                                }
                            }
                            break;
//							//pagenum
//							sprintf(tmp, "p-%d ",
//													num]->getXPositionImage());
//							//zone x1,x1,h,w
//							//sprintf()
//							xmlNewProp(node, (const xmlChar*)ATTR_LINKREF,(const xmlChar*)tmp);
                        }
                    }//switch
                }
            } //action isOk
        } // link isOk
    }

}


GBool TextPage::testOverlap(double x11,double y11,double x12,double y12,double x21,double y21,double x22,double y22){
    return ( (min(x12,x22) >= max(x11,x21)) &&
             (min(y12,y22) >= max(y11,y21)) );
}
GBool TextPage::testAnnotatedText(double xMin,double yMin,double xMax,double yMax){
    Object objQuadPoints;
    Dict *dict;

    int nb = highlightedObject.size();
    for (int i=0; i<nb; i++) {
        dict = highlightedObject[i];
        // Get the localization (points series) of the annotation into the page
        if ((*dict).lookup("QuadPoints", &objQuadPoints)->isArray()){
            Object point;
            double xmax2=0;
            double ymax2=0;
            double xmin2=0;
            double ymin2=0;
            double x,y,x1,y1;
            for (int i = 0 ; i < objQuadPoints.arrayGetLength() ; ++i){
                objQuadPoints.arrayGet(i, &point);
                if (i%2==0){
                    //even = x
                    if (point.isReal()) {
                        x = point.getReal();
                    }
                }else{
                    //odd= y

                    if (point.isReal()) {
                        y = point.getReal();

                        curstate->transform(x,y,&x1,&y1);
//						printf("POINT : %g %g\t %g %g\n",x,y,x1,y1);
                        if (xmin2 ==0){xmin2=x1;}
                        else{xmin2=min(xmin2,x1);}
                        xmax2=max(xmax2,x1);

                        if (ymin2 ==0){ymin2=y1;}
                        else{ymin2=min(ymin2,y1);}
                        ymax2=max(ymax2,y1);
                    }
                }
                point.free();

            }
            if ( (min(xMax,xmax2) >= max(xMin,xmin2)) && (min(yMax,ymax2) >= max(yMin,ymin2)) ){
                return gTrue;
            }
        }
        objQuadPoints.free();

    }
    return gFalse;
}

GBool TextFontStyleInfo::cmp(TextFontStyleInfo *tsi) {
    if( ((fontName->cmp(tsi->getFontName())  == 0 )
            &&  (fontSize == tsi->getFontSize())
            && (fontColor->cmp(tsi->getFontColor())  == 0 )
            //&& (fontType == tsi->getFontType())
            //&& (fontStyle == tsi->getFontStyle())
    )
            )
        return gTrue;
    else return gFalse;
}

void TextPage::dump(GBool blocks, GBool fullFontName) {
    UnicodeMap *uMap;

    TextWord *word;
    TextFontStyleInfo *fontStyleInfo;
    GString *stringTemp;

    GString *id;
    char* tmp;

    tmp=(char*)malloc(10*sizeof(char));

    // For TEXT tag attributes
    double xMin = 0;
    double yMin = 0;
    double xMax = 0;
    double yMax = 0;
    double yMaxRot = 0;
    double yMinRot = 0;
    double xMaxRot = 0;
    double xMinRot = 0;
    int firstword= 1; // firstword of a TEXT tag

    // x and y for nodeline: min (xi) min(yi) whatever the text rotation is
    double minLineX =0;
    double minLineY = 0;

    xmlNodePtr node = NULL;
    xmlNodePtr nodeline = NULL;
    xmlNodePtr nodeblocks = NULL;
    xmlNodePtr nodeImageInline = NULL;

    GBool lineFinish= gFalse;
    GBool newBlock= gFalse;
    GBool endPage= gFalse;
    GBool lastBlockInserted = gFalse; // true if the last added block is inserted before an existing node, false is appended

    // Informations about the current line
    double lineX = 0;
    double lineYmin = 0;
    double lineWidth = 0;
    double lineHeight = 0;
    double lineFontSize = 0;

    // Informations about the previous line
    double linePreviousX = 0;
    double linePreviousYmin = 0;
    double linePreviousWidth = 0;
    double linePreviousHeight = 0;
    double linePreviousFontSize = 0;

    // Get the output encoding
    if (!(uMap = globalParams->getTextEncoding())) {
        return;
    }

    numText = 1;
    numBlock = 1;
    // Output the page in raw (content stream) order

    lineFontSize = 0;


    printSpace = xmlNewNode(NULL, (const xmlChar*)TAG_PRINTSPACE);
    printSpace->type = XML_ELEMENT_NODE;
    xmlAddChild(page, printSpace);

    nodeline = xmlNewNode(NULL, (const xmlChar*)TAG_TEXT);
    nodeline->type = XML_ELEMENT_NODE;

    if (blocks) {
        nodeblocks = xmlNewNode(NULL, (const xmlChar*)TAG_BLOCK);
        nodeblocks->type = XML_ELEMENT_NODE;

        // PL: block is added when finished
        //xmlAddChild(printSpace, nodeblocks);
        id = new GString("p");
        xmlNewProp(nodeblocks, (const xmlChar*)ATTR_ID,
                   (const xmlChar*)buildIdBlock(num, numBlock, id)->getCString());
        delete id;
        numBlock = numBlock + 1;
    } else {
        if (rawWords) {
            xmlAddChild(printSpace, nodeline);
        }
    }

    xMin= yMin = xMax = yMax =0;
    minLineX = 999999999;
    minLineY = 999999999;

    for (word = rawWords; word; word = word->next) {

        fontStyleInfo = new TextFontStyleInfo;

        lineFinish = gFalse;
        if (firstword) { // test useful?
            xMin = word->xMin;
            yMin = word->yMin;
            xMax = word->xMax;
            yMax = word->yMax;
            yMaxRot = word->yMax;
            yMinRot = word->yMin;
            xMaxRot = word->xMax;
            xMinRot = word->xMin;
            lineX = word->xMin;
            lineYmin = word->yMin;
            firstword = 0;
            lineFontSize = 0;
        }
        if (word->xMin < minLineX) {
            minLineX = word->xMin;
        } // for nodeline
        if (word->yMin < minLineY) {
            minLineY = word->yMin;
        } // for nodeline

        node = xmlNewNode(NULL, (const xmlChar*)TAG_TOKEN);

        node->type = XML_ELEMENT_NODE;

        id = new GString("p");
        xmlNewProp(node, (const xmlChar*)ATTR_SID, (const xmlChar*)buildSID(num, word->getIdx(), id)->getCString());
        delete id;

        id = new GString("p");
        xmlNewProp(node, (const xmlChar*)ATTR_ID, (const xmlChar*)buildIdToken(num, numToken, id)->getCString());
        delete id;
        numToken = numToken + 1;

        stringTemp = new GString();
        testLinkedText(node,word->xMin,word->yMin,word->xMax,word->yMax);
        if (testAnnotatedText(word->xMin,word->yMin,word->xMax,word->yMax)){
            xmlNewProp(node, (const xmlChar*)ATTR_HIGHLIGHT,(const xmlChar*)"yes");
        }
        dumpFragment(word->text, word->len, uMap, stringTemp);
        //printf("%s\n",stringTemp->getCString());

        xmlNewProp(node, (const xmlChar*)ATTR_TOKEN_CONTENT,
                   (const xmlChar*)stringTemp->getCString());
        delete stringTemp;

        if (word->fontSize > lineFontSize) {
            lineFontSize = word->fontSize;
        }

        // If option verbose is selected
        if (verbose) {
            addAttributsNodeVerbose(node, tmp, word);
        }
        if (word->getFontName()) {
            xmlChar* xcFontName;
            // If the font name normalization option is selected
            if (fullFontName) {
                //xmlNewProp(node, (const xmlChar*)ATTR_FONT_NAME,
                //(const xmlChar*)word->getFontName());
                //(const xmlChar*)"none1");
                xcFontName = (xmlChar*)word->getFontName();
            } else {
                xcFontName
                        = (xmlChar*)word->normalizeFontName(word->getFontName());
                //xmlNewProp(
                //		node,
                //			(const xmlChar*)ATTR_FONT_NAME,
                //(const xmlChar*)word->normalizeFontName(word->getFontName()));
                //OK (const xmlChar*)"none2");
            }
            //ugly code because I don't know how all these types...
            //convert to a Unicode*
            int size = xmlStrlen(xcFontName);
            Unicode* uncdFontName = (Unicode *)malloc((size+1)
                                                      * sizeof(Unicode));
            for (int i=0; i < size; i++) {
                uncdFontName[i] = (Unicode) xcFontName[i];
            }
            uncdFontName[size] = (Unicode)0;
            GString* gsFontName = new GString();
            dumpFragment(uncdFontName, size, uMap, gsFontName);
            xmlNewProp(node, (const xmlChar*)ATTR_FONT_NAME,
                       (const xmlChar*)gsFontName->getCString());
            fontStyleInfo->setFontName(gsFontName);
            //delete gsFontName;


        }

        addAttributsNode(node, word, xMax, yMax, yMinRot, yMaxRot, xMinRot,
                         xMaxRot, fontStyleInfo);
        addAttributTypeReadingOrder(node, tmp, word);

        /// annotations: higlighted, underline,

        double xxMin, xxMax, xxMinNext;
        double yyMin, yyMax, yyMinNext;

        // Rotation cases
        switch (word->rot) {
            case 0:
                xxMin = word->xMin;
                xxMax = word->xMax;
                yyMin = word->yMin;
                yyMax = word->yMax;
                if (word->next) {
                    xxMinNext = word->next->xMin;
                    yyMinNext = word->next->yMin;
                }
                break;

            case 3:
                xxMin = word->yMin;
                xxMax = word->yMax;
                yyMin = word->xMax;
                yyMax = word->xMin;
                if (word->next) {
                    xxMinNext = word->next->yMin;
                    yyMinNext = word->next->xMax;
                }
                break;

            case 2:
                xxMin = word->xMax;
                xxMax = word->xMin;
                yyMin = word->yMax;
                yyMax = word->yMin;
                if (word->next) {
                    xxMinNext = word->next->xMax;
                    yyMinNext = word->next->yMax;
                }
                break;

            case 1:
                xxMin = word->yMax;
                xxMax = word->yMin;
                yyMin = word->xMax;
                yyMax = word->xMin;
                if (word->next) {
                    xxMinNext = word->next->yMax;
                    yyMinNext = word->next->xMax;
                }
                break;
        }

        // Get the rotation into four axis
        int rotation = -1;
        if (word->rot==0 && word->angle==0) {
            rotation = 0;
        }
        if (word->rot==1 && word->angle==90) {
            rotation = 1;
        }
        if (word->rot==2 && word->angle==180) {
            rotation = 2;
        }
        if (word->rot==3 && word->angle==270) {
            rotation = 3;
        }

        // Add next images inline whithin the current line if the noImageInline option is not selected
        if (!parameters->getImageInline()) {
            if (indiceImage != -1) {
                int nb = listeImageInline.size();
                for (; indiceImage<nb; indiceImage++) {
                    if (idWORDBefore
                        == listeImageInline[indiceImage]->idWordBefore) {
                        nodeImageInline = xmlNewNode(NULL,
                                                     (const xmlChar*)TAG_TOKEN);
                        nodeImageInline->type = XML_ELEMENT_NODE;
                        id = new GString("p");
                        xmlNewProp(nodeImageInline, (const xmlChar*)ATTR_ID,
                                   (const xmlChar*)buildIdToken(num, numToken, id)->getCString());
                        delete id;
                        numToken = numToken + 1;
                        id = new GString("p");
                        xmlNewProp(
                                nodeImageInline,
                                (const xmlChar*)ATTR_SID,
                                (const xmlChar*)buildSID(num, listeImageInline[indiceImage]->getIdx(), id)->getCString());
                        delete id;
                        sprintf(
                                tmp,
                                ATTR_NUMFORMAT,
                                listeImageInline[indiceImage]->getXPositionImage());
                        xmlNewProp(nodeImageInline, (const xmlChar*)ATTR_X,
                                   (const xmlChar*)tmp);
                        sprintf(
                                tmp,
                                ATTR_NUMFORMAT,
                                listeImageInline[indiceImage]->getYPositionImage());
                        xmlNewProp(nodeImageInline, (const xmlChar*)ATTR_Y,
                                   (const xmlChar*)tmp);
                        sprintf(tmp, ATTR_NUMFORMAT,
                                listeImageInline[indiceImage]->getWidthImage());
                        xmlNewProp(nodeImageInline, (const xmlChar*)ATTR_WIDTH,
                                   (const xmlChar*)tmp);
                        sprintf(tmp, ATTR_NUMFORMAT,
                                listeImageInline[indiceImage]->getHeightImage());
                        xmlNewProp(nodeImageInline,
                                   (const xmlChar*)ATTR_HEIGHT,
                                   (const xmlChar*)tmp);
                        xmlNewProp(
                                nodeImageInline,
                                (const xmlChar*)ATTR_HREF,
                                (const xmlChar*)listeImageInline[indiceImage]->getHrefImage()->getCString());
                        xmlAddChild(nodeline, nodeImageInline);
                    }
                }
            }
        }

        // Add the attributes width and height to the node TEXT
        // The line is finished IF :

        // 		- there is no next word
        //		- and IF the rotation of current word is different of the rotation next word
        //		- and IF the difference between the base of current word and the yMin next word is superior to the maxSpacingWordsBetweenTwoLines
        //		-      or IF the difference between the base of current word and the base next word is superior to maxIntraLineDelta * lineFontSize
        //		- and IF the xMax current word ++ maxWordSpacing * lineFontSize is superior to the xMin next word.
        //		HD 24/07/09 ?? - or if the font size of the current word is far different from the next word
        if (word->next && (word->rot==word->next->rot) && (
                (
                        (fabs(word->base - word->next->baseYmin) < maxSpacingWordsBetweenTwoLines) ||
                        (fabs(word->next->base - word->base) < maxIntraLineDelta * min(lineFontSize,word->next->fontSize) )
                )
                && (word->next->xMin <= word->xMax + maxWordSpacing * lineFontSize)
        )
                ) {

            // IF - switch the rotation :
            //			base word and yMin word are inferior to yMin next word
            //			xMin word is superior to xMin next word
            //			xMax word is superior to xMin next word and the difference between the base of current word and the next word is superior to maxIntraLineDelta*lineFontSize
            //			xMin next word is superior to xMax word + maxWordSpacing * lineFontSize
            //THEN if one of these tests is true, the line is finish
            if (( (rotation==-1) ? ((word->base < word->next->yMin)
                                    && (word->yMin < word->next->yMin)) : (word->rot==0
                                                                           ||word->rot==1) ? ((word->base < yyMinNext) && (yyMin
                                                                                                                           < yyMinNext)) : ((word->base > yyMinNext) && (yyMin
                                                                                                                                                                         > yyMinNext)) ) || ( (rotation==-1) ? (word->next->xMin
                                                                                                                                                                                                                < word->xMin) : (word->rot==0) ? (xxMinNext < xxMin)
                                                                                                                                                                                                                                               : (word->rot==1 ? xxMinNext > xxMin
                                                                                                                                                                                                                                                               : (word->rot==2 ? xxMinNext > xxMin : xxMinNext
                                                                                                                                                                                                                                                                                                     < xxMin) ) )
                || ( (rotation==-1) ? (word->next->xMin<word->xMax)
                                      && (fabs(word->next->base-word->base)
                                          >maxIntraLineDelta*lineFontSize)
                                    : (word->rot==0||word->rot==3) ? ( (xxMinNext<xxMax)
                                                                       && (fabs(word->next->base-word->base)
                                                                           >maxIntraLineDelta*lineFontSize) )
                                                                   : ( (xxMinNext > xxMax)
                                                                       && (fabs(word->next->base
                                                                                -word->base)
                                                                           >maxIntraLineDelta*lineFontSize) ))
                || ( (rotation==-1) ? (word->next->xMin > word->xMax
                                                          + maxWordSpacing * lineFontSize) : (word->rot==0
                                                                                              ||word->rot==3) ? (xxMinNext > xxMax
                                                                                                                             + maxWordSpacing * lineFontSize) : (xxMinNext
                                                                                                                                                                 < xxMax - maxWordSpacing * lineFontSize))) {

                xmlAddChild(nodeline, node);
                double arr;
                if (word->rot==2) {
                    arr = fabs(xMaxRot-xMinRot);
                    sprintf(tmp, ATTR_NUMFORMAT, arr);
                    xmlNewProp(nodeline, (const xmlChar*)ATTR_WIDTH,
                               (const xmlChar*)tmp);
                    lineWidth = arr;
                } else {
                    arr = xMax-xMin;
                    sprintf(tmp, ATTR_NUMFORMAT, arr);
                    xmlNewProp(nodeline, (const xmlChar*)ATTR_WIDTH,
                               (const xmlChar*)tmp);
                    lineWidth = arr;
                }

                if (word->rot==0||word->rot==2) {
                    arr = yMax-yMin;
                    sprintf(tmp, ATTR_NUMFORMAT, arr);
                    xmlNewProp(nodeline, (const xmlChar*)ATTR_HEIGHT,
                               (const xmlChar*)tmp);
                    lineHeight = arr;
                }

                if (word->rot==1||word->rot==3) {
                    arr = yMaxRot-yMinRot;
                    sprintf(tmp, ATTR_NUMFORMAT, arr);
                    xmlNewProp(nodeline, (const xmlChar*)ATTR_HEIGHT,
                               (const xmlChar*)tmp);
                    lineHeight = arr;
                }

                sprintf(tmp, ATTR_NUMFORMAT, minLineX);
                xmlNewProp(nodeline, (const xmlChar*)ATTR_X,
                           (const xmlChar*)tmp);
                sprintf(tmp, ATTR_NUMFORMAT, minLineY);
                xmlNewProp(nodeline, (const xmlChar*)ATTR_Y,
                           (const xmlChar*)tmp);

                // Add the ID attribute for the TEXT tag
                id = new GString("p");
                xmlNewProp(nodeline, (const xmlChar*)ATTR_ID,
                           (const xmlChar*)buildIdText(num, numText, id)->getCString());
                delete id;
                numText = numText + 1;

                //testLinkedText(nodeline,minLineX,minLineY,minLineX+lineWidth,minLineY+lineHeight);
//				if (testAnnotatedText(minLineX,minLineY,minLineX+lineWidth,minLineY+lineHeight)){
//					xmlNewProp(nodeline, (const xmlChar*)ATTR_HIGHLIGHT,(const xmlChar*)"yes");
//				}
                if (word->fontSize > lineFontSize) {
                    lineFontSize = word->fontSize;
                }

                // Include a TOKEN tag for the image inline if it exists
                if (!parameters->getImageInline()) {
                    addImageInlineNode(nodeline, nodeImageInline, tmp, word);
                }

                if (word->next) {
                    firstword = 1;
                    if (blocks) {
                        lineFinish = gTrue;
                    } else {
                        // new line
                        nodeline = xmlNewNode(NULL, (const xmlChar*)TAG_TEXT);
                        nodeline->type = XML_ELEMENT_NODE;
                        xmlAddChild(printSpace, nodeline);
                        minLineY = 999999999;
                        minLineX = 999999999;
                    }
                } else {
                    endPage = gTrue;
                }
                xMin = yMin = xMax = yMax = yMinRot = yMaxRot = xMaxRot
                        = xMinRot = 0;
            } else {
                xmlAddChild(nodeline, node);

                // Include a TOKEN tag for the image inline if it exists
                if (!parameters->getImageInline()) {
                    addImageInlineNode(nodeline, nodeImageInline, tmp, word);
                }
            }
        } else {
            // node to be added to nodeline
            double arr;
            if (word->rot==2) {
                arr = fabs(xMaxRot-xMinRot);
                sprintf(tmp, ATTR_NUMFORMAT, arr);
                xmlNewProp(nodeline, (const xmlChar*)ATTR_WIDTH,
                           (const xmlChar*)tmp);
                lineWidth = arr;
            } else {
                arr = xMax-xMin;
                sprintf(tmp, ATTR_NUMFORMAT, arr);
                xmlNewProp(nodeline, (const xmlChar*)ATTR_WIDTH,
                           (const xmlChar*)tmp);
                lineWidth = arr;
            }

            if (word->rot==0||word->rot==2) {
                arr = yMax-yMin;
                sprintf(tmp, ATTR_NUMFORMAT, arr);
                xmlNewProp(nodeline, (const xmlChar*)ATTR_HEIGHT,
                           (const xmlChar*)tmp);
                lineHeight = arr;
            }

            if (word->rot==1||word->rot==3) {
                arr = yMaxRot-yMinRot;
                sprintf(tmp, ATTR_NUMFORMAT, arr);
                xmlNewProp(nodeline, (const xmlChar*)ATTR_HEIGHT,
                           (const xmlChar*)tmp);
                lineHeight = arr;
            }

            xmlAddChild(nodeline, node);

            // Include a TOKEN tag for the image inline if it exists
            if (!parameters->getImageInline()) {
                addImageInlineNode(nodeline, nodeImageInline, tmp, word);
            }

            // Add the ID attribute for the TEXT tag
            id = new GString("p");
            xmlNewProp(nodeline, (const xmlChar*)ATTR_ID,
                       (const xmlChar*)buildIdText(num, numText, id)->getCString());
            delete id;
            numText = numText + 1;

            //testLinkedText(nodeline,minLineX,minLineY,minLineX+lineWidth,minLineY+lineHeight);
//			if (testAnnotatedText(minLineX,minLineY,minLineX+lineWidth,minLineY+lineHeight)){
//				xmlNewProp(nodeline, (const xmlChar*)ATTR_HIGHLIGHT,(const xmlChar*)"yes");
//			}

            sprintf(tmp, ATTR_NUMFORMAT, minLineX);
            xmlNewProp(nodeline, (const xmlChar*)ATTR_X, (const xmlChar*)tmp);
            sprintf(tmp, ATTR_NUMFORMAT, minLineY);
            xmlNewProp(nodeline, (const xmlChar*)ATTR_Y, (const xmlChar*)tmp);

            if (word->fontSize > lineFontSize) {
                lineFontSize = word->fontSize;
            }

            firstword = 1;
            xMin = yMin = xMax = yMax = yMinRot = yMaxRot = xMaxRot = xMinRot
                    = 0;
            minLineY = 99999999;
            minLineX = 99999999;

            if (word->next) {
                if (blocks) {
                    lineFinish = gTrue;
                } else {
                    nodeline = xmlNewNode(NULL, (const xmlChar*)TAG_TEXT);
                    nodeline->type = XML_ELEMENT_NODE;
                    xmlAddChild(printSpace, nodeline);
                }
            } else {
                endPage = gTrue;
            }
        }

        // IF block option is selected
        // IF it's the end of line or the end of page
        if ( (blocks && lineFinish) || (blocks && endPage)) {
            // IF it's the first line
            if (linePreviousX == 0) {
                if (word->next) {
                    if (word->next->xMin > (lineX + lineWidth)
                                           + (maxColSpacing * lineFontSize)) {
                        newBlock = gTrue;
                    }
                }

                // PL: check if X and Y coordinates of the current block are present
                // and set them if it's not the case
                if (nodeblocks != NULL) {
                    // get block X and Y
                    xmlChar *attrValue = xmlGetProp(nodeblocks, (const xmlChar*)ATTR_X);
                    if (attrValue == NULL) {
                        // set the X attribute
                        if (lineX != 0) {
                            sprintf(tmp, ATTR_NUMFORMAT, lineX);
                            xmlNewProp(nodeblocks, (const xmlChar*)ATTR_X, (const xmlChar*)tmp);
                        }
                    } else
                        xmlFree(attrValue);

                    attrValue = xmlGetProp(nodeblocks, (const xmlChar*)ATTR_Y);
                    if (attrValue == NULL) {
                        // set the Y attribute
                        if (lineYmin != 0) {
                            sprintf(tmp, ATTR_NUMFORMAT, lineYmin);
                            xmlNewProp(nodeblocks, (const xmlChar*)ATTR_Y, (const xmlChar*)tmp);
                        }
                    } else
                        xmlFree(attrValue);
                }

                xmlAddChild(nodeblocks, nodeline);
            } else {
                if (newBlock) {
                    // PL: previous block height and width
                    if (nodeblocks != NULL) {
                        // get block X and Y
                        double blockX = 0;
                        double blockY = 0;
                        xmlChar *attrValue = xmlGetProp(nodeblocks, (const xmlChar*)ATTR_X);
                        if (attrValue != NULL) {
                            blockX = atof((const char*)attrValue);
                            xmlFree(attrValue);
                        }
                        attrValue = xmlGetProp(nodeblocks, (const xmlChar*)ATTR_Y);
                        if (attrValue != NULL) {
                            blockY = atof((const char*)attrValue);
                            xmlFree(attrValue);
                        }

                        double blockWidth = std::abs((linePreviousX + linePreviousWidth) - blockX);
                        double blockHeight = std::abs((linePreviousYmin + linePreviousHeight) - blockY);

                        sprintf(tmp, ATTR_NUMFORMAT, blockHeight);
                        xmlNewProp(nodeblocks, (const xmlChar*)ATTR_HEIGHT, (const xmlChar*)tmp);
                        sprintf(tmp, ATTR_NUMFORMAT, blockWidth);
                        xmlNewProp(nodeblocks, (const xmlChar*)ATTR_WIDTH, (const xmlChar*)tmp);

                        // adding previous block to the page element
                        if (readingOrder)
                            lastBlockInserted = addBlockChildReadingOrder(nodeblocks, lastBlockInserted);
                        else
                            xmlAddChild(printSpace, nodeblocks);
                    }

                    nodeblocks = xmlNewNode(NULL, (const xmlChar*)TAG_BLOCK);
                    nodeblocks->type = XML_ELEMENT_NODE;
                    // PL: block is added when finished
                    //xmlAddChild(printSpace, nodeblocks);
                    id = new GString("p");
                    xmlNewProp(nodeblocks, (const xmlChar*)ATTR_ID,
                               (const xmlChar*)buildIdBlock(num, numBlock, id)->getCString());
                    delete id;

                    // PL: new block X and Y
                    if (lineX != 0) {
                        sprintf(tmp, ATTR_NUMFORMAT, lineX);
                        xmlNewProp(nodeblocks, (const xmlChar*)ATTR_X, (const xmlChar*)tmp);
                    }
                    if (lineYmin != 0) {
                        sprintf(tmp, ATTR_NUMFORMAT, lineYmin);
                        xmlNewProp(nodeblocks, (const xmlChar*)ATTR_Y, (const xmlChar*)tmp);
                    }
                    numBlock = numBlock + 1;
                    xmlAddChild(nodeblocks, nodeline);
                    newBlock = gFalse;
                } else {
                    if (((lineYmin + lineHeight) >= linePreviousYmin)
                        && (fabs(lineFontSize - linePreviousFontSize)
                            < lineFontSize * maxBlockFontSizeDelta1)
                        && ((lineYmin - linePreviousYmin)
                            < (linePreviousFontSize
                               * maxLineSpacingDelta))) {
                        if (endPage) {
                            // PL: check if the width and the height of the current block are present
                            // and set them if it's not the case
                            if (nodeblocks != NULL) {
                                // check width and height
                                xmlChar *attrValue = xmlGetProp(nodeblocks, (const xmlChar*)ATTR_HEIGHT);
                                if (attrValue == NULL) {
                                    // set the height attribute
                                    double blockY = 0;
                                    attrValue = xmlGetProp(nodeblocks, (const xmlChar*)ATTR_Y);
                                    if (attrValue != NULL) {
                                        blockY = atof((const char*)attrValue);
                                        xmlFree(attrValue);
                                    }
                                    double blockHeight = std::abs((linePreviousYmin + linePreviousHeight) - blockY);
                                    sprintf(tmp, ATTR_NUMFORMAT, blockHeight);
                                    xmlNewProp(nodeblocks, (const xmlChar*)ATTR_HEIGHT, (const xmlChar*)tmp);
                                } else
                                    xmlFree(attrValue);

                                attrValue = xmlGetProp(nodeblocks, (const xmlChar*)ATTR_WIDTH);
                                if (attrValue == NULL) {
                                    // set the width attribute
                                    double blockX = 0;
                                    xmlChar *attrValue = xmlGetProp(nodeblocks, (const xmlChar*)ATTR_X);
                                    if (attrValue != NULL) {
                                        blockX = atof((const char*)attrValue);
                                        xmlFree(attrValue);
                                    }
                                    double blockWidth = std::abs((linePreviousX + linePreviousWidth) - blockX);
                                    sprintf(tmp, ATTR_NUMFORMAT, blockWidth);
                                    xmlNewProp(nodeblocks, (const xmlChar*)ATTR_WIDTH, (const xmlChar*)tmp);
                                } else
                                    xmlFree(attrValue);
                            }
                        }

                        xmlAddChild(nodeblocks, nodeline);
                    } else {
                        // PL: previous block height and width
                        if (nodeblocks != NULL) {
                            // get block X and Y
                            double blockX = 0;
                            double blockY = 0;
                            xmlChar *attrValue = xmlGetProp(nodeblocks, (const xmlChar*)ATTR_X);
                            if (attrValue != NULL) {
                                blockX = atof((const char*)attrValue);
                                xmlFree(attrValue);
                            }
                            attrValue = xmlGetProp(nodeblocks, (const xmlChar*)ATTR_Y);
                            if (attrValue != NULL) {
                                blockY = atof((const char*)attrValue);
                                xmlFree(attrValue);
                            }

                            double blockWidth = std::abs((linePreviousX + linePreviousWidth) - blockX);
                            double blockHeight = std::abs((linePreviousYmin + linePreviousHeight) - blockY);

                            sprintf(tmp, ATTR_NUMFORMAT, blockHeight);
                            xmlNewProp(nodeblocks, (const xmlChar*)ATTR_HEIGHT, (const xmlChar*)tmp);
                            sprintf(tmp, ATTR_NUMFORMAT, blockWidth);
                            xmlNewProp(nodeblocks, (const xmlChar*)ATTR_WIDTH, (const xmlChar*)tmp);

                            // adding previous block to the page element
                            if (readingOrder)
                                lastBlockInserted = addBlockChildReadingOrder(nodeblocks, lastBlockInserted);
                            else
                                xmlAddChild(printSpace, nodeblocks);
                        }

                        nodeblocks = xmlNewNode(NULL, (const xmlChar*)TAG_BLOCK);
                        nodeblocks->type = XML_ELEMENT_NODE;
                        // PL: block is added when finished
                        //xmlAddChild(printSpace, nodeblocks);
                        id = new GString("p");
                        xmlNewProp(nodeblocks, (const xmlChar*)ATTR_ID,
                                   (const xmlChar*)buildIdBlock(num, numBlock, id)->getCString());
                        delete id;

                        // PL: new block X and Y
                        if (lineX != 0) {
                            sprintf(tmp, ATTR_NUMFORMAT, lineX);
                            xmlNewProp(nodeblocks, (const xmlChar*)ATTR_X, (const xmlChar*)tmp);
                        }
                        if (lineYmin != 0) {
                            sprintf(tmp, ATTR_NUMFORMAT, lineYmin);
                            xmlNewProp(nodeblocks, (const xmlChar*)ATTR_Y, (const xmlChar*)tmp);
                        }

                        numBlock = numBlock + 1;
                        xmlAddChild(nodeblocks, nodeline);
                    }
                }
            }
            if (endPage) {
                endPage = gFalse;

                if (nodeblocks) {
                    if (readingOrder)
                        addBlockChildReadingOrder(nodeblocks, lastBlockInserted);
                    else
                        xmlAddChild(printSpace, nodeblocks);
                    lastBlockInserted = gFalse;
                }
            }

            // We save informations about the future previous line
            linePreviousX = lineX;
            linePreviousYmin = lineYmin;
            linePreviousWidth = lineWidth;
            linePreviousHeight = lineHeight;
            linePreviousFontSize = lineFontSize;
            minLineY = 99999999;
            minLineX = 99999999;
            nodeline = xmlNewNode(NULL, (const xmlChar*)TAG_TEXT);
            nodeline->type = XML_ELEMENT_NODE;
        }

        if(xmlChildElementCount(nodeline) > 0 && word->next)
        {

            xmlNodePtr spacingNode = xmlNewNode(NULL, (const xmlChar*)TAG_SPACING);
            spacingNode->type = XML_ELEMENT_NODE;
            sprintf(tmp, ATTR_NUMFORMAT, (word->next->xMin - word->xMax));
            xmlNewProp(spacingNode, (const xmlChar*)ATTR_WIDTH,
                       (const xmlChar*)tmp);
            sprintf(tmp, ATTR_NUMFORMAT, (word->yMin));
            xmlNewProp(spacingNode, (const xmlChar*)ATTR_Y,
                       (const xmlChar*)tmp);
            sprintf(tmp, ATTR_NUMFORMAT, (word->xMax));
            xmlNewProp(spacingNode, (const xmlChar*)ATTR_X,
                       (const xmlChar*)tmp);
            
            xmlAddChild(nodeline, spacingNode);

        }

    } // end FOR

    free(tmp);
    delete word;
    uMap->decRefCnt();
}

// PL: Insert a block in the page's block list according to the reading order
GBool TextPage::addBlockChildReadingOrder(xmlNodePtr nodeblock, GBool lastInserted) {

    // if Y_pos of the block to be inserted is less than Y_pos of the existing block
    // (i.e. block is located above)
    // and, in case of vertical overlap,
    //		X_pos + width of the block to be inserted is less than X_pos of this existing block
    // (i.e. block is on the left and the surfaces of the block are not overlaping -
    // 2 columns case)
    // then the block order is before the existing block

    xmlNode *cur_node = NULL;
    GBool notInserted = gTrue;
    // we get the first child of the current page node
    unsigned long nbChildren = xmlChildElementCount(printSpace);
    if (nbChildren > 0) {
        xmlNodePtr previousPageItem = NULL;

        // coordinates of the block to be inserted
        double blockX = 0;
        xmlChar *attrValue = xmlGetProp(nodeblock, (const xmlChar*)ATTR_X);
        if (attrValue != NULL) {
            blockX = atof((const char*)attrValue);
            xmlFree(attrValue);
        }

        double blockY = 0;
        attrValue = xmlGetProp(nodeblock, (const xmlChar*)ATTR_Y);
        if (attrValue != NULL) {
            blockY = atof((const char*)attrValue);
            xmlFree(attrValue);
        }

        double blockHeight = 0;
        attrValue = xmlGetProp(nodeblock, (const xmlChar*)ATTR_HEIGHT);
        if (attrValue != NULL) {
            blockHeight = atof((const char*)attrValue);
            xmlFree(attrValue);
        }

        double blockWidth = 0;
        attrValue = xmlGetProp(nodeblock, (const xmlChar*)ATTR_WIDTH);
        if (attrValue != NULL) {
            // set the width attribute
            blockWidth = atof((const char*)attrValue);
            xmlFree(attrValue);
        }

//cout << "to be inserted: " << nodeblock->name << ", X: " << blockX << ", Y: " << blockY << ", H: " << blockHeight << ", W: " << blockWidth << endl;

        xmlNodePtr firstPageItem = xmlFirstElementChild(printSpace);
        // we get all the block nodes in the XML tree corresponding to the page
        for (cur_node = firstPageItem; cur_node && notInserted; cur_node = cur_node->next) {
            if ( (cur_node->type == XML_ELEMENT_NODE) && (strcmp((const char*)cur_node->name, TAG_BLOCK) ==0) ) {
                //cout << "node type: Element, name: " << cur_node->name << endl;

                double currentY = 0;
                attrValue = xmlGetProp(cur_node, (const xmlChar*)ATTR_Y);
                if (attrValue != NULL) {
                    currentY = atof((const char*)attrValue);
                    xmlFree(attrValue);
                }

                if (currentY < blockY)
                    continue;

                double currentX = 0;
                attrValue = xmlGetProp(cur_node, (const xmlChar*)ATTR_X);
                if (attrValue != NULL) {
                    currentX = atof((const char*)attrValue);
                    xmlFree(attrValue);
                }

                double currentWidth = 0;
                attrValue = xmlGetProp(cur_node, (const xmlChar*)ATTR_WIDTH);
                if (attrValue != NULL) {
                    currentWidth = atof((const char*)attrValue);
                    xmlFree(attrValue);
                }

                if (blockY < currentY) {
                    if (blockY + blockHeight < currentY) {
                        // we don't have any vertical overlap
                        // check the X-pos, the block cannot be on the right of the current block
                        if ( (blockX < currentX + currentWidth) ||
                             ( (blockX + blockWidth > currentX + currentWidth) && (blockX + blockWidth > currentX) ) ||
                             lastInserted) {
                            // we can insert the block before the current block
                            xmlNodePtr result = xmlAddPrevSibling(cur_node, nodeblock);
                            notInserted = false;
                        }
                    }

                    // we have vertical overlap, check position on X axis

                    /*double currentHeight = 0;
					attrValue = xmlGetProp(cur_node, (const xmlChar*)ATTR_HEIGHT);
					if (attrValue != NULL) {
						currentHeight = atof((const char*)attrValue);
						xmlFree(attrValue);
					}*/
                }
                /*if (notInserted && (blockX + blockWidth < currentX)) {
					// does not work for multi column sections one after the other
					xmlNodePtr result = xmlAddPrevSibling(cur_node, nodeblock);
					notInserted = false;
				}*/
            }
        }
    }

    if (notInserted) {
        xmlAddChild(printSpace, nodeblock);
//cout << "append" << endl;
        return gFalse;
    }
    else {
//cout << "prev inserted" << endl;
        return gTrue;
    }
}

void TextPage::addImageInlineNode(xmlNodePtr nodeline,
                                  xmlNodePtr nodeImageInline, char* tmp, TextWord *word) {
    indiceImage = -1;
    idWORDBefore = -1;
    GBool first= gTrue;
    int nb = listeImageInline.size();
    for (int i=0; i<nb; i++) {
        if (word->idWord == listeImageInline[i]->idWordBefore) {
            if (listeImageInline[i]->getXPositionImage()>word->xMax
                && listeImageInline[i]->getYPositionImage()<= word->yMax) {
                nodeImageInline = xmlNewNode(NULL, (const xmlChar*)TAG_TOKEN);
                nodeImageInline->type = XML_ELEMENT_NODE;
                GString *id;
                id = new GString("p");
                xmlNewProp(nodeImageInline, (const xmlChar*)ATTR_ID,
                           (const xmlChar*)buildIdToken(num, numToken, id)->getCString());
                delete id;
                numToken = numToken + 1;
                id = new GString("p");
                xmlNewProp(nodeImageInline, (const xmlChar*)ATTR_SID,
                           (const xmlChar*)buildSID(num, listeImageInline[i]->getIdx(), id)->getCString());
                delete id;
                sprintf(tmp, ATTR_NUMFORMAT,
                        listeImageInline[i]->getXPositionImage());
                xmlNewProp(nodeImageInline, (const xmlChar*)ATTR_X,
                           (const xmlChar*)tmp);
                sprintf(tmp, ATTR_NUMFORMAT,
                        listeImageInline[i]->getYPositionImage());
                xmlNewProp(nodeImageInline, (const xmlChar*)ATTR_Y,
                           (const xmlChar*)tmp);
                sprintf(tmp, ATTR_NUMFORMAT,
                        listeImageInline[i]->getWidthImage());
                xmlNewProp(nodeImageInline, (const xmlChar*)ATTR_WIDTH,
                           (const xmlChar*)tmp);
                sprintf(tmp, ATTR_NUMFORMAT,
                        listeImageInline[i]->getHeightImage());
                xmlNewProp(nodeImageInline, (const xmlChar*)ATTR_HEIGHT,
                           (const xmlChar*)tmp);
                xmlNewProp(nodeImageInline, (const xmlChar*)ATTR_HREF,
                           (const xmlChar*)listeImageInline[i]->getHrefImage()->getCString());
                xmlAddChild(nodeline, nodeImageInline);
                idImageInline = listeImageInline[i]->getIdImageCurrent();
            }
            int j = i;
            for (; j<nb; j++) {
                if (word->idWord == listeImageInline[j]->idWordBefore) {
                    if (listeImageInline[j]->getXPositionImage()>word->xMax
                        && listeImageInline[j]->getYPositionImage()
                           <= word->yMax) {
                        if (idImageInline
                            != listeImageInline[j]->getIdImageCurrent()) {
                            nodeImageInline = xmlNewNode(NULL,
                                                         (const xmlChar*)TAG_TOKEN);
                            nodeImageInline->type = XML_ELEMENT_NODE;
                            GString *id;
                            id = new GString("p");
                            xmlNewProp(nodeImageInline,
                                       (const xmlChar*)ATTR_ID,
                                       (const xmlChar*)buildIdToken(num, numToken, id)->getCString());
                            delete id;
                            numToken = numToken + 1;
                            id = new GString("p");
                            xmlNewProp(nodeImageInline,
                                       (const xmlChar*)ATTR_SID,
                                       (const xmlChar*)buildSID(num, listeImageInline[j]->getIdx(), id)->getCString());
                            delete id;
                            sprintf(tmp, ATTR_NUMFORMAT,
                                    listeImageInline[j]->getXPositionImage());
                            xmlNewProp(nodeImageInline, (const xmlChar*)ATTR_X,
                                       (const xmlChar*)tmp);
                            sprintf(tmp, ATTR_NUMFORMAT,
                                    listeImageInline[j]->getYPositionImage());
                            xmlNewProp(nodeImageInline, (const xmlChar*)ATTR_Y,
                                       (const xmlChar*)tmp);
                            sprintf(tmp, ATTR_NUMFORMAT,
                                    listeImageInline[j]->getWidthImage());
                            xmlNewProp(nodeImageInline,
                                       (const xmlChar*)ATTR_WIDTH,
                                       (const xmlChar*)tmp);
                            sprintf(tmp, ATTR_NUMFORMAT,
                                    listeImageInline[j]->getHeightImage());
                            xmlNewProp(nodeImageInline,
                                       (const xmlChar*)ATTR_HEIGHT,
                                       (const xmlChar*)tmp);
                            xmlNewProp(
                                    nodeImageInline,
                                    (const xmlChar*)ATTR_HREF,
                                    (const xmlChar*)listeImageInline[j]->getHrefImage()->getCString());
                            xmlAddChild(nodeline, nodeImageInline);
                            idImageInline
                                    = listeImageInline[j]->getIdImageCurrent();
                        }
                    } else {
                        if (first) {
                            indiceImage = j;
                            idWORDBefore = word->idWord;
                            first = gFalse;
                        }
                    }
                }
            }
            break;
        }
    }
}

GString* TextPage::buildIdImage(int pageNum, int imageNum, GString *id) {
    char* tmp=(char*)malloc(10*sizeof(char));
    sprintf(tmp, "%d", pageNum);
    id->append(tmp);
    id->append("_i");
    sprintf(tmp, "%d", imageNum);
    id->append(tmp);
    free(tmp);
    return id;
}

GString* TextPage::buildSID(int pageNum, int sid, GString *id) {
    char* tmp=(char*)malloc(10*sizeof(char));
    sprintf(tmp, "%d", pageNum);
    id->append(tmp);
    id->append("_s");
    sprintf(tmp, "%d", sid);
    id->append(tmp);
    free(tmp);
    return id;
}
GString* TextPage::buildIdText(int pageNum, int textNum, GString *id) {
    char* tmp=(char*)malloc(10*sizeof(char));
    sprintf(tmp, "%d", pageNum);
    id->append(tmp);
    id->append("_t");
    sprintf(tmp, "%d", textNum);
    id->append(tmp);
    free(tmp);
    return id;
}

GString* TextPage::buildIdToken(int pageNum, int tokenNum, GString *id) {
    char* tmp=(char*)malloc(10*sizeof(char));
    sprintf(tmp, "%d", pageNum);
    id->append(tmp);
    id->append("_w");
    sprintf(tmp, "%d", tokenNum);
    id->append(tmp);
    free(tmp);
    return id;
}

GString* TextPage::buildIdBlock(int pageNum, int blockNum, GString *id) {
    char* tmp=(char*)malloc(10*sizeof(char));
    sprintf(tmp, "%d", pageNum);
    id->append(tmp);
    id->append("_b");
    sprintf(tmp, "%d", blockNum);
    id->append(tmp);
    free(tmp);
    return id;
}

GString* TextPage::buildIdClipZone(int pageNum, int clipZoneNum, GString *id) {
    char* tmp=(char*)malloc(10*sizeof(char));
    sprintf(tmp, "%d", pageNum);
    id->append(tmp);
    id->append("_c");
    sprintf(tmp, "%d", clipZoneNum);
    id->append(tmp);
    free(tmp);
    return id;
}

int TextPage::dumpFragment(Unicode *text, int len, UnicodeMap *uMap, GString *s) {
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
        if (primaryLR) {

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

void TextPage::saveState(GfxState *state) {
    idStack.push(idCur);
}

void TextPage::restoreState(GfxState *state) {

    idCur = idStack.top();
    idStack.pop();
}

void TextPage::doPathForClip(GfxPath *path, GfxState *state,
                             xmlNodePtr currentNode) {
    char * tmp;
    tmp = (char*)malloc(500*sizeof(char));

    // Increment the absolute object index
    idx++;
    xmlNodePtr groupNode = NULL;

    // GROUP tag
    groupNode = xmlNewNode(NULL, (const xmlChar*)TAG_GROUP);
    xmlAddChild(currentNode, groupNode);

    GString *id;
    id = new GString("p");
    xmlNewProp(groupNode, (const xmlChar*)ATTR_SID, (const xmlChar*)buildSID(num, getIdx(), id)->getCString());
    delete id;

    createPath(path, state, groupNode);
    free(tmp);
}

void TextPage::doPath(GfxPath *path, GfxState *state, GString* gattributes) {

    // Increment the absolute object index
    idx++;
    //printf("path %d\n",idx);
//	if (idx>10000){return;}

    xmlNodePtr groupNode = NULL;

    // GROUP tag
    groupNode = xmlNewNode(NULL, (const xmlChar*)TAG_GROUP);
    xmlAddChild(vecroot, groupNode);

    GString *id;
    id = new GString("p");
    xmlNewProp(groupNode, (const xmlChar*)ATTR_SID, (const xmlChar*)buildSID(num, getIdx(), id)->getCString());
    delete id;

    xmlNewProp(groupNode, (const xmlChar*)ATTR_STYLE,
               (const xmlChar*)gattributes->getCString());

    id = new GString("p");
    xmlNewProp(groupNode, (const xmlChar*)ATTR_CLIPZONE,
               (const xmlChar*)buildIdClipZone(num, idCur, id)->getCString());
    delete id;
    createPath(path, state, groupNode);
}

void TextPage::createPath(GfxPath *path, GfxState *state, xmlNodePtr groupNode) {
    GfxSubpath *subpath;
    double x0, y0, x1, y1, x2, y2, x3, y3;
    int n, m, i, j;
    double a, b;
    char *tmp;
    tmp = (char*)malloc(500*sizeof(char));

    xmlNodePtr pathnode = NULL;

    n = path->getNumSubpaths();
    for (i = 0; i < n; ++i) {
        subpath = path->getSubpath(i);
        m = subpath->getNumPoints();
        x0 = subpath->getX(0);
        y0 = subpath->getY(0);
        state->transform(x0, y0, &a, &b);
        x0 = a;
        y0 = b;

        // M tag : moveto
        pathnode = xmlNewNode(NULL, (const xmlChar*)TAG_M);
        sprintf(tmp, "%g", x0);
        xmlNewProp(pathnode, (const xmlChar*)ATTR_X, (const xmlChar*)tmp);
        sprintf(tmp, "%g", y0);
        xmlNewProp(pathnode, (const xmlChar*)ATTR_Y, (const xmlChar*)tmp);
        xmlAddChild(groupNode, pathnode);

        j = 1;
        while (j < m) {
            if (subpath->getCurve(j)) {
                x1 = subpath->getX(j);
                y1 = subpath->getY(j);
                x2 = subpath->getX(j+1);
                y2 = subpath->getY(j+1);
                x3 = subpath->getX(j+2);
                y3 = subpath->getY(j+2);
                state->transform(x1, y1, &a, &b);
                x1 = a;
                y1=b;
                state->transform(x2, y2, &a, &b);
                x2 = a;
                y2=b;
                state->transform(x3, y3, &a, &b);
                x3 = a;
                y3=b;

                // C tag  : curveto
                pathnode=xmlNewNode(NULL, (const xmlChar*)TAG_C);
                sprintf(tmp, "%g", x1);
                xmlNewProp(pathnode, (const xmlChar*)ATTR_X1,
                           (const xmlChar*)tmp);
                sprintf(tmp, "%g", y1);
                xmlNewProp(pathnode, (const xmlChar*)ATTR_Y1,
                           (const xmlChar*)tmp);
                sprintf(tmp, "%g", x2);
                xmlNewProp(pathnode, (const xmlChar*)ATTR_X2,
                           (const xmlChar*)tmp);
                sprintf(tmp, "%g", y2);
                xmlNewProp(pathnode, (const xmlChar*)ATTR_Y2,
                           (const xmlChar*)tmp);
                sprintf(tmp, "%g", x3);
                xmlNewProp(pathnode, (const xmlChar*)ATTR_X3,
                           (const xmlChar*)tmp);
                sprintf(tmp, "%g", y3);
                xmlNewProp(pathnode, (const xmlChar*)ATTR_Y3,
                           (const xmlChar*)tmp);
                xmlAddChild(groupNode, pathnode);

                j += 3;
            } else {
                x1 = subpath->getX(j);
                y1 = subpath->getY(j);
                state->transform(x1, y1, &a, &b);
                x1 = a;
                y1=b;

                // L tag : lineto
                pathnode=xmlNewNode(NULL, (const xmlChar*)TAG_L);
                sprintf(tmp, "%g", x1);
                xmlNewProp(pathnode, (const xmlChar*)ATTR_X,
                           (const xmlChar*)tmp);
                sprintf(tmp, "%g", y1);
                xmlNewProp(pathnode, (const xmlChar*)ATTR_Y,
                           (const xmlChar*)tmp);
                xmlAddChild(groupNode, pathnode);

                ++j;
            }
        }

        if (subpath->isClosed()) {
            if (!xmlHasProp(groupNode, (const xmlChar*)ATTR_CLOSED)) {
                xmlNewProp(groupNode, (const xmlChar*)ATTR_CLOSED,
                           (const xmlChar*)sTRUE);
            }
        }
    }
    free(tmp);
}

void TextPage::clip(GfxState *state) {
    idClip++;
    idCur = idClip;

    xmlNodePtr gnode = NULL;
    char tmp[100];
    double xMin = 0;
    double yMin = 0;
    double xMax = 0;
    double yMax = 0;

    // Increment the absolute object index
    idx++;
    //printf("clip %d\n",idx);
//	if (idx>10000){
//		return;
//	}

    // CLIP tag
    gnode = xmlNewNode(NULL, (const xmlChar*)TAG_CLIP);
    xmlAddChild(vecroot, gnode);

    GString *id;
    id = new GString("p");
    xmlNewProp(gnode, (const xmlChar*)ATTR_SID, (const xmlChar*)buildSID(num, getIdx(), id)->getCString());
    delete id;

    // Get the clipping box
    state->getClipBBox(&xMin, &yMin, &xMax, &yMax);
    sprintf(tmp, "%g", xMin);
    xmlNewProp(gnode, (const xmlChar*)ATTR_X, (const xmlChar*)tmp);
    sprintf(tmp, "%g", yMin);
    xmlNewProp(gnode, (const xmlChar*)ATTR_Y, (const xmlChar*)tmp);
    sprintf(tmp, "%g", xMax-xMin);
    xmlNewProp(gnode, (const xmlChar*)ATTR_WIDTH, (const xmlChar*)tmp);
    sprintf(tmp, "%g", yMax-yMin);
    xmlNewProp(gnode, (const xmlChar*)ATTR_HEIGHT, (const xmlChar*)tmp);

    id = new GString("p");
    xmlNewProp(gnode, (const xmlChar*)ATTR_IDCLIPZONE,
               (const xmlChar*)buildIdClipZone(num, idClip, id)->getCString());
    delete id;
    //   	free(tmp);
    doPathForClip(state->getPath(), state, gnode);
}

void TextPage::eoClip(GfxState *state) {

    idClip++;
    idCur = idClip;

    xmlNodePtr gnode = NULL;
    char tmp[100];
    double xMin = 0;
    double yMin = 0;
    double xMax = 0;
    double yMax = 0;

    // Increment the absolute object index
    idx++;

    // CLIP tag
    gnode=xmlNewNode(NULL, (const xmlChar*)TAG_CLIP);
    xmlAddChild(vecroot, gnode);

    GString *id;
    id = new GString("p");
    xmlNewProp(gnode, (const xmlChar*)ATTR_SID, (const xmlChar*)buildSID(num, getIdx(), id)->getCString());
    delete id;

    // Get the clipping box
    state->getClipBBox(&xMin, &yMin, &xMax, &yMax);
    sprintf(tmp, "%g", xMin);
    xmlNewProp(gnode, (const xmlChar*)ATTR_X, (const xmlChar*)tmp);
    sprintf(tmp, "%g", yMin);
    xmlNewProp(gnode, (const xmlChar*)ATTR_Y, (const xmlChar*)tmp);
    sprintf(tmp, "%g", xMax-xMin);
    xmlNewProp(gnode, (const xmlChar*)ATTR_WIDTH, (const xmlChar*)tmp);
    sprintf(tmp, "%g", yMax-yMin);
    xmlNewProp(gnode, (const xmlChar*)ATTR_HEIGHT, (const xmlChar*)tmp);

    id = new GString("p");
    xmlNewProp(gnode, (const xmlChar*)ATTR_IDCLIPZONE,
               (const xmlChar*)buildIdClipZone(num, idClip, id)->getCString());
    delete id;

    xmlNewProp(gnode, (const xmlChar*)ATTR_EVENODD, (const xmlChar*)sTRUE);
    //   	free(tmp);
    doPathForClip(state->getPath(), state, gnode);
}

void TextPage::clipToStrokePath(GfxState *state) {

    idClip++;
    idCur = idClip;

    xmlNodePtr gnode = NULL;
    char tmp[100];
    double xMin = 0;
    double yMin = 0;
    double xMax = 0;
    double yMax = 0;

    // Increment the absolute object index
    idx++;

    // CLIP tag
    gnode=xmlNewNode(NULL, (const xmlChar*)TAG_CLIP);
    xmlAddChild(vecroot, gnode);

    GString *id;
    id = new GString("p");
    xmlNewProp(gnode, (const xmlChar*)ATTR_SID, (const xmlChar*)buildSID(num, getIdx(), id)->getCString());
    delete id;

    // Get the clipping box
    state->getClipBBox(&xMin, &yMin, &xMax, &yMax);
    sprintf(tmp, "%g", xMin);
    xmlNewProp(gnode, (const xmlChar*)ATTR_X, (const xmlChar*)tmp);
    sprintf(tmp, "%g", yMin);
    xmlNewProp(gnode, (const xmlChar*)ATTR_Y, (const xmlChar*)tmp);
    sprintf(tmp, "%g", xMax-xMin);
    xmlNewProp(gnode, (const xmlChar*)ATTR_WIDTH, (const xmlChar*)tmp);
    sprintf(tmp, "%g", yMax-yMin);
    xmlNewProp(gnode, (const xmlChar*)ATTR_HEIGHT, (const xmlChar*)tmp);

    id = new GString("p");
    xmlNewProp(gnode, (const xmlChar*)ATTR_IDCLIPZONE,
               (const xmlChar*)buildIdClipZone(num, idClip, id)->getCString());
    delete id;

    xmlNewProp(gnode, (const xmlChar*)ATTR_EVENODD, (const xmlChar*)sTRUE);
    //   	free(tmp);
    doPathForClip(state->getPath(), state, gnode);

}

// Draw the image mask
void TextPage::drawImageMask(GfxState *state, Object *ref, Stream *str,
                             int width, int height, GBool invert, GBool inlineImg, GBool dumpJPEG,
                             int imageIndex) {


    // ugly code / to be simplified

    int i;
    FILE *f;
    int c;
    int size;
    GString *id;

    double x0, y0; // top left corner of image
    double w0, h0, w1, h1; // size of image
    double xt, yt, wt, ht;
    GBool rotate, xFlip, yFlip;
    char tmp[10];

    xmlNodePtr node = NULL;

    // Increment the absolute object index
    idx++;

    // get image position and size
    state->transform(0, 0, &xt, &yt);
    state->transformDelta(1, 1, &wt, &ht);
    if (wt > 0) {
        x0 = xt;
        w0 = wt;
    } else {
        x0 = xt + wt;
        w0 = -wt;
    }
    if (ht > 0) {
        y0 = yt;
        h0 = ht;
    } else {
        y0 = yt + ht;
        h0 = -ht;
    }
    state->transformDelta(1, 0, &xt, &yt);
    rotate = fabs(xt) < fabs(yt);
    if (rotate) {
        w1 = h0;
        h1 = w0;
        xFlip = ht < 0;
        yFlip = wt > 0;
    } else {
        w1 = w0;
        h1 = h0;
        xFlip = wt < 0;
        yFlip = ht > 0;
    }



    // HREF
    GString *relname = new GString(RelfileName);
    relname->append("-");
    relname->append(GString::fromInt(imageIndex));
    GString *refname = new GString(ImgfileName);
    refname->append("-");
    refname->append(GString::fromInt(imageIndex));
    if(dumpJPEG && str->getKind() == strDCT && !inlineImg) {
        relname->append(".jpg");
        refname->append(".jpg");
        // initialize stream
        str = ((DCTStream *)str)->getRawStream();
        str->reset();

        // copy the stream
        while ((c = str->getChar()) != EOF)
            fputc(c, f);

        str->close();
        fclose(f);
    } else {
        // open the image file and write the PBM header
        relname->append(".pbm");
        refname->append(".pbm");

        if (!(f = fopen(relname->getCString(), "wb"))) {
            //error(-1, "Couldn't open image file '%s'", relname->getCString());
            return;
        }
        fprintf(f, "P4\n");
        fprintf(f, "%d %d\n", width, height);

        // initialize stream
        str->reset();

        // copy the stream
        size = height * ((width + 7) / 8);
        for (i = 0; i < size; ++i) {
            fputc(str->getChar(), f);
        }

        str->close();
        fclose(f);
    }

    if (!inlineImg || (inlineImg && parameters->getImageInline())) {
        node = xmlNewNode(NULL, (const xmlChar*)TAG_IMAGE);
        GString *id;
        id = new GString("p");
        xmlNewProp(node, (const xmlChar*)ATTR_ID, (const xmlChar*)buildIdImage(num, numImage, id)->getCString());
        delete id;
        numImage = numImage + 1;

        id = new GString("p");
        xmlNewProp(node, (const xmlChar*)ATTR_SID, (const xmlChar*)buildSID(num, getIdx(), id)->getCString());
        delete id;

        sprintf(tmp, ATTR_NUMFORMAT, x0);
        xmlNewProp(node, (const xmlChar*)ATTR_X, (const xmlChar*)tmp);
        sprintf(tmp, ATTR_NUMFORMAT, y0);

        xmlNewProp(node, (const xmlChar*)ATTR_Y, (const xmlChar*)tmp);
        sprintf(tmp, ATTR_NUMFORMAT, w0);
        xmlNewProp(node, (const xmlChar*)ATTR_WIDTH, (const xmlChar*)tmp);
        sprintf(tmp, ATTR_NUMFORMAT, h0);
        xmlNewProp(node, (const xmlChar*)ATTR_HEIGHT, (const xmlChar*)tmp);
        if (inlineImg) {
            xmlNewProp(node, (const xmlChar*)ATTR_INLINE, (const xmlChar*)sTRUE);
        }
        xmlNewProp(node, (const xmlChar*)ATTR_MASK, (const xmlChar*)sTRUE);
        xmlNewProp(node, (const xmlChar*)ATTR_HREF,
                   (const xmlChar*)refname->getCString());
        xmlAddChild(printSpace, node);
    }

    if (inlineImg && !parameters->getImageInline()) {
        listeImageInline.push_back(new ImageInline(x0, y0, w0, h0, getIdWORD(), imageIndex, refname, getIdx()));
    }

    id = new GString("p");
    xmlNewProp(node, (const xmlChar*)ATTR_CLIPZONE,
               (const xmlChar*)buildIdClipZone(num, idCur, id)->getCString());
    delete id;
    //  free(tmp);
    return;
}
// Draw the image
void TextPage::drawImage(GfxState *state, Object *ref, Stream *str, int width,
                         int height, GfxImageColorMap *colorMap, int *maskColors,
                         GBool inlineImg, GBool dumpJPEG, int imageIndex) {

    // ugly code / to be simplified


    int i;
    FILE *f;
    int size;
    Guchar *p;
    GfxRGB rgb;
    ImageStream *imgStr;
    int c;
    double x0, y0; // top left corner of image
    double w0, h0, w1, h1; // size of image
    double xt, yt, wt, ht;
    GBool rotate, xFlip, yFlip;
    int x, y;
    GString *id;

    xmlNodePtr node = NULL;

    char tmp[10];

    // Increment the absolute object index
    idx++;

    // get image position and size
    state->transform(0, 0, &xt, &yt);
    state->transformDelta(1, 1, &wt, &ht);
    if (wt > 0) {
        x0 = (xt);
        w0 = (wt);
    } else {
        x0 = (xt + wt);
        w0 = (-wt);
    }
    if (ht > 0) {
        y0 = (yt);
        h0 = (ht);
    } else {
        y0 = (yt + ht);
        h0 = (-ht);
    }
    state->transformDelta(1, 0, &xt, &yt);
    rotate = fabs(xt) < fabs(yt);
    if (rotate) {
        w1 = h0;
        h1 = w0;
        xFlip = ht < 0;
        yFlip = wt > 0;
    } else {
        w1 = w0;
        h1 = h0;
        xFlip = wt < 0;
        yFlip = ht > 0;
    }

    GString *relname = new GString(RelfileName);
    relname->append("-");
    relname->append(GString::fromInt(imageIndex));

    GString *refname = new GString(ImgfileName);
    refname->append("-");
    refname->append(GString::fromInt(imageIndex));

    // HREF
    if (dumpJPEG && str->getKind() == strDCT && colorMap->getNumPixelComps()
                                                == 3 && !inlineImg) {
        refname->append(".jpg");
        relname->append(".jpg");
        if (!(f = fopen(relname->getCString(), "wb"))) {
            //error(-1, "Couldn't open image file '%s'", relname->getCString());
            return;
        }

        // initialize stream
        str = ((DCTStream *)str)->getRawStream();
        str->reset();

        // copy the stream
        while ((c = str->getChar()) != EOF)
            fputc(c, f);

        str->close();
        fclose(f);
    } else if (colorMap->getNumPixelComps() == 1 && colorMap->getBits() == 1) {
        refname->append(".pbm");
        relname->append(".pbm");

        // open the image file and write the PBM header
        if (!(f = fopen(relname->getCString(), "wb"))) {
            //error(-1, "Couldn't open image file '%s'", relname->getCString());
            return;
        }
        fprintf(f, "P4\n");
        fprintf(f, "%d %d\n", width, height);

        // initialize stream
        str->reset();

        // copy the stream
        size = height * ((width + 7) / 8);
        for (i = 0; i < size; ++i) {
            fputc(str->getChar() ^ 0xff, f);
        }

        str->close();
        fclose(f);
    } else {
        refname->append(".ppm");
        relname->append(".ppm");
        if (!(f = fopen(relname->getCString(), "wb"))) {
            //error(-1, (char*)"Couldn't open image file '%s'", relname->getCString());
            return;
        }
        fprintf(f, "P6\n");
        fprintf(f, "%d %d\n", width, height);
        fprintf(f, "255\n");

        // initialize stream
        imgStr = new ImageStream(str, width, colorMap->getNumPixelComps(), colorMap->getBits());
        imgStr->reset();

        // for each lin21e...
        for (y = 0; y < height; ++y) {

            // write the line
            p = imgStr->getLine();
            for (x = 0; x < width; ++x) {
                colorMap->getRGB(p, &rgb);
                fputc(colToByte(rgb.r), f);
                fputc(colToByte(rgb.g), f);
                fputc(colToByte(rgb.b), f);
                p += colorMap->getNumPixelComps();
            }
        }
        delete imgStr;
        fclose(f);
    }

    if (!inlineImg || (inlineImg && parameters->getImageInline())) {
        node = xmlNewNode(NULL, (const xmlChar*)TAG_IMAGE);
        GString *id;
        id = new GString("p");
        xmlNewProp(node, (const xmlChar*)ATTR_ID, (const xmlChar*)buildIdImage(num, numImage, id)->getCString());
        delete id;
        numImage = numImage + 1;

        id = new GString("p");
        xmlNewProp(node, (const xmlChar*)ATTR_SID, (const xmlChar*)buildSID(num, getIdx(), id)->getCString());
        delete id;

        sprintf(tmp, ATTR_NUMFORMAT, x0);
        xmlNewProp(node, (const xmlChar*)ATTR_X, (const xmlChar*)tmp);
        sprintf(tmp, ATTR_NUMFORMAT, y0);
        xmlNewProp(node, (const xmlChar*)ATTR_Y, (const xmlChar*)tmp);
        sprintf(tmp, ATTR_NUMFORMAT, w0);
        xmlNewProp(node, (const xmlChar*)ATTR_WIDTH, (const xmlChar*)tmp);
        sprintf(tmp, ATTR_NUMFORMAT, h0);
        xmlNewProp(node, (const xmlChar*)ATTR_HEIGHT, (const xmlChar*)tmp);
        //	if (rotate){
        //		xmlNewProp(node,(const xmlChar*)ATTR_ROTATION,(const xmlChar*)sTRUE);
        //	}
        //	else{
        //		xmlNewProp(node,(const xmlChar*)ATTR_ROTATION,(const xmlChar*)sFALSE);
        //	}
        if (inlineImg) {
            xmlNewProp(node, (const xmlChar*)ATTR_INLINE, (const xmlChar*)sTRUE);
        }
        xmlNewProp(node, (const xmlChar*)ATTR_HREF,
                   (const xmlChar*)refname->getCString());
        xmlAddChild(printSpace, node);
    }

    if (inlineImg && !parameters->getImageInline()) {
        listeImageInline.push_back(new ImageInline(x0, y0, w0, h0, getIdWORD(), imageIndex, refname, getIdx()));
    }

    id = new GString("p");
    xmlNewProp(node, (const xmlChar*)ATTR_CLIPZONE,
               (const xmlChar*)buildIdClipZone(num, idCur, id)->getCString());
    delete id;

    return;
}

const char* TextPage::drawImageOrMask(GfxState *state, Object* ref, Stream *str,
                                      int width, int height,
                                      GfxImageColorMap *colorMap,
                                      int* /* maskColors */, GBool inlineImg, GBool mask, int imageIndex)
{
    GString pic_file;

    double x0, y0; // top left corner of image
    double w0, h0, w1, h1; // size of image
    double xt, yt, wt, ht;

    GBool rotate, xFlip, yFlip;
    GString *id;

    xmlNodePtr node = NULL;
    const char* extension = NULL;

    char tmp[10];

    // Increment the absolute object index
    idx++;

    // get image position and size
    state->transform(0, 0, &xt, &yt);
    state->transformDelta(1, 1, &wt, &ht);
    if (wt > 0) {
        x0 = (xt);
        w0 = (wt);
    } else {
        x0 = (xt + wt);
        w0 = (-wt);
    }
    if (ht > 0) {
        y0 = (yt);
        h0 = (ht);
    } else {
        y0 = (yt + ht);
        h0 = (-ht);
    }

    // register the block in the block structure of the page
    double x1, y1, x2, y2, temp;
    bool flip_x = false;
    bool flip_y = false;
    int flip = 0;
    // when drawing a picture, we are in the scaled coordinates of the picture
    // in which the top-left corner is at coordinates (0,1) and
    // and  the bottom-right corner is at coordinates (1,0).
    // 0 1
    // 1 0
    state->transform(0, 1, &x1, &y1);
    state->transform(1, 0, &x2, &y2);

    // Detect if the picture is printed flipped
    if (x1 > x2)
    {
        flip |= 1;
        flip_x = true;
        temp = x1;
        x1 = x2;
        x2 = temp;
    }
    if (y1 > y2)
    {
        flip |= 2;
        flip_y = true;
        temp = y1;
        y1 = y2;
        y2 = temp;
    }
    GString *relname = new GString(RelfileName);
    relname->append("-");
    relname->append(GString::fromInt(imageIndex));

    GString *refname = new GString(ImgfileName);
    refname->append("-");
    refname->append(GString::fromInt(imageIndex));

//	if (pic_file.getLength() == 0)
    if (1)
    {
        // ------------------------------------------------------------
        // dump JPEG file
        // ------------------------------------------------------------

        if (str->getKind() == strDCT && (mask || colorMap->getNumPixelComps() == 3) && !inlineImg)
        {
            // TODO, do we need to flip Jpegs too?

            // open image file
            extension = ".jpg";
            relname->append(".jpg");
            refname->append(".jpg");
//			compose_image_filename(dev_picture_base, ++dev_picture_number, extension, pic_file);

            FILE* img_file = fopen(relname->getCString(), "wb");
            if (img_file != NULL)
            {
                // initialize stream
                str = ((DCTStream *)str)->getRawStream();
                str->reset();

                int c;

                // copy the stream
                while ((c = str->getChar()) != EOF)
                {
                    fputc(c, img_file);
                }

                // cleanup
                str->close();
                // file cleanup
                fclose(img_file);
            }
            // else TODO report error
        }

            // ------------------------------------------------------------
            // dump black and white image
            // ------------------------------------------------------------

        else if (mask || (colorMap->getNumPixelComps() == 1 && colorMap->getBits() == 1))
        {
            extension = ".png";
            relname->append(".png");
            refname->append(".png");
//			compose_image_filename(dev_picture_base, ++dev_picture_number, extension, pic_file);

            int stride = (width + 7) >> 3;
            unsigned char* data = new unsigned char[stride * height];

            if (data != NULL)
            {
                str->reset();

                // Prepare increments and initial value for flipping
                int k, x_increment, y_increment;

                if (flip_x)
                {
                    if (flip_y)
                    {
                        // both flipped
                        k = height * stride - 1;
                        x_increment = -1;
                        y_increment = 0;
                    }
                    else
                    {
                        // x flipped
                        k = (stride - 1);
                        x_increment = -1;
                        y_increment = 2 * stride;
                    }
                }
                else
                {
                    if (flip_y)
                    {
                        // y flipped
                        k = (height - 1) * stride;
                        x_increment = 1;
                        y_increment = -2 * stride;
                    }
                    else
                    {
                        // not flipped
                        k = 0;
                        x_increment = 1;
                        y_increment = 0;
                    }
                }

                // Retrieve the image raw data (columnwise monochrome pixels)
                for (int y = 0; y < height; y++)
                {
                    for (int x = 0; x < stride; x++)
                    {
                        data[k] = (unsigned char) str->getChar();
                        k += x_increment;
                    }

                    k += y_increment;
                }

                // there is more if the image is flipped in x...
                if (flip_x)
                {
                    int total = height * stride;
                    unsigned char a;

                    // bitwise flip of all bytes:
                    for (k = 0; k < total; k++)
                    {
                        a		= data[k];
                        a		= ( a                         >> 4) + ( a                         << 4);
                        a		= ((a & 0xCC /* 11001100b */) >> 2) + ((a & 0x33 /* 00110011b */) << 2);
                        data[k]	= ((a & 0xAA /* 10101010b */) >> 1) + ((a & 0x55 /* 01010101b */) << 1);
                    }

                    int complementary_shift = (width & 7);

                    if (complementary_shift != 0)
                    {
                        // now shift everything <shift> bits
                        int shift = 8 - complementary_shift;
                        unsigned char mask = 0xFF << complementary_shift;	// mask for remainder
                        unsigned char b;
                        unsigned char remainder = 0; // remainder is part that comes out when shifting
                        // a byte which is reintegrated in the next byte

                        for (k = total - 1; k >= 0; k--)
                        {
                            a = data[k];
                            b = (a & mask) >> complementary_shift;
                            data[k] = (a << shift) | remainder;
                            remainder = b;
                        }
                    }
                }

                str->close();

                // Set a B&W palette
                png_color palette[2];
                palette[0].red = palette[0].green = palette[0].blue = 0;
                palette[1].red = palette[1].green = palette[1].blue = 0xFF;

                // Save PNG file
                save_png(relname, width, height, stride, data, 1, PNG_COLOR_TYPE_PALETTE, palette, 2);

            }
        }

            // ------------------------------------------------------------
            // dump color or greyscale image
            // ------------------------------------------------------------

        else
        {
            extension = ".png";
            refname->append(".png");
            relname->append(".png");

            unsigned char* data = new unsigned char[width * height * 3];

            if (data != NULL)
            {
                ImageStream* imgStr = new ImageStream(str, width, colorMap->getNumPixelComps(), colorMap->getBits());
                imgStr->reset();

                GfxRGB rgb;

                // Prepare increments and initial value for flipping
                int k, x_increment, y_increment;

                if (flip_x)
                {
                    if (flip_y)
                    {
                        // both flipped
                        k = 3 * height * width - 3;
                        x_increment = -6;
                        y_increment = 0;
                    }
                    else
                    {
                        // x flipped
                        k = 3 * (width - 1);
                        x_increment = -6;
                        y_increment = 6 * width;
                    }
                }
                else
                {
                    if (flip_y)
                    {
                        // y flipped
                        k = 3 * (height - 1) * width;
                        x_increment = 0;
                        y_increment = -6 * width;
                    }
                    else
                    {
                        // not flipped
                        k = 0;
                        x_increment = 0;
                        y_increment = 0;
                    }
                }

                // Retrieve the image raw data (RGB pixels)
                for (int y = 0; y < height; y++)
                {
                    Guchar* p = imgStr->getLine();
                    for (int x = 0; x < width; x++)
                    {
                        colorMap->getRGB(p, &rgb);
                        data[k++] = clamp(rgb.r >> 8);
                        data[k++] = clamp(rgb.g >> 8);
                        data[k++] = clamp(rgb.b >> 8);
                        k += x_increment;
                        p += colorMap->getNumPixelComps();
                    }

                    k += y_increment;
                }

                delete imgStr;

                // Save PNG file
                save_png(relname, width, height, width * 3, data, 24, PNG_COLOR_TYPE_RGB, NULL, 0);
            }
        }

    }
    if (!inlineImg || (inlineImg && parameters->getImageInline())) {
        node = xmlNewNode(NULL, (const xmlChar*)TAG_IMAGE);
        GString *id;
        id = new GString("p");
        xmlNewProp(node, (const xmlChar*)ATTR_ID, (const xmlChar*)buildIdImage(num, numImage, id)->getCString());
        delete id;
        numImage = numImage + 1;

        id = new GString("p");
        xmlNewProp(node, (const xmlChar*)ATTR_SID, (const xmlChar*)buildSID(num, getIdx(), id)->getCString());
        delete id;

        sprintf(tmp, ATTR_NUMFORMAT, x0);
        xmlNewProp(node, (const xmlChar*)ATTR_X, (const xmlChar*)tmp);
        sprintf(tmp, ATTR_NUMFORMAT, y0);
        xmlNewProp(node, (const xmlChar*)ATTR_Y, (const xmlChar*)tmp);
        sprintf(tmp, ATTR_NUMFORMAT, w0);
        xmlNewProp(node, (const xmlChar*)ATTR_WIDTH, (const xmlChar*)tmp);
        sprintf(tmp, ATTR_NUMFORMAT, h0);
        xmlNewProp(node, (const xmlChar*)ATTR_HEIGHT, (const xmlChar*)tmp);
//			if (rotate){
//				xmlNewProp(node,(const xmlChar*)ATTR_ROTATION,(const xmlChar*)sTRUE);
//			}
//			else{
//				xmlNewProp(node,(const xmlChar*)ATTR_ROTATION,(const xmlChar*)sFALSE);
//			}
        if (inlineImg) {
            xmlNewProp(node, (const xmlChar*)ATTR_INLINE, (const xmlChar*)sTRUE);
        }
        xmlNewProp(node, (const xmlChar*)ATTR_HREF,
                   (const xmlChar*)refname->getCString());
        xmlAddChild(printSpace, node);
    }

    if (inlineImg && !parameters->getImageInline()) {
        listeImageInline.push_back(new ImageInline(x0, y0, w0, h0, getIdWORD(), imageIndex, refname, getIdx()));
    }

    id = new GString("p");
    xmlNewProp(node, (const xmlChar*)ATTR_CLIPZONE,
               (const xmlChar*)buildIdClipZone(num, idCur, id)->getCString());
    delete id;

    return extension;
//	append_image_block(round(x1), round(y1), round(x2-x1), round(y2-y1), pic_file);
}


void file_write_data (png_structp png_ptr, png_bytep data, png_size_t length)
{
    FILE* file = (FILE*) png_ptr->io_ptr;

    if (fwrite(data, 1, length,file) != length)
        png_error(png_ptr, "Write Error");
}

//------------------------------------------------------------

void file_flush_data (png_structp png_ptr)
{
    FILE* file = (FILE*) png_ptr->io_ptr;

    if (fflush(file))
        png_error(png_ptr, "Flush Error");
}


bool TextPage::save_png (GString* file_name,
                         unsigned int width, unsigned int height, unsigned int row_stride,
                         unsigned char* data,
                         unsigned char bpp, unsigned char color_type, png_color* palette, unsigned short color_count)
{
    png_struct *png_ptr;
    png_info *info_ptr;

    // Create necessary structs
    png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (png_ptr == NULL)
    {
        return false;
    }

    info_ptr = png_create_info_struct(png_ptr);
    if (info_ptr == NULL)
    {
        png_destroy_write_struct(&png_ptr, (png_infopp) NULL);
        return false;
    }

    // Open file
    FILE* file = fopen(file_name->getCString(), "wb");
    if (file == NULL)
    {
        png_destroy_write_struct(&png_ptr, (png_infopp) NULL);
        return false;
    }

    if (setjmp(png_ptr->jmpbuf))
    {
        png_destroy_write_struct(&png_ptr, (png_infopp) &info_ptr);
        fclose(file);
        return false;
    }

    // Writing functions
    png_set_write_fn(png_ptr, file, (png_rw_ptr) file_write_data, (png_flush_ptr) file_flush_data);

    // Image header
    info_ptr->width				= width;
    info_ptr->height			= height;
    info_ptr->pixel_depth		= bpp;
    info_ptr->channels			= (bpp>8) ? (unsigned char)3: (unsigned char)1;
    info_ptr->bit_depth			= (unsigned char)(bpp/info_ptr->channels);
    info_ptr->color_type		= color_type;
    info_ptr->compression_type	= info_ptr->filter_type = 0;
    info_ptr->valid				= 0;
    info_ptr->rowbytes			= row_stride;
    info_ptr->interlace_type	= PNG_INTERLACE_NONE;

    // Background
    png_color_16 image_background={ 0, 255, 255, 255, 0 };
    png_set_bKGD(png_ptr, info_ptr, &image_background);

    // Metrics
    png_set_pHYs(png_ptr, info_ptr, 3780, 3780, PNG_RESOLUTION_METER); // 3780 dot per meter

    // Palette
    if (palette != NULL)
    {
        png_set_IHDR(png_ptr, info_ptr, info_ptr->width, info_ptr->height, info_ptr->bit_depth,
                     PNG_COLOR_TYPE_PALETTE, info_ptr->interlace_type,
                     PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
        info_ptr->valid |= PNG_INFO_PLTE;
        info_ptr->palette = palette;
        info_ptr->num_palette = color_count;
    }

    // Write the file header
    png_write_info(png_ptr, info_ptr);

    // Interlace handling
    int num_pass = png_set_interlace_handling(png_ptr);
    for (int pass = 0; pass < num_pass; pass++){
        for (unsigned int y = 0; y < height; y++)
        {
            png_write_row(png_ptr, &data[row_stride * y]);
        }
    }

    // Finish writing
    png_write_end(png_ptr, info_ptr);

    // Cleanup
    png_destroy_write_struct(&png_ptr, (png_infopp) &info_ptr);

    fclose(file);

    return true;
}



//------------------------------------------------------------------------
// XmlAltoOutputDev
//------------------------------------------------------------------------

XmlAltoOutputDev::XmlAltoOutputDev(GString *fileName, GString *fileNamePdf,
                           Catalog *catalog, GBool physLayoutA, GBool verboseA, GString *nsURIA,
                           GString *cmdA)	{
    text = NULL;
    physLayout = physLayoutA;
    rawOrder = 1;
    ok = gTrue;
    doc = NULL;
    vecdoc = NULL;
    docroot = NULL;
    vecroot = NULL;
    verbose = verboseA;
    GString *imgDirName;
    Catalog *myCatalog;

    //curstate=(double*)malloc(10000*sizeof(6*double));

    myCatalog = catalog;
    UnicodeMap *uMap;

    blocks = parameters->getDisplayBlocks();
    fullFontName = parameters->getFullFontName();
    noImageInline = parameters->getImageInline();

    fileNamePDF = new GString(fileNamePdf);

    if (nsURIA) {
        nsURI = new GString(nsURIA);
    } else {
        nsURI = NULL;
    }

    myfilename = new GString(fileName);

    dataDir = new GString(fileName);
    dataDir->append("_data");

    imgDirName = new GString(dataDir);

    // Display images
    if (parameters->getDisplayImage() || !parameters->getCutAllPages()) {

#ifdef WIN32
        _mkdir(dataDir->getCString());
#else
        mkdir(dataDir->getCString(), 00777);
#endif

        imgDirName->append("/image");
        imageIndex = 0;

#ifndef WIN32
        char *aux = strdup(fileName->getCString());
        baseFileName = new GString(basename(aux));
        baseFileName->append("_data/image");
        free(aux);
#endif

#ifdef WIN32
        baseFileName = new GString(fileName);
		baseFileName->append("_data/image");
#endif

    }// end IF


    lPictureReferences = new GList();

    doc = xmlNewDoc((const xmlChar*)VERSION);

    globalParams->setTextEncoding((char*)ENCODING_UTF8);
    doc->encoding = xmlStrdup((const xmlChar*)ENCODING_UTF8);
    docroot = xmlNewNode(NULL, (const xmlChar*)TAG_ALTO);

    xmlNsPtr nsXi = xmlNewNs(docroot, (const xmlChar*)ALTO_URI,
                             NULL);
    xmlSetNs(docroot, nsXi);

    nsXi = xmlNewNs(docroot, (const xmlChar*)XLINK_URI,
                    (const xmlChar*)XLINK_PREFIX);

    nsXi = xmlNewNs(docroot, (const xmlChar*)XSI_URI,
                    (const xmlChar*)XSI_PREFIX);

    xmlNewNsProp(docroot, nsXi, (const xmlChar*)SCHEMA_LOCATION_ATTR_NAME,
                 (const xmlChar*)SCHEMA_LOCATION_URI);

    // The namespace DS to add at the DOCUMENT tag
    if (nsURI) {
        xmlNewNs(docroot, (const xmlChar*)nsURI->getCString(), NULL);
    }

    xmlDocSetRootElement(doc, docroot);


    // here we add basic structure : see https://www.loc.gov/standards/alto/techcenter/structure.html
    xmlNodePtr nodeDescription = xmlNewNode(NULL, (const xmlChar*)TAG_DESCRIPTION);
    nodeDescription->type = XML_ELEMENT_NODE;
    xmlAddChild(docroot, nodeDescription);


    xmlNodePtr nodeStyles = xmlNewNode(NULL, (const xmlChar*)TAG_STYLES);
    nodeStyles->type = XML_ELEMENT_NODE;
    xmlAddChild(docroot, nodeStyles);

    xmlNodePtr nodeLayout = xmlNewNode(NULL, (const xmlChar*)TAG_LAYOUT);
    nodeLayout->type = XML_ELEMENT_NODE;
    xmlAddChild(docroot, nodeLayout);

    // adding the processing description
    xmlNodePtr nodeMeasurementUnit = xmlNewNode(NULL, (const xmlChar*)TAG_MEASUREMENTUNIT);
    nodeMeasurementUnit->type = XML_ELEMENT_NODE;
    xmlNodeSetContent(nodeMeasurementUnit,
                      (const xmlChar*)xmlEncodeEntitiesReentrant(nodeMeasurementUnit->doc,
                                                                 (const xmlChar*)MEASUREMENT_UNIT));
    xmlAddChild(nodeDescription, nodeMeasurementUnit);

    xmlNodePtr nodeSourceImageInformation = xmlNewNode(NULL, (const xmlChar*)TAG_SOURCE_IMAGE_INFO);
    nodeSourceImageInformation->type = XML_ELEMENT_NODE;
    xmlAddChild(nodeDescription, nodeSourceImageInformation);

    xmlNodePtr nodeNameFilePdf = xmlNewNode(NULL,
                                            (const xmlChar*)TAG_PDFFILENAME);
    nodeNameFilePdf->type = XML_ELEMENT_NODE;
    if (!(uMap = globalParams->getTextEncoding())) {
        return;
    }
    GString *title;
    title = new GString();
    title= toUnicode(fileNamePDF,uMap);
//	dumpFragment((Unicode*)fileNamePDF, fileNamePDF->getLength(), uMap, title);
    xmlNodeSetContent(nodeNameFilePdf,
                      (const xmlChar*)xmlEncodeEntitiesReentrant(nodeNameFilePdf->doc,
                                                                 (const xmlChar*)title->getCString()));
    xmlAddChild(nodeSourceImageInformation, nodeNameFilePdf);

    xmlNodePtr nodeOCRProcessing = xmlNewNode(NULL, (const xmlChar*)TAG_OCRPROCESSING);
    nodeOCRProcessing->type = XML_ELEMENT_NODE;
    xmlAddChild(nodeDescription, nodeOCRProcessing);
    xmlNewProp(nodeOCRProcessing, (const xmlChar*)ATTR_NAMEID_OCRPROCESSING,
               (const xmlChar*)ATTR_VALUEID_OCRPROCESSING);

    xmlNodePtr nodeOCRProcessingStep = xmlNewNode(NULL, (const xmlChar*)TAG_OCRPROCESSINGSTEP);
    nodeOCRProcessingStep->type = XML_ELEMENT_NODE;
    xmlAddChild(nodeOCRProcessing, nodeOCRProcessingStep);

    xmlNodePtr nodeProcessingDate = xmlNewNode(NULL, (const xmlChar*)TAG_PROCESSINGDATE);
    nodeProcessingDate->type = XML_ELEMENT_NODE;
    xmlAddChild(nodeOCRProcessingStep, nodeProcessingDate);
    time_t t;
    time(&t);
    xmlNodeSetContent(nodeProcessingDate, (const xmlChar*)xmlEncodeEntitiesReentrant(
            nodeProcessingDate->doc, (const xmlChar*)ctime(&t)));

    xmlNodePtr nodeProcessingSoftware = xmlNewNode(NULL, (const xmlChar*)TAG_PROCESSINGSOFTWARE);
    nodeProcessingSoftware->type = XML_ELEMENT_NODE;
    xmlAddChild(nodeOCRProcessingStep, nodeProcessingSoftware);

    xmlNodePtr nodeSoftwareCreator = xmlNewNode(NULL, (const xmlChar*)TAG_SOFTWARE_CREATOR);
    nodeSoftwareCreator->type = XML_ELEMENT_NODE;
    xmlNodeSetContent(nodeSoftwareCreator,
                      (const xmlChar*)xmlEncodeEntitiesReentrant(nodeSoftwareCreator->doc,
                                                                 (const xmlChar*)PDFALTO_CREATOR));
    xmlAddChild(nodeProcessingSoftware, nodeSoftwareCreator);

    xmlNodePtr nodeSoftwareName = xmlNewNode(NULL, (const xmlChar*)TAG_SOFTWARE_NAME);
    nodeSoftwareName->type = XML_ELEMENT_NODE;
    xmlNodeSetContent(nodeSoftwareName,
                      (const xmlChar*)xmlEncodeEntitiesReentrant(nodeSoftwareName->doc,
                                                                 (const xmlChar*)PDFALTO_NAME));
    xmlAddChild(nodeProcessingSoftware, nodeSoftwareName);
//    xmlNewProp(nodeProcess, (const xmlChar*)ATTR_CMD,
//               (const xmlChar*)cmdA->getCString());

    xmlNodePtr nodeSoftwareVersion = xmlNewNode(NULL, (const xmlChar*)TAG_SOFTWARE_VERSION);
    nodeSoftwareVersion->type = XML_ELEMENT_NODE;
    xmlNodeSetContent(nodeSoftwareVersion,
                      (const xmlChar*)xmlEncodeEntitiesReentrant(nodeSoftwareName->doc,
                                                                 (const xmlChar*)PDFALTO_VERSION));
    xmlAddChild(nodeProcessingSoftware, nodeSoftwareVersion);


    // The file of vectorials instructions
    vecdoc = xmlNewDoc((const xmlChar*)VERSION);
    vecdoc->encoding = xmlStrdup((const xmlChar*)ENCODING_UTF8);
    vecroot = xmlNewNode(NULL, (const xmlChar*)TAG_VECTORIALINSTRUCTIONS);

    xmlDocSetRootElement(vecdoc, vecroot);

    xmlNewProp(vecroot, (const xmlChar*)"file",
               (const xmlChar*)fileName->getCString());

    needClose = gFalse;

    delete fileNamePDF;

    //
    text = new TextPage(verbose, catalog, nodeLayout, imgDirName, baseFileName, nsURI);
}

XmlAltoOutputDev::~XmlAltoOutputDev() {
    xmlSaveFile(myfilename->getCString(), doc);
    xmlFreeDoc(doc);

    for (int i = 0; i < lPictureReferences->getLength(); i++)
    {
        delete ((PictureReference*) lPictureReferences->get(i));
    }
    if (text) {
        delete text;
    }
    if (nsURI) {
        delete nsURI;
    }
}





void XmlAltoOutputDev::addInfo(PDFDocXrce *pdfdocxrce){
    Object info;

    GString *content;

    xmlNodePtr nodeSourceImageInfo = findNodeByName(docroot, (const xmlChar*)TAG_SOURCE_IMAGE_INFO);

    xmlNodePtr titleNode = xmlNewNode(NULL, (const xmlChar*)"TITLE");
    xmlNodePtr subjectNode = xmlNewNode(NULL, (const xmlChar*)"SUBJECT");
    xmlNodePtr keywordsNode = xmlNewNode(NULL, (const xmlChar*)"KEYWORDS");
    xmlNodePtr authorNode = xmlNewNode(NULL, (const xmlChar*)"AUTHOR");
    xmlNodePtr creatorNode = xmlNewNode(NULL, (const xmlChar*)"CREATOR");
    xmlNodePtr producerNode = xmlNewNode(NULL, (const xmlChar*)"PRODUCER");
    xmlNodePtr creationDateNode = xmlNewNode(NULL, (const xmlChar*)"CREATIONDATE");
    xmlNodePtr modDateNode = xmlNewNode(NULL, (const xmlChar*)"MODIFICATIONDATE");


    titleNode->type = XML_ELEMENT_NODE;
    subjectNode->type = XML_ELEMENT_NODE;
    keywordsNode->type = XML_ELEMENT_NODE;
    authorNode->type = XML_ELEMENT_NODE;
    creatorNode->type = XML_ELEMENT_NODE;
    producerNode->type = XML_ELEMENT_NODE;
    creationDateNode->type = XML_ELEMENT_NODE;
    modDateNode->type = XML_ELEMENT_NODE;

    xmlAddChild(nodeSourceImageInfo, titleNode);
    xmlAddChild(nodeSourceImageInfo, subjectNode);
    xmlAddChild(nodeSourceImageInfo, keywordsNode);
    xmlAddChild(nodeSourceImageInfo, authorNode);
    xmlAddChild(nodeSourceImageInfo, creatorNode);
    xmlAddChild(nodeSourceImageInfo, producerNode);
    xmlAddChild(nodeSourceImageInfo, creationDateNode);
    xmlAddChild(nodeSourceImageInfo, modDateNode);

//	xmlNewProp(nodeVersion, (const xmlChar*)ATTR_VALUE,(const xmlChar*)PDFTOXML_VERSION);
//
//	infoNode =
    pdfdocxrce->getDocInfo(&info);
    if (info.isDict()) {

        content = getInfoString(info.getDict(), "Title");
        xmlNodeSetContent(titleNode, (const xmlChar*)xmlEncodeEntitiesReentrant(
                titleNode->doc, (const xmlChar*)content->getCString()));

        content = getInfoString(info.getDict(), "Subject");
        xmlNodeSetContent(subjectNode, (const xmlChar*)xmlEncodeEntitiesReentrant(
                subjectNode->doc, (const xmlChar*)content->getCString()));

        content = getInfoString(info.getDict(), "Keywords");
        xmlNodeSetContent(keywordsNode, (const xmlChar*)xmlEncodeEntitiesReentrant(
                keywordsNode->doc, (const xmlChar*)content->getCString()));

        content = getInfoString(info.getDict(), "Author");
        xmlNodeSetContent(authorNode, (const xmlChar*)xmlEncodeEntitiesReentrant(
                authorNode->doc, (const xmlChar*)content->getCString()));

        content = getInfoString(info.getDict(), "Creator");
        xmlNodeSetContent(creatorNode, (const xmlChar*)xmlEncodeEntitiesReentrant(
                creatorNode->doc, (const xmlChar*)content->getCString()));

        content = getInfoString(info.getDict(), "Producer");
        xmlNodeSetContent(producerNode, (const xmlChar*)xmlEncodeEntitiesReentrant(
                producerNode->doc, (const xmlChar*)content->getCString()));

        content = getInfoDate(info.getDict(), "CreationDate");
        xmlNodeSetContent(creationDateNode, (const xmlChar*)xmlEncodeEntitiesReentrant(
                creationDateNode->doc, (const xmlChar*)content->getCString()));

        content = getInfoDate(info.getDict(), "ModDate");
        xmlNodeSetContent(modDateNode, (const xmlChar*)xmlEncodeEntitiesReentrant(
                modDateNode->doc, (const xmlChar*)content->getCString()));
    }
    info.free();
}


xmlNodePtr XmlAltoOutputDev::findNodeByName(xmlNodePtr rootnode, const xmlChar * nodename)
{
    xmlNodePtr node = rootnode;
    if(node == NULL){
        printf("Document is empty!");
        return NULL;
    }

    while(node != NULL){

        if(!xmlStrcmp(node->name, nodename)){
            return node;
        }
        else if(node->children != NULL){
            node = node->children;
            xmlNodePtr intNode =  findNodeByName(node, nodename);
            if(intNode != NULL){
                return intNode;
            }
        }
        node = node->next;
    }
    return NULL;
}


GString* XmlAltoOutputDev::getInfoString(Dict *infoDict, const char *key) {
    UnicodeMap *uMap = globalParams->getTextEncoding();
    Object obj;
    GString *s = new GString();
    GString *s1 = new GString();
    GBool isUnicode;
    Unicode u;
    char buf[8];
    int i, n;

    if (infoDict->lookup(key, &obj)->isString()) {
        //fputs(text, stdout);
        s1 = obj.getString();
        if ((s1->getChar(0) & 0xff) == 0xfe &&
            (s1->getChar(1) & 0xff) == 0xff) {
            isUnicode = gTrue;
            i = 2;
        } else {
            isUnicode = gFalse;
            i = 0;
        }
        while (i < obj.getString()->getLength()) {
            if (isUnicode) {
                u = ((s1->getChar(i) & 0xff) << 8) |
                    (s1->getChar(i+1) & 0xff);
                i += 2;
            } else {
                u = pdfDocEncoding[s1->getChar(i) & 0xff];
                ++i;
            }
            n = uMap->mapUnicode(u, buf, sizeof(buf));
            //fwrite(buf, 1, n, stdout);
            s->append(buf,n);
        }
        //fputc('\n', stdout);
    }
    obj.free();
    return s;
}

GString* XmlAltoOutputDev::getInfoDate(Dict *infoDict, const char *key) {
    Object obj;
    char *s;
    int year, mon, day, hour, min, sec, n;
    struct tm tmStruct;
    char buf[256];
    GString *s1 = new GString();

    if (infoDict->lookup(key, &obj)->isString()) {
        s = obj.getString()->getCString();
        if (s[0] == 'D' && s[1] == ':') {
            s += 2;
        }
        if ((n = sscanf(s, "%4d%2d%2d%2d%2d%2d",
                        &year, &mon, &day, &hour, &min, &sec)) >= 1) {
            switch (n) {
                case 1: mon = 1;
                case 2: day = 1;
                case 3: hour = 0;
                case 4: min = 0;
                case 5: sec = 0;
            }
            tmStruct.tm_year = year - 1900;
            tmStruct.tm_mon = mon - 1;
            tmStruct.tm_mday = day;
            tmStruct.tm_hour = hour;
            tmStruct.tm_min = min;
            tmStruct.tm_sec = sec;
            tmStruct.tm_wday = -1;
            tmStruct.tm_yday = -1;
            tmStruct.tm_isdst = -1;
            // compute the tm_wday and tm_yday fields
            if (mktime(&tmStruct) != (time_t)-1 &&
                strftime(buf, sizeof(buf), "%c", &tmStruct)) {
                s1->append(buf);
            } else {
                s1->append(s);
            }
        } else {
            s1->append(s);
        }
        //fputc('\n', stdout);
    }
    obj.free();
    return s1;
}



GString* XmlAltoOutputDev::toUnicode(GString *s,UnicodeMap *uMap){

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

void XmlAltoOutputDev::startPage(int pageNum, GfxState *state) {
    //curstate = (double*)malloc(6*sizeof(double));
    for (int i=0;i<6;++i){
        curstate[i]=state->getCTM()[i];
    }
    if (parameters->getCutAllPages() == 1) {
        text->startPage(pageNum, state, gFalse);

    }
    if (parameters->getCutAllPages() == 0) {
        text->startPage(pageNum, state, gTrue);
    }

}


void XmlAltoOutputDev::endPage() {

    printf("endPage");
    text->configuration();
    if (parameters->getDisplayText()) {
        text->dump(blocks, fullFontName);
    }

    text->endPage(dataDir);

}

void XmlAltoOutputDev::updateFont(GfxState *state) {
    text->updateFont(state);
}

void XmlAltoOutputDev::drawChar(GfxState *state, double x, double y, double dx,
                            double dy, double originX, double originY, CharCode c, int nBytes,
                            Unicode *u, int uLen) {
    if (uLen ==0) {
        uLen=1;
    }
    text->addChar(state, x, y, dx, dy, c, nBytes, u, uLen);
}

void XmlAltoOutputDev::stroke(GfxState *state) {
    GString * attr = new GString();
    char tmp[100];
    GfxRGB rgb;

    // The stroke attribute : the stroke color value
    state->getStrokeRGB(&rgb);
    GString * hexColor = colortoString(rgb);
    sprintf(tmp, "stroke: %s;", hexColor->getCString() );
    attr->append(tmp);
    delete hexColor;

    // The stroke-opacity attribute
    double fo = state->getStrokeOpacity();
    if (fo != 1) {
        sprintf(tmp, "stroke-opacity: %g;", fo);
        attr->append(tmp);
    }

    // The stroke-dasharray attribute : line dasharray information
    // We use the transformWidth function to adjust the values with the CTM value
    double *dash;
    int length;
    int i;
    double start;
    state->getLineDash(&dash, &length, &start);
    // IF there is information about line dash
    if (length != 0) {
        attr->append("stroke-dasharray:");
        for (i=0; i<length; i++) {
            sprintf(tmp, "%g", state->transformWidth(dash[i]) == 0 ? 1
                                                                   : state->transformWidth(dash[i]));
            attr->append(tmp);
            sprintf(tmp, "%s", (i == length-1) ? "" : ", ");
            attr->append(tmp);
        }
        attr->append(";");
    }

    // The fill attribute : none value
    attr->append("fill:none;");

    // The stroke-width attribute
    // Change the line width value with the CTM value
    double lineWidth1 = state->getLineWidth();
    state->setLineWidth(state->getTransformedLineWidth());
    double lineWidth2 = state->getLineWidth();
    if (lineWidth1 != lineWidth2) {
        lineWidth2 = lineWidth2 + 0.5;
    }
    sprintf(tmp, "stroke-width: %g;", lineWidth2);
    attr->append(tmp);

    // The stroke-linejoin attribute
    int lineJoin = state->getLineJoin();
    switch (lineJoin) {
        case 0:
            attr->append("stroke-linejoin:miter;");
            break;
        case 1:
            attr->append("stroke-linejoin:round;");
            break;
        case 2:
            attr->append("stroke-linejoin:bevel;");
            break;
    }

    // The stroke-linecap attribute
    int lineCap = state->getLineCap();
    switch (lineCap) {
        case 0:
            attr->append("stroke-linecap:butt;");
            break;
        case 1:
            attr->append("stroke-linecap:round;");
            break;
        case 2:
            attr->append("stroke-linecap:square;");
            break;
    }

    // The stroke-miterlimit attribute
    double miter = state->getMiterLimit();
    if (miter != 4) {
        sprintf(tmp, "stroke-miterlimit: %g;", miter);
    }
    attr->append(tmp);

    doPath(state->getPath(), state, attr);
}

void XmlAltoOutputDev::fill(GfxState *state) {
    GString * attr = new GString();
    char tmp[100];
    GfxRGB rgb;

    // The fill attribute which give color value
    state->getFillRGB(&rgb);
    GString * hexColor = colortoString(rgb);
    sprintf(tmp, "fill: %s;", hexColor->getCString() );
    attr->append(tmp);
    delete hexColor;

    // The fill-opacity attribute
    double fo = state->getFillOpacity();
    sprintf(tmp, "fill-opacity: %g;", fo);
    attr->append(tmp);

    doPath(state->getPath(), state, attr);
}

void XmlAltoOutputDev::eoFill(GfxState *state) {
    GString * attr = new GString();
    char tmp[100];
    GfxRGB rgb;

    // The fill attribute which give color value
    state->getFillRGB(&rgb);
    GString * hexColor = colortoString(rgb);
    sprintf(tmp, "fill: %s;", hexColor->getCString() );
    attr->append(tmp);
    delete hexColor;

    // The fill-rule attribute with evenodd value
    attr->append("fill-rule: evenodd;");

    // The fill-opacity attribute
    double fo = state->getFillOpacity();
    sprintf(tmp, "fill-opacity: %g;", fo);
    attr->append(tmp);

    doPath(state->getPath(), state, attr);
}

void XmlAltoOutputDev::clip(GfxState *state) {
    if (parameters->getDisplayImage()) {
        text->clip(state);
    }
}

void XmlAltoOutputDev::eoClip(GfxState *state) {
    text->eoClip(state);
}
void XmlAltoOutputDev::clipToStrokePath(GfxState *state) {
    text->clipToStrokePath(state);
}

void XmlAltoOutputDev::doPath(GfxPath *path, GfxState *state, GString *gattributes) {
    if (parameters->getDisplayImage()) {
        text->doPath(path, state, gattributes);
    }
}

void XmlAltoOutputDev::saveState(GfxState *state) {
    text->saveState(state);
}

void XmlAltoOutputDev::restoreState(GfxState *state) {
    text->restoreState(state);
}

// Return the hexadecimal value of the color of string
GString *XmlAltoOutputDev::colortoString(GfxRGB rgb) const {
    char* temp;
    temp = (char*)malloc(10*sizeof(char));
    sprintf(temp, "#%02X%02X%02X", static_cast<int>(255*colToDbl(rgb.r)),
            static_cast<int>(255*colToDbl(rgb.g)), static_cast<int>(255
                                                                    *colToDbl(rgb.b)));
    GString *tmp = new GString(temp);

    free(temp);

    return tmp;
}

GString *XmlAltoOutputDev::convtoX(unsigned int xcol) const {
    GString *xret=new GString();
    char tmp;
    unsigned int k;
    k = (xcol/16);
    if ((k>=0)&&(k<10))
        tmp=(char) ('0'+k);
    else
        tmp=(char)('a'+k-10);
    xret->append(tmp);
    k = (xcol%16);
    if ((k>=0)&&(k<10))
        tmp=(char) ('0'+k);
    else
        tmp=(char)('a'+k-10);
    xret->append(tmp);
    return xret;
}




void XmlAltoOutputDev::drawImage(GfxState *state, Object *ref, Stream *str,
                             int width, int height, GfxImageColorMap *colorMap, int *maskColors,
                             GBool inlineImg) {

    const char* ext;

    int index=-1;
    if (parameters->getDisplayImage()) {
        // test if already processed

        // register the block in the block structure of the page
        double x1, y1, x2, y2, temp;
        bool flip_x = false;
        bool flip_y = false;
        int flip = 0;
        // when drawing a picture, we are in the scaled coordinates of the picture
        // in which the top-left corner is at coordinates (0,1) and
        // and  the bottom-right corner is at coordinates (1,0).
        state->transform(0, 1, &x1, &y1);
        state->transform(1, 0, &x2, &y2);

        // Detect if the picture is printed flipped
        if (x1 > x2)
        {
            flip |= 1;
            flip_x = true;
            temp = x1;
            x1 = x2;
            x2 = temp;
        }

        if (y1 > y2)
        {
            flip |= 2;
            flip_y = true;
            temp = y1;
            y1 = y2;
            y2 = temp;
        }
        int reference = -1;
        if ((ref != NULL) && (ref->isRef()))
        {
            reference = ref->getRefNum();

            for (int i = 0; i < lPictureReferences->getLength(); i++)
            {
                PictureReference* pic_reference = (PictureReference*) lPictureReferences->get(i);
                if (   (pic_reference->reference_number == reference)
                       && (pic_reference->picture_flip == flip))
                {
                    index = pic_reference->picture_number;
                    // We already created a file for this picture
//					printf("IMAGE ALREADY SEEN flip=%d\t%d %d\n",flip,reference,index);
//					compose_image_filename(dev_picture_base,
//										   pic_reference->picture_number,
//										   pic_reference->picture_extension,
//										   pic_file);
                    break;
                }
            }
        }
        //new image
        if (index == -1)
        {
            //HD : in order to avoid millions of small (pixel) images
            if (height > 8 && width > 8 && imageIndex <1000){
                imageIndex+=1;
                // Save this in the references
                //			text->drawImage(state, ref, str, width, height, colorMap, maskColors,inlineImg, dumpJPEG, imageIndex);
                ext= text->drawImageOrMask(state, ref, str, width, height, colorMap, maskColors, inlineImg, false,imageIndex); // not a mask
                lPictureReferences->append(new PictureReference(reference, flip, imageIndex, ext));


            }

        }
        else{
            //HD : in order to avoid millions of small (pixel) images
            if (height > 8 && width > 8 && imageIndex <1000){
                //			text->drawImage(state, ref, str, width, height, colorMap, maskColors,inlineImg, dumpJPEG, imageIndex);
                ext= text->drawImageOrMask(state, ref, str, width, height, colorMap, maskColors, inlineImg, false,index); // not a mask

            }
        }
    }
}

void XmlAltoOutputDev::drawImageMask(GfxState *state, Object *ref, Stream *str,
                                 int width, int height, GBool invert, GBool inlineImg) {


    const char* ext;
    // register the block in the block structure of the page
    double x1, y1, x2, y2, temp;
    bool flip_x = false;
    bool flip_y = false;
    int flip = 0;

    int index = -1;
    // when drawing a picture, we are in the scaled coordinates of the picture
    // in which the top-left corner is at coordinates (0,1) and
    // and  the bottom-right corner is at coordinates (1,0).
    state->transform(0, 1, &x1, &y1);
    state->transform(1, 0, &x2, &y2);

    // Detect if the picture is printed flipped
    if (x1 > x2)
    {
        flip |= 1;
        flip_x = true;
        temp = x1;
        x1 = x2;
        x2 = temp;
    }

    if (y1 > y2)
    {
        flip |= 2;
        flip_y = true;
        temp = y1;
        y1 = y2;
        y2 = temp;
    }
    int reference = -1;
    if ((ref != NULL) && (ref->isRef()))
    {
        reference = ref->getRefNum();

        for (int i = 0; i < lPictureReferences->getLength(); i++)
        {
            PictureReference* pic_reference = (PictureReference*) lPictureReferences->get(i);

            if (   (pic_reference->reference_number == reference)
                   && (pic_reference->picture_flip == flip))
            {
                // We already created a file for this picture
                index = pic_reference->picture_number;
//				printf("MASK IMAGE ALREADY SEEN flip=%d\t%d %d\n",flip,reference,index);
                break;
            }
        }
    }
    //new image
    if (index == -1)
    {
        if (parameters->getDisplayImage()) {
            //HD : in order to avoid millions of small (pixel) images
            if (height>8 && width > 8 && imageIndex<1000){
                imageIndex +=1;
                // Save this in the references
                ext= text->drawImageOrMask(state, ref, str, width, height, NULL, NULL, inlineImg, true,imageIndex); // mask
                lPictureReferences->append(new PictureReference(reference, flip, imageIndex, ext));
                //			text->drawImageMask(state, ref, str, width, height, invert, inlineImg,
                //				dumpJPEG, imageIndex);
            }
        }
    }
    else{
        if (parameters->getDisplayImage()) {
            //HD : in order to avoid millions of small (pixel) images
            if (height>8 && width > 8  && imageIndex<1000){
                //use reference instead
                ext= text->drawImageOrMask(state, ref, str, width, height, NULL, NULL, inlineImg, true,index); // mask
                //			text->drawImageMask(state, ref, str, width, height, invert, inlineImg,
                //				dumpJPEG, imageIndex);
            }
        }
    }
}

void XmlAltoOutputDev::initOutline(int nbPage) {
    char* tmp = (char*)malloc(10*sizeof(char));
    docOutline = xmlNewDoc((const xmlChar*)VERSION);
    globalParams->setTextEncoding((char*)ENCODING_UTF8);
    docOutlineRoot = xmlNewNode(NULL, (const xmlChar*)TAG_TOCITEMS);
    sprintf(tmp, "%d", nbPage);
    xmlNewProp(docOutlineRoot, (const xmlChar*)ATTR_NB_PAGES,
               (const xmlChar*)tmp);
    xmlDocSetRootElement(docOutline, docOutlineRoot);
}

void XmlAltoOutputDev::generateOutline(GList *itemsA, PDFDoc *docA, int levelA) {
    UnicodeMap *uMap;
    GString *enc;
    idItemToc = 0;
    if (itemsA && itemsA->getLength() > 0) {
        enc = new GString("UTF-8");
        uMap = globalParams->getUnicodeMap(enc);
        delete enc;
        dumpOutline(docOutlineRoot,itemsA, docA, uMap, levelA, idItemToc);
        uMap->decRefCnt();
    }
}
int XmlAltoOutputDev::dumpFragment(Unicode *text, int len, UnicodeMap *uMap,
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
GBool XmlAltoOutputDev::dumpOutline(xmlNodePtr parentNode,GList *itemsA, PDFDoc *docA, UnicodeMap *uMapA,
                                int levelA, int idItemTocParentA) {

    // store them in a list

    xmlNodePtr nodeTocItem = NULL;
    xmlNodePtr nodeItem = NULL;
    xmlNodePtr nodeString = NULL;
    xmlNodePtr nodeLink = NULL;
    double x2,y2;

    GBool atLeastOne= gFalse;

    char* tmp = (char*)malloc(10*sizeof(char));

    //    UnicodeMap *uMap;

    // Get the output encoding
    if (!(uMapA = globalParams->getTextEncoding())) {
        return 0;
    }
    int i;
    GString *title;

    nodeTocItem = xmlNewNode(NULL, (const xmlChar*)TAG_TOCITEMLIST);
    sprintf(tmp, "%d", levelA);
    xmlNewProp(nodeTocItem, (const xmlChar*)ATTR_LEVEL, (const xmlChar*)tmp);

    if (levelA != 0) {
        sprintf(tmp, "%d", idItemTocParentA);
        xmlNewProp(nodeTocItem, (const xmlChar*)ATTR_ID_ITEM_PARENT,
                   (const xmlChar*)tmp);
    }
    //xmlAddChild(docOutlineRoot, nodeTocItem);
    xmlAddChild(parentNode, nodeTocItem);

    for (i = 0; i < itemsA->getLength(); ++i) {

        title = new GString();

        ((OutlineItem *)itemsA->get(i))->open(); // open the kids
        dumpFragment(((OutlineItem *)itemsA->get(i))->getTitle(), ((OutlineItem *)itemsA->get(i))->getTitleLength(), uMapA, title);
        //printf("%s\n",title->getCString());
        LinkActionKind kind;

        LinkDest *dest;
        GString *namedDest;

        GString *fileName;
        int page = 0;

        double left = 0;
        double top = 0;
        double right = 0;
        double bottom = 0;
        x2 = 0;
        y2 = 0;

        if (((OutlineItem *)itemsA->get(i))->getAction()) {

            switch (kind = ((OutlineItem *)itemsA->get(i))->getAction()->getKind()) {

                // GOTO action
                case actionGoTo:
                    dest = NULL;
                    namedDest = NULL;

                    if ((dest = ((LinkGoTo *)((OutlineItem *)itemsA->get(i))->getAction())->getDest())) {
                        dest = dest->copy();
                    } else if ((namedDest = ((LinkGoTo *)((OutlineItem *)itemsA->get(i))->getAction())->getNamedDest())) {
                        namedDest = namedDest->copy();
                    }
                    if (namedDest) {
                        dest = docA->findDest(namedDest);
                    }

                    if (dest) {
                        if (dest->isPageRef()) {
                            Ref pageref = dest->getPageRef();
                            page=docA->getCatalog()->findPage(pageref.num, pageref.gen);
                        } else {
                            page = dest->getPageNum();
                        }
                    }


                    left = dest->getLeft();
                    top = dest->getTop();
                    x2= left;
                    y2=top;
                    //printf("%g %g %g %g %g %g\n",curstate[0],curstate[1],curstate[2],curstate[3],curstate[4],curstate[5]);
                    x2 = curstate[0] * left +  curstate[2] * top +  curstate[4];
                    y2 = curstate[1] * left +  curstate[3] * top +  curstate[5];

                    //printf("%g %g \n",x2,y2);
                    bottom = dest->getBottom();
                    right = dest->getRight();

                    if (dest) {
                        delete dest;
                    }
                    if (namedDest) {
                        delete namedDest;
                    }
                    break;


                    // LAUNCH action
                case actionLaunch:
                    fileName = ((LinkLaunch *)((OutlineItem *)itemsA->get(i))->getAction())->getFileName();
                    delete fileName;
                    break;

                    // URI action
                case actionURI:
                    break;

                    // NAMED action
                case actionNamed:
                    break;

                    // MOVIE action
                case actionMovie:
                    break;

                    // UNKNOWN action
                case actionUnknown:
                    break;
            } // end SWITCH
        } // end IF

        // ITEM node
        nodeItem = xmlNewNode(NULL, (const xmlChar*)TAG_ITEM);
        nodeItem->type = XML_ELEMENT_NODE;
        sprintf(tmp, "%d", idItemToc);
        xmlNewProp(nodeItem, (const xmlChar*)ATTR_ID, (const xmlChar*)tmp);
        xmlAddChild(nodeTocItem, nodeItem);

        // STRING node
        nodeString = xmlNewNode(NULL, (const xmlChar*)TAG_STRING);
        nodeString->type = XML_ELEMENT_NODE;
        //   	  	xmlNodeSetContent(nodeString,(const xmlChar*)xmlEncodeEntitiesReentrant(nodeString->doc,(const xmlChar*)title->getCString()));
        xmlNodeSetContent(nodeString,
                          (const xmlChar*)xmlEncodeEntitiesReentrant(nodeString->doc,
                                                                     (const xmlChar*)title->getCString()));

        xmlAddChild(nodeItem, nodeString);

        // LINK node
        nodeLink = xmlNewNode(NULL, (const xmlChar*)TAG_LINK);
        nodeLink->type = XML_ELEMENT_NODE;

        sprintf(tmp, "%d", page);
        xmlNewProp(nodeLink, (const xmlChar*)ATTR_PAGE, (const xmlChar*)tmp);
        sprintf(tmp, "%g", y2);
        xmlNewProp(nodeLink, (const xmlChar*)ATTR_TOP, (const xmlChar*)tmp);
        sprintf(tmp, "%g", bottom);
        xmlNewProp(nodeLink, (const xmlChar*)ATTR_BOTTOM, (const xmlChar*)tmp);
        sprintf(tmp, "%g", x2);
        xmlNewProp(nodeLink, (const xmlChar*)ATTR_LEFT, (const xmlChar*)tmp);
        sprintf(tmp, "%g", right);
        xmlNewProp(nodeLink, (const xmlChar*)ATTR_RIGHT, (const xmlChar*)tmp);

        xmlAddChild(nodeItem, nodeLink);
        int idItemCurrent = idItemToc;
        idItemToc++;
        if (((OutlineItem *)itemsA->get(i))->hasKids()) {
            dumpOutline(nodeItem,((OutlineItem *)itemsA->get(i))->getKids(), docA, uMapA, levelA+1,
                        idItemCurrent);
        }

        delete title;
        ((OutlineItem *)itemsA->get(i))->close(); // close the kids

        atLeastOne = gTrue;
    } // end FOR

    return atLeastOne;
}


void XmlAltoOutputDev::closeOutline(GString *shortFileName) {
    shortFileName->append("_");
    shortFileName->append(NAME_OUTLINE);
    shortFileName->append(EXTENSION_XML);
    xmlSaveFile(shortFileName->getCString(), docOutline);
    xmlFreeDoc(docOutline);
}
