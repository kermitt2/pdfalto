//
// Created by azhar on 20/12/2017.
//

#ifndef PDF2XML_XMLALTOOUTPUTDEV_H
#define PDF2XML_XMLALTOOUTPUTDEV_H

#include <aconf.h>

#ifdef USE_GCC_PRAGMAS
#pragma interface
#endif

#include <stdio.h>
#include "gtypes.h"
#include "GfxFont.h"

#include <stddef.h>
#include "config.h"
#include "Object.h"
#include "GlobalParams.h"
#include "OutputDev.h"
#include "PDFDoc.h"
#include "GfxState.h"
#include "Link.h"
#include "Parameters.h"
#include "Catalog.h"
#include "AnnotsXrce.h"

// PNG lib
#include "png.h"


using namespace std;
#include <list>
#include <stack>
#include <vector>
#include <string>
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>

#include "PDFDocXrce.h"

#define xoutRound(x) ((int)(x + 0.5))

class GfxPath;
class GfxFont;
class GfxColorSpace;
class GfxSeparationColorSpace;
class PDFRectangle;
struct PSFont16Enc;
class PSOutCustomColor;

class GString;
class GList;
class GfxState;
class UnicodeMap;

class XmlAltoOutputDev;

//------------------------------------------------------------------------

typedef void (*TextOutputFunc)(void *stream, char *text, int len);

//------------------------------------------------------------------------
// TextFontInfo
//------------------------------------------------------------------------
/**
 * TextFontInfo class (based on TextOutputDev.h, Copyright 1997-2003 Glyph & Cog, LLC)<br></br>
 * Xerox Research Centre Europe <br></br>
 * @date 04-2006
 * @author Herv� D�jean
 * @author Sophie Andrieu
 * @version xpdf 3.01
 */

class TextFontInfo {
public:

    TextFontInfo(GfxState *state);
    ~TextFontInfo();

    GBool matches(GfxState *state);

//#if TEXTOUT_WORD_LIST
    // Get the font name (which may be NULL).
    GString *getFontName() { return fontName; }

    // Get font descriptor flags.
    GBool isFixedWidth() { return flags & fontFixedWidth; }
    GBool isSerif() { return flags & fontSerif; }
    GBool isSymbolic() { return flags & fontSymbolic; }
    GBool isItalic() { return flags & fontItalic; }
    GBool isBold() { return flags & fontBold; }
//#endif

private:

    GfxFont *gfxFont;
//#if TEXTOUT_WORD_LIST
    GString *fontName;
    int flags;
//#endif

    friend class TextWord;
    friend class TextPage;
};



class TextFontStyleInfo {
public:

    ~TextFontStyleInfo();

    // Get the font name (which may be NULL).
    int getId() const { return id; }
    void setId(int fontId){id = fontId;}

    GString* getFontName() const { return fontName; }
    GString* getFontNameCS() { return fontName; }
    void setFontName(GString* fontname){fontName = fontname;}

    double getFontSize() const { return fontSize; }
    void setFontSize(double fontsize){fontSize = fontsize;}

    GString* getFontColor() const { return fontColor; }
    void setFontColor(GString* fontcolor){fontColor = fontcolor;}

    GBool getFontType() const { return fontType; }
    void setFontType(GBool fonttype){fontType = fonttype;}

    GBool getFontStyle() const { return fontStyle; }
    void setFontStyle(GBool fontstyle){fontStyle = fontstyle;}

    // Compare two text fonts:  -1:<  0:=  +1:>
    GBool cmp(TextFontStyleInfo *tsi);
//#endif

private:

    int id;
    GString* fontName;
    double fontSize;
    GString* fontColor;
    GBool fontType; //Enumeration : serif (gTrue) or sans-serif(gFalse)
    GBool fontStyle; //Enumeration : proportional(gFalse) or fixed(gTrue)
//#endif

    friend class TextFontInfo;
};


//class TextFontInfo {
//public:
//
//  /** Construct a new <code>TextFontInfo</code>
//   * @param state The state
//   */
//  TextFontInfo(GfxState *state);
//
//  /** Destructor
//   */
//  ~TextFontInfo();
//
//  /** Match the state font with the current font
//   * @param state The state description */
//  GBool matches(GfxState *state);
//
//private:
//
//  /** The font */
//  GfxFont *gfxFont;
//
//#if TEXTOUT_WORD_LIST
//  GString *fontName;
//#endif
//
//  friend class TextWord;
//  friend class TextPage;
//};

//------------------------------------------------------------------------
// ImageInline
//------------------------------------------------------------------------
/**
 * ImageInline class <br></br>
 * Xerox Research Centre Europe <br></br>
 * @date 04-2006
 * @author Sophie Andrieu
 * @version xpdf 3.01
 */
class ImageInline {
public:

    /** Construct a new <code>ImageInline</code><br></br>
     * An <code>ImageInline</code> is an image which is localized in the stream and it is composed<br></br>
     * by a x position, a y position, a width, a height and href which get the path of this image.<br></br>
     * The two variables idWord and idImage are used in the lines building to include images.
     * @param xPosition The x position of this image
     * @param yPosition The y position of this image
     * @param width The width value of this image
     * @param height The height value of this image
     * @param idWord The id of word which precede this image
     * @param idImage The id of current image
     * @param href The image href
     * @param idx The absolute object index in the stream */
    ImageInline(double xPosition, double yPosition, double width, double height, int idWord, int idImage, GString *href, int index);

    /** Destructor
     */
    ~ImageInline();

    /** Get the absolute object index in the stream
     * @return The absolute object index in the stream */
    int getIdx(){return idx;};
    /** Get the x position of the image
     *  @return The x position of the image */
    double getXPositionImage(){return xPositionImage;}
    /** Get the y position of the image
     *  @return The y position of the image */
    double getYPositionImage(){return yPositionImage;}
    /** Get the width value of the image
     *  @return The width value of the image */
    double getWidthImage(){return widthImage;}
    /** Get the height value of the image
     *  @return The height value of the image */
    double getHeightImage(){return heightImage;}
    /** Get the id of the word which precede this image
     *  @return The id of the word which precede this image */
    int getIdWordBefore(){return idWordBefore;}
    /** Get the id of this image
     *  @return The id of this image */
    int getIdImageCurrent(){return idImageCurrent;}
    /** Get the href of this image
     *  @return The href of this image */
    GString* getHrefImage(){return hrefImage;}

    /** Modify the x position of the image
     *  @param xPosition The new x position image value */
    void setXPositionImage(double xPosition){xPositionImage = xPosition;}
    /** Modify the y position of the image
     *  @param yPosition The new y position image value */
    void setYPositionImage(double yPosition){yPositionImage = yPosition;}
    /** Modify the width value of the image
     *  @param width The new width image value */
    void setWidthImage(double width){widthImage = width;}
    /** Modify the height value of the image
     *  @param height The new height image value */
    void setHeightImage(double height){heightImage = height;}
    /** Modify the id of the word which precede this image
     *  @param id The new id value of the word which precede this image */
    void setIdWordBefore(int id){idWordBefore = id;}
    /** Modify the id of this image
     *  @param id The new id value of this image */
    void setIdImageCurrent(int id){idImageCurrent = id;}
    /** Modify the href of this image
     *  @param href The new href value of this image */
    void setHrefImage(GString *href){hrefImage = href;}

private:

    /** The absolute object index in the stream **/
    int idx;
    /** The x position of the image */
    double xPositionImage;
    /** The y position of the image */
    double yPositionImage;
    /** The width value of the image */
    double widthImage;
    /** The height value of the image */
    double heightImage;
    /** The id word which precede this image */
    int idWordBefore;
    /** The id current image */
    int idImageCurrent;
    /** The path current image */
    GString* hrefImage;

    friend class TextWord;
    friend class TextPage;
};

//------------------------------------------------------------------------
// TextWord
//------------------------------------------------------------------------
/**
 * TextWord class (based on TextOutputDev.h, Copyright 1997-2003 Glyph & Cog, LLC)<br></br>
 * Xerox Research Centre Europe <br></br>
 * @date 04-2006
 * @author Herv� D�jean
 * @author Sophie Andrieu
 * @version xpdf 3.01 */

class TextWord {
public:

    /** Construct a new <code>TextWord</code>
     * @param state The state description
     * @param rotA The rotation value of the current word
     * @param angleDegre The angle value of the current word (value in degree)
     * @param angleSkewingY The skewing angle value for the Y axis of this word (value in degree)
     * @param angleSkewingX The skewing angle value for the X axis of this word (value in degree)
     * @param x0 The x value coordinate about the left bottom corner of the word box
     * @param y0 The y value coordinate about the left bottom corner of the word box
     * @param charPosA The position of the character
     * @param fontA The fonts informations about the current word
     * @param fontSize The font size about the current word
     * @param idWord The id of the word (used to include and localize the image inline in the stream)
     * @param idx The absolute object index in the stream */
    TextWord(GfxState *state, int rotA, int angleDegre, int angleSkewingY, int angleSkewingX, double x0, double y0,
             int charPosA, TextFontInfo *fontA, double fontSize, int idWord, int index);

    /** Destructor */
    ~TextWord();

    /** Get the absolute object index in the stream
     * @return The absolute object index in the stream */
    int getIdx(){return idx;};

    /** Add a character to the current word
     *  @param state The state description
     *  @param x The x value
     *  @param y The y value
     *  @param dx The dx value
     *  @param dy The dy value
     *  @param u The unicode char to add
     */
    void addChar(GfxState *state, double x, double y,
                 double dx, double dy, Unicode u);

    /** Merge <code>word</code> onto the end of <code>this</code>
     *  @param word The current word */
    void merge(TextWord *word);

    /** Compares <code>this</code> to <code>word</code>, returning -1 (<), 0 (=), or +1 (>),<br></br>
     *  based on a primary-axis comparison, e.g., x ordering if rot=0
     *  @param word The current word */
    int primaryCmp(TextWord *word);

    /** Get the distance along the primary axis between <code>this</code> and <code>word</code>
     *  @param word The current word */
    double primaryDelta(TextWord *word);

    /**
     * return True if overlap with w2
     */
    GBool overlap(TextWord *w2);

    static int cmpYX(const void *p1, const void *p2);

    /** Get the font name of the current word
     *  @return The font name of the current word */
    char *getFontName(){return fontName;}

    /** Check if the current <code>TextWord</code> is <b>bold</b>
     *  @return <code>true</code> if the current <code>TextWord</code> is <b>bold</b>, <code>false</code> else */
    GBool isBold() {return bold;}

    /** Check if the current <code>TextWord</code> is <b>italic</b>
     *  @return <code>true</code> if the current <code>TextWord</code> is <b>italic</b>, <code>false</code> else */
    GBool isItalic() {return italic;}

    /** Check if the current <code>TextWord</code> is <b>symbolic</b>
     *  @return <code>true</code> if the current <code>TextWord</code> is <b>symbolic</b>, <code>false</code> else */
    GBool isSymbloic() {return symbolic;}

    /** Get the color RGB of the word
     * @param r The Red color value
     * @param g The Green color value
     * @param b The Blue color value */
    void getColor(double *r, double *g, double *b)
    { *r = colorR; *g = colorG; *b = colorB; }

    GString *convtoX(double xcol) const;

    /** Convert the RGB color to string hexadecimal color value*/
    GString *colortoString() const;

    /** Normalize the font name :<br></br>
     *  - remove the prefix (if it is present) which is the basefont of subsetted fonts.
     * This prefix is composed of six characters (A-Z letters) and is followed by a +.<br></br>
     *  - remove the suffix which can indicated if font is italic, bold, normal... This
     * suffix is preceded by a coma or by a hyphen.<br/>
     * <b>CACHJA+OfficinaSerif-Bold</b> is normalized in <b>OfficinaSerif</b> <br></br>
     * <b>TimesNewRoman,Italic</b> is normalized in <b>TimesNewRoman</b>
     * @param fontName The current font name
     * @return The new font name which is normalized */
    const char* normalizeFontName(char* fontName);

#if TEXTOUT_WORD_LIST
    int getLength() { return len; }
  Unicode getChar(int idx) { return text[idx]; }

  GString *getFontName() { return font->fontName; }
  void getBBox(double *xMinA, double *yMinA, double *xMaxA, double *yMaxA)
    { *xMinA = xMin; *yMinA = yMin; *xMaxA = xMax; *yMaxA = yMax; }
  int getCharPos() { return charPos; }
  int getCharLen() { return charLen; }
#endif

private:

    /** The value of the character spacing */
    float charSpace;
    /** The value of the word spacing : Related to justification (space "inserted" in order to get the line justified) */
    float wordSpace;
    /** The value of the horizontal scaling */
    float horizScaling;
    /** The value of the rise*/
    float rise ;
    /** The value of the render */
    float render;
    /** The value of the leading */
    float leading;

    /** Rank in the original flow */
    int indexmin;
    int indexmax;

    /** The id of the word (used to include and localize the image inline in the stream) */
    int idWord;

    /** The absolute object index in the stream **/
    int idx;

    /** The rotation which is multiple of 90 degrees (0, 1, 2, or 3) */
    int rot;
    /** The angle value of rotation in degree (0 to 360 degree) */
    int angle;
    /** The y angle skewing value : the value can be positive or negative and it is function the angle position in the y axis */
    int angleSkewing_Y;
    /** The x angle skewing value : the value can be positive or negative and it is function the angle position in the x axis */
    int angleSkewing_X;

    /** The x value minimum coordinate of bounding box */
    double xMin;
    /** The x value maximum coordinate of bounding box */
    double xMax;
    /** The y value minimum coordinate of bounding box */
    double yMin;
    /** The y value maximum coordinate of bounding box */
    double yMax;
    /** The baseline x or y coordinate */
    double base;
    /** The yMin of the word function the rotation */
    double baseYmin;

    /** The unicode text */
    Unicode *text;
    /** "near" edge x or y coord of each char (plus one extra entry for the last char) */
    double *edge;
    /** The length of text and edge arrays */
    int len;
    /** The size of text and edge arrays */
    int size;
    /** The character position (within content stream) */
    int charPos;
    /** The number of content stream characters in this word */
    int charLen;

    /** <code>true</code> if the current <code>TextWord</code> is <b>bold</b>, <code>false</code> else */
    GBool  bold;
    /** <code>true</code> if the current <code>TextWord</code> is <b>italic</b>, <code>false</code> else */
    GBool  italic;
    /** <code>true</code> if the current <code>TextWord</code> is <b>symbolic</b>, <code>false</code> else */
    GBool  symbolic;
    /** <code>true</code> if the current <code>TextWord</code> is <b>serif</b>, <code>false</code> else */
    GBool  serif;

    /** The font information */
    TextFontInfo *font;
    /** The font name */
    char* fontName;
    /** The font size */
    double fontSize;

    /** To set if there is a space between this word and the next word on the line */
    GBool spaceAfter;
    /** The next word in line */
    TextWord *next;

    /** The value of word Red color */
    double colorR;
    /** The value of word Green color */
    double colorG;
    /** The value of word Blue color */
    double colorB;

    friend class TextPage;
};

//------------------------------------------------------------------------
// TextPage
//------------------------------------------------------------------------
/**
 * TextPage class (based on TextOutputDev.h, Copyright 1997-2003 Glyph & Cog, LLC)<br></br>
 * Xerox Research Centre Europe <br></br>
 * @date 04-2006
 * @author Herv� D�jean
 * @author Sophie Andrieu
 * @version xpdf 3.01
 */

class TextPage {
public:

    /** Construct a new <code>TextPage</code>
     * @param verboseA The value of the verbose option
     * @param node The root node
     * @param dir The directory which contain all data
     * @param base The directory name which contain all data with the prefix 'image' : <pdfFileName>.xml_data/image
     * @param nsURIA The namespace specified
     */
    TextPage(GBool verboseA,Catalog *catalog, xmlNodePtr node, GString* dir, GString *base, GString *nsURIA);

    /** Destructor */
    ~TextPage();

    /** Start a new page
     * @param pageNum The numero of the current page
     * @param state The state description
     * @param cut To know if the cutPages option is selected */
    void startPage(int pageNum, GfxState *state, GBool cut);

    /** End the current page
     * @param dataDir The name directory data */
    void endPage(GString *dataDir);

    /** Configuration for the <b>start</b> of a new page */
    void configuration();

    int getPageNumber() {return num;}

    /** Update the current font
     *  @param state The state description */
    void updateFont(GfxState *state);

    /** Begin a new word
     *  @param state The state description
     *  @param x0 The x value of left bottom corner of the box word
     *  @param y0 The y value of left bottom corner of the box word */
    void beginWord(GfxState *state, double x0, double y0);

    /** Add a character to the current word
     *  @param state The state description
     *  @param x The x value of left bottom corner of the box character
     *  @param y The y value of left bottom corner of the box character
     *  @param dx The dx value
     *  @param dy The dy value
     *  @param c The code character
     *  @param nBytes The number of bytes
     *  @param u The unicode character value
     *  @param uLen The lenght */
    void addChar(GfxState *state, double x, double y,
                 double dx, double dy,
                 CharCode c, int nBytes, Unicode *u, int uLen);

    /** End the current word, sorting it into the list of words */
    void endWord();

    /** Add a word, sorting it into the list of words
     *  @param word The current word */
    void addWord(TextWord *word);

    /** Dump contents of the current page
     * @param blocks To know if the blocks option is selected
     * @param fullFontName To know if the fullFontName option is selected */
    void dump(GBool blocks, GBool fullFontName);

    /** PL: insert a block in the page's list of block nodes according to the reading order
     * @param nodeblock the block node to be inserted
     * @param lastInserted boolean indicating if the previously added block node was inserted before
     * an existing node (value is true) or append top the list (value is false)
     * @return true the node is the inserted before an existing node, false if it is append to the list
     */
    GBool addBlockChildReadingOrder(xmlNodePtr nodeblock, GBool lastInserted);

    /** Add a specific TOKEN tag in the current line when we meet an image inline.
     * This TOKEN tag is empty and it has five attributes which are : x, y, width,
     * height and href which get the uri of image inline.
     * @param nodeline The TEXT node which the current line
     * @param nodeImageInline The TOKEN node which represents the image inline to add to the current line
     * @param tmp A variable to build attributes
     * @param word The word which has been read */
    void addImageInlineNode(xmlNodePtr nodeline, xmlNodePtr nodeImageInline, char* tmp, TextWord *word);

    /** Add attributes to TOKEN node when verbose option is selected
     * @param node The current TOKEN node
     * @param tmp A variable which store the string attributs values
     * @param word The current word */
    void addAttributsNodeVerbose(xmlNodePtr node, char* tmp, TextWord *word);

    /** Add all minimum attributes to TOKEN node
     * @param node The current TOKEN node
     * @param word The current word
     * @param xMaxi The x value maximum coordinate of the left bottom corner word box
     * @param yMaxi The y value maximum coordinate of the left bottom corner word box
     * @param yMinRot The y value minimum coordinate of the left bottom corner word box (used for rotation 1 and 3)
     * @param yMaxRot The y value maximum coordinate of the left bottom corner word box (used for rotation 1 and 3)
     * @param xMinRot The x value minimum coordinate of the left bottom corner word box (used for rotation 1 and 3)
     * @param xMaxRot The x value maximum coordinate of the left bottom corner word box (used for rotation 1 and 3) */
    void addAttributsNode(xmlNodePtr node, TextWord *word, double &xMaxi, double &yMaxi, double &yMinRot,double &yMaxRot, double &xMinRot, double &xMaxRot, TextFontStyleInfo *fontStyleInfo);

    /** Add the type attribute to TOKEN node for the reading order
     * @param node The current TOKEN node
     * @param tmp A variable which store the string attributs values
     * @param word The current word */
    void addAttributTypeReadingOrder(xmlNodePtr node, char* tmp, TextWord *word);

    /** Add the GROUP tag whithin the instructions vectorials node
     * @param path The path description
     * @param state The state description
     * @param gattributes All attributes for the <i>style</i> attribut*/
    void doPath(GfxPath *path, GfxState *state, GString *gattributes);

    /** Add the GROUP tag whithin the CLIP current node
     * @param path The path description
     * @param state The state description
     * @param currentNode The current CLIP node to add the GROUP tag */
    void doPathForClip(GfxPath *path, GfxState *state, xmlNodePtr currentNode);

    /** Add the M, L and C tag whithin the GROUP current node
     * @param path The path description
     * @param state The state description
     * @param groupNode The current GROUP node  */
    void createPath(GfxPath *path, GfxState *state, xmlNodePtr groupNode);

    /** Get the clipping box and add the CLIP tag whithin the instructions vectorials node.
     * In this case, the rule use is the even-odd.
     * @param state The state description */
    void eoClip(GfxState *state);
    void clipToStrokePath(GfxState *state);

    /** Get the clipping box and add the CLIP tag whithin the instructions vectorials node.
     * In this case, the rule use is the nonzero winding number.
     * @param state The state description */
    void clip(GfxState *state);

    /** Save graphics state
     * @param state The state description */
    void saveState(GfxState *state);

    /** Restore graphics state
     * @param state The state description */
    void restoreState(GfxState *state);


//  void addLink(int xMin, int yMin, int xMax, int yMax, Link *link);


    //from mobi
    const char* drawImageOrMask(GfxState *state, Object* ref, Stream *str,
                                int width, int height,
                                GfxImageColorMap *colorMap,
                                int* /* maskColors */, GBool inlineImg, GBool mask,int imageIndex);

    // utility function to save raw data to a png file using the ong lib
    bool save_png (GString* file_name,
                   unsigned int width, unsigned int height, unsigned int row_stride,
                   unsigned char* data,
                   unsigned char bpp = 24, unsigned char color_type = PNG_COLOR_TYPE_RGB,
                   png_color* palette = NULL, unsigned short color_count = 0);



    /** Draw the image
     * @param state The state description
     * @param ref The reference
     * @param str The stream
     * @param width The image width
     * @param height The image height
     * @param colorMap The image color map
     * @param maskColors The mask
     * @param inlineImg To know if the image is inline or not
     * @param dumpJPEG To know if the image is a JPEG format
     * @param imageIndex The numero of image */
    void drawImage(GfxState *state, Object *ref, Stream *str,
                   int width, int height, GfxImageColorMap *colorMap,
                   int *maskColors, GBool inlineImg,GBool dumpJPEG,int imageIndex) ;

    /** Draw the image mask
     * @param state The state description
     * @param ref The reference
     * @param str The stream
     * @param width The image width
     * @param height The image height
     * @param invert
     * @param inlineImg To know if the image is inline or not
     * @param dumpJPEG To know if the image is a JPEG format
     * @param imageIndex The numero of image */
    void drawImageMask(GfxState *state, Object *ref, Stream *str,
                       int width, int height, GBool invert,
                       GBool inlineImg, GBool dumpJPEG, int imageIndex);

    /** Get the id of the current word : this method is used for include the images inline in the stream
     * @return The id of the current word */
    int getIdWORD(){return idWORD;};

    /** Get the absolute object index in the stream
     * @return The absolute object index in the stream */
    int getIdx(){return idx;};

    vector<ImageInline*> listeImageInline;

    /** The list of all recognized font styles*/
    vector<TextFontStyleInfo*> fontStyles;

    // clamp to uint8
    static inline int clamp (int x)
    {
        if (x > 255) return 255;
        if (x < 0) return 0;
        return x;
    }

private:

    /** Clear all */
    void clear();

    /** Build the string : unicode to string
     * @param text The unicode text
     * @param len The lenght
     * @param uMap The map
     * @param s The string which is building
     * @return The number of characters */
    int dumpFragment(Unicode *text, int len, UnicodeMap *uMap, GString *s);

    /** Build the attribute <i>id</i> for the <i>IMAGE</i> tag as <b>pNumberOfPage_iNumberOfImage</b>
     * @param pageNum The numero of the current page
     * @param imageNum The IMAGE numero of the current page
     * @param id The variable which store the result to return
     * @return The id generated which is a string */
    GString* buildIdImage(int pageNum, int imageNum, GString *id);

    /** Build the attribute <i>id</i> for the <i>TEXT</i> tag as <b>pNumberOfPage_tNumberOfText</b>
     * @param pageNum The numero of the current page
     * @param textNum The TEXT numero of the current page
     * @param id The variable which store the result to return
     * @return The id generated which is a string */
    GString* buildIdText(int pageNum, int textNum, GString *id);

    /** Build the attribute <i>id</i> for the <i>TOKEN</i> tag as <b>pNumberOfPage_wNumberOfToken</b>
     * @param pageNum The numero of the current page
     * @param tokenNum The TOKEN numero of the current page
     * @param id The variable which store the result to return
     * @return The id generated which is a string */
    GString* buildIdToken(int pageNum, int tokenNum, GString *id);

    /** Build the attribute <i>sid</i> for the object tags as <b>pNumberOfPage_sSID</b>
     * @param pageNum The numero of the current page
     * @param sid The absolute object index in the current page
     * @param id The variable which store the result to return
     * @return The sid generated which is a string */
    GString* buildSID(int pageNum, int sid, GString *id);

    /** Build the attribute <i>id</i> for the <i>BLOCK</i> tag as <b>pNumberOfPage_bNumberOfBlock</b>
     * @param pageNum The numero of the current page
     * @param blockNum The BLOCK numero of the current page
     * @param id The variable which store the result to return
     * @return The id generated which is a string */
    GString* buildIdBlock(int pageNum, int blockNum, GString *id);

    /** Build the attribute <i>id</i> for the <i>CLIP</i> tag as <b>pNumberOfPage_cNumberOfClip</b>
     * @param pageNum The numero of the current page
     * @param clipZoneNum The CLIP numero of the current page
     * @param id The variable which store the result to return
     * @return The id generated which is a string */
    GString* buildIdClipZone(int pageNum, int clipZoneNum, GString *id);

    /**
     * test if an annotation overlaps this zone
     * if test add the annotation subtype as attribut
     */
    GBool testOverlap(double x11,double y11,double x12,double y12,double x21,double y21,double x22,double y22);
    GBool testAnnotatedText(double xMin,double yMin,double xMax,double yMax);
    void testLinkedText(xmlNodePtr node,double xMin,double yMin,double xMax,double yMax);



    /** The numero of the current <i>PAGE</i> */
    int num;
    /** The <i>IMAGE</i> numero in the current page */
    int numImage;
    /** The <i>TEXT</i> numero in the current page */
    int numText;
    /** The <i>TOKEN</i> numero in the current page */
    int numToken;
    /** The <i>BLOCK</i> numero in the current page */
    int numBlock;

    /** The absolute object index in the stream */
    int idx;
    Catalog *myCat;
    GfxState *curstate;
    /** The id current word */
    int idWORD;
    /** The image indice */
    int indiceImage;
    /** The id of word just before the image inline inclusion */
    int idWORDBefore;
    /** The id of current image inline*/
    int idImageInline;
    /** The namespace specified */
    GString *namespaceURI;

    /** The root element */
    xmlNodePtr root;

    /** The XML document for page */
    xmlDocPtr docPage;
    /** The page element */
    xmlNodePtr page;

    /** The printSpace element */
    xmlNodePtr printSpace;

    /** The XML document for each page generated */
    xmlDocPtr pageCut;

    /** The XML document for vectorials instructions */
    xmlDocPtr vecdoc;
    /** The vectorials intructions element */
    xmlNodePtr vecroot;

    /** The directory name which contain all data */
    GString *dataDirectory;
    /** The rel position for writting files */
    GString *RelfileName;
    /** For XML ref with xi:include */
    GString *ImgfileName;

    /**   */

    void *vecOutputStream;

    /** To keep text in content stream order */
    GBool rawOrder;
    /** PL: To modify the blocks in reading order */
    GBool readingOrder;
    /** To know if the verbose option is selected */
    GBool verbose;
    /** To know if the cutPages option is selected */
    GBool cutter;

    /** The width of current page */
    double pageWidth;
    /** The height of current page */
    double pageHeight;
    /** The currently active string */
    TextWord *curWord;
    /** The next character position (within content stream) */
    int charPos;
    /** The current font */
    TextFontInfo *curFont;
    /** The current font size */
    double curFontSize;
    /** The current nesting level (for Type 3 fonts) */
    int nest;
    /** The number of "tiny" chars seen so far */
    int nTinyChars;
    /** To set if the last added char overlapped the previous char */
    GBool lastCharOverlap;

    /** The primary rotation */
    int primaryRot;
    /** The primary direction (<code>true</code> means L-to-R, <code>false</code> means R-to-L) */
    GBool primaryLR;
    /** The list of words, in raw order (only if rawOrder is set) */
    TextWord *rawWords;
    /** The last word on rawWords list */
    TextWord *rawLastWord;
    /** All fonts info objects used on this page <code>TextFontInfo</code> */
    GList *fonts;
    /** The <b>x</b> value coordinate of the last "find" result */
    double lastFindXMin;
    /** The <b>y</b> value coordinate of the last "find" result */
    double lastFindYMin;
    GBool haveLastFind;

    /** The id for each clip tag */
    int idClip;
    /** stack for idclip */
    stack<int> idStack;

    /** id of the current idclip */
    int idCur;

    AnnotsXrce **annotsArray;		// annotations array
    vector<Link*> linkList;

    vector<Dict*>underlineObject;
    vector<Dict*>highlightedObject;
    Links *pageLinks;

};



// Simple class to save picture references
class PictureReference
{
public:

    PictureReference (int ref, int flip, int number, const char* const extension) :
            reference_number(ref),
            picture_flip(flip),
            picture_number(number),
            picture_extension(extension)
    {}

    int					reference_number;
    int					picture_flip;		// 0 = none, 1 = flip X, 2 = flip Y, 3 = flip both
    int					picture_number;
    const char *const	picture_extension;
};




//------------------------------------------------------------------------
// XmlAltoOutputDev
//------------------------------------------------------------------------
/**
 * XmlAltoOutputDev.h (based on TextOutputDev.h, Copyright 1997-2003 Glyph & Cog, LLC)<br></br>
 * Xerox Research Centre Europe <br></br>
 * @date 04-2006
 * @author Herv� D�jean
 * @author Sophie Andrieu
 * @version xpdf 3.01
 * @see OutputDev
 */

class XmlAltoOutputDev: public OutputDev {
public:

    /**
     * Construct a new <code>XmlOutputDev</code><br></br>
     * Open a text output file. If <i>fileName</i> is NULL, no file is written
     * (this is useful, e.g., for searching text). If <i>physLayoutA</i> is
     * <code>true</code>, the original physical ayout of the text is maintained.
     * If <i>verbose</i> is <code>true</code>, the output xml file will contain
     * more informations attributes about TOKEN tag.
     * @param fileName The file name
     * @param fileNamePdf The PDF file name
     * @param physLayoutA To know if the physLayout option is selected
     * @param verboseA To know if the vernose option is selected
     * @param nsURIA The namespace URI if it specified
     * @param cmdA The command line used to execute the tool
     */
    XmlAltoOutputDev(GString *fileName, GString *fileNamePdf,Catalog *catalog, GBool physLayoutA, GBool verboseA, GString *nsURIA, GString *cmdA);

    /**
     *  Destructor
     */
    virtual ~XmlAltoOutputDev();

    /** Check if file was successfully created
     * @return <code>true</code> if the file was successfully created, <code>false</code> else*/
    virtual GBool isOk() { return ok; }

    /** Does this device use upside-down coordinates?
     * (Upside-down means (0,0) is the top left corner of the page.)
     * @return <code>true</code> if this device use upside-down, <code>false</code> otherwise */
    virtual GBool upsideDown() { return gTrue; }

    /** Does this device use drawChar() or drawString()?
     * @return <code>true</code> if this device use drawChar(), <code>false</code> otherwise */
    virtual GBool useDrawChar() { return gTrue; }

    /** Does this device use beginType3Char/endType3Char?  Otherwise,
     * text in Type 3 fonts will be drawn with drawChar/drawString.
     * @return <code>true</code> if this device use beginType3Char/endType3Char, <code>false</code> otherwise */
    virtual GBool interpretType3Chars() { return gFalse; }

    /** Start a page
     * @param pageNum The numero of the current page
     * @param state The state description */
    virtual void startPage(int pageNum, GfxState *state);

    /** End a page */
    virtual void endPage();

    /** Update text font state
     * @param state The state description */
    virtual void updateFont(GfxState *state);

    /** Draw the current character
     * @param state The state description
     * @param x The x value coordinate
     * @param y The y value coordinate
     * @param dx The dx value coordinate
     * @param dy The dy value coordinate
     * @param originX The origin x value coordinate
     * @param originY The origin y value coordinate
     * @param c The current character
     * @param nBytes The bytes
     * @param u The unicode text
     * @param uLen The lenght */
    virtual void drawChar(GfxState *state, double x, double y,
                          double dx, double dy,
                          double originX, double originY,
                          CharCode c, int nBytes, Unicode *u, int uLen);

    /** Save graphics state
     * @param state The state description */
    virtual void saveState(GfxState *state);
    /** Restore graphics state
     * @param state The state description */
    virtual void restoreState(GfxState *state);

    /** Add informations about the stroke painting
     * @param state The state description */
    virtual void stroke(GfxState *state);
    /** Add informations about the fill painting
     * @param state The state description */
    virtual void fill(GfxState *state) ;
    /** Add informations about the fill painting with even-odd rule
     * @param state The state description */
    virtual void eoFill(GfxState *state) ;

    /** Create a clipping
     * @param state The state description */
    virtual void clip(GfxState *state) ;
    /** Create a clipping with even-odd rule
     * @param state The state description */
    virtual void eoClip(GfxState *state) ;
    virtual void clipToStrokePath(GfxState *state);

    GString *convtoX(unsigned int xcol) const;

    /** Return the hexadecimal value of the color of string
     * @param rgb The color in RGB value
     * @return The hexadecimal value color in a string value*/
    GString *colortoString(GfxRGB rgb) const;


    /** Draw the image mask
     * @param state The state description
     * @param ref The reference
     * @param str The stream
     * @param width The image width value
     * @param height The image height value
     * @param invert To know if the current image is invert
     * @param inlineImg To know if the current image is inline or not */
    virtual void drawImageMask(GfxState *state, Object *ref, Stream *str,
                               int width, int height, GBool invert, GBool inlineImg);



    /** Draw the image
     * @param state The state description
     * @param ref The reference
     * @param str The stream
     * @param width The image width value
     * @param height The image height value
     * @param colorMap The color map
     * @param maskColors The mask color value
     * @param inlineImg To know if the current image is inline or not */
    virtual void drawImage(GfxState *state, Object *ref, Stream *str,
                           int width, int height, GfxImageColorMap *colorMap,
                           int *maskColors, GBool inlineImg);

    /** Initialize for the outline creation */
    virtual void initOutline(int nbPage);

    /**Close and save the XML outline file which was created
     * @param shortFileName The XML outline short file name */
    virtual void closeOutline(GString *shortFileName);

    /** Dump the outline
     * @param itemsA The items list
     * @param docA The PDF document object
     * @param uMapA The unicode map
     * @param levelA The hierarchic level of the current items list
     * @param idItemTocParentA The id of the parent item of the current item */
    GBool dumpOutline(xmlNodePtr parentNode,GList *itemsA, PDFDoc *docA, UnicodeMap *uMapA, int levelA, int idItemTocParentA);
    int dumpFragment(Unicode *text, int len, UnicodeMap *uMap, GString *s);
    /** Generate an XML outline file : call the dumpOutline function
     * @param itemsA The items list
     * @param docA The PDF document object
     * @param levelA The hierarchic level of the current items list */
    void generateOutline(GList *itemsA, PDFDoc *docA, int levelA);

    /** Add the pdf metadata informations
     * @param doc The PDF document object */
    void addInfo(PDFDocXrce *doc);

    /** Picks and returns the needed data
     * @param infoDict dictionary containing metadata informations
     * @param key the element looking for */
    GString* getInfoString(Dict *infoDict, const char *key);

    /** Picks and returns the needed data
     * @param infoDict dictionary containing metadata informations
     * @param key the element looking for */
    GString* getInfoDate(Dict *infoDict, const char *key);

    /** [Utility function] looks for and returns a node
     * @param rootnode the parent node we'll start looking from
     * @param nodename the name of the node we are looking for */
    xmlNodePtr findNodeByName(xmlNodePtr rootnode, const xmlChar * nodename);

    GString* toUnicode(GString *s,UnicodeMap *uMap);


    xmlNodePtr getDocRoot(){return docroot;}

    xmlDocPtr getDoc(){return doc;}
private:

    /** Generate the path
     * @param path The current path
     * @param state The state description
     * @param gattributes Style attributes to add to the current path */
    void doPath(GfxPath *path, GfxState *state, GString* gattributes);

    double curstate[6];
    //double *curstate[1000];


    /** The XML document */
    xmlDocPtr  doc;
    /** The root node */
    xmlNodePtr docroot;


    /** The XML document outline */
    xmlDocPtr  docOutline;
    /** The root outline node */
    xmlNodePtr docOutlineRoot;

    Catalog *myCatalog;


    /** The XML document for vectorials instructions */
    xmlDocPtr  vecdoc;
    /** The root vectorials instructions node */
    xmlNodePtr vecroot;

    /** Need to close the output file? (only if outputStream is a FILE*) */
    GBool needClose;
    /** The text of the current page */
    TextPage *text;
    /** To maintain original physical layout when dumping text */
    GBool physLayout;
    /** To keep text in content stream order */
    GBool verbose;
    /** To keep text in content stream order */
    GBool rawOrder;
    /** To know if the blocks option is selected */
    GBool blocks;
    /** To know if the fullFontName option is selected */
    GBool fullFontName;
    /** To know if the noImageInline option is selected */
    GBool noImageInline;
    /** set up ok */
    GBool ok;

    /** The PDF file name */
    GString *fileNamePDF;

    /** The directory name which contain all data */
    GString *dataDir;
    GString *baseFileName;
    /** The buffer for output file names */
    GString *myfilename;
    /** The namespace specified */
    GString *nsURI;

    /** To set to dump native JPEG files */
    GBool dumpJPEG;
    /** The index for each image */
    int imageIndex;

    /** list of pictures references*/
    GList *lPictureReferences;


    /** The item id for each toc items */
    int idItemToc;




};

#endif
