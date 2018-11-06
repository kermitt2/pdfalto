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
#include "GHash.h"
// PNG lib
#include "png.h"


using namespace std;
#include <list>
#include <stack>
#include <vector>
#include <string>
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include <splash/SplashTypes.h>
#include <splash/SplashFont.h>
#include <splash/SplashFontEngine.h>
#include <splash/SplashPath.h>
#include <splash/Splash.h>

// number of Type 3 fonts to cache
#define splashOutT3FontCacheSize 8

#include "PDFDocXrce.h"

#define xoutRound(x) ((int)(x + 0.5))

class GfxPath;
class GfxFont;
class GfxColorSpace;
class GfxSeparationColorSpace;
class PDFRectangle;
struct PSFont16Enc;
class PSOutCustomColor;

class GHash;
class GString;
class GList;
class GfxState;
class UnicodeMap;

class TextBlock;


class T3FontCache;
struct T3FontCacheTag;

class DataGeneratorOutputDev;


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

    Ref fontID;
    GfxFont *gfxFont;
//#if TEXTOUT_WORD_LIST
    GString *fontName;
    int flags;
    double mWidth;
    double ascent, descent;
//#endif

    friend class TextLine;
    friend class TextWord;
    friend class TextRawWord;
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

    GBool getFontWidth() const { return fontWidth; }
    void setFontWidth(GBool fontwidth){fontWidth = fontwidth;}

    GBool isBold() const { return isbold; }
    void setIsBold(GBool isBold){isbold = isBold;}

    GBool isItalic() const { return isitalic; }
    void setIsItalic(GBool isItalic){isitalic = isItalic;}

    GBool isSubscript() const { return issubscript; }
    void setIsSubscript(GBool isSubscript){issubscript = isSubscript;}

    GBool isSuperscript() const { return issuperscript; }
    void setIsSuperscript(GBool isSuperscript){issuperscript = isSuperscript;}

    // Compare two text fonts:  -1:<  0:=  +1:>
    GBool cmp(TextFontStyleInfo *tsi);
//#endif

private:

    int id;
    GString* fontName;
    double fontSize;
    GString* fontColor;
    GBool fontType = gFalse; //Enumeration : serif (gTrue) or sans-serif(gFalse)
    GBool fontWidth = gFalse; //Enumeration : proportional(gFalse) or fixed(gTrue)

    //see fontStylesType in alto schema
    GBool isbold = gFalse;
    GBool isitalic = gFalse;
    GBool issubscript = gFalse;
    GBool issuperscript = gFalse;

    friend class TextFontInfo;
};

//------------------------------------------------------------------------
// TextChar
//------------------------------------------------------------------------

class TextChar {
public:

    TextChar(GfxState *state, Unicode cA, CharCode charCodeA, int charPosA, int charLenA,
             double xMinA, double yMinA, double xMaxA, double yMaxA,
             int rotA, GBool clippedA, GBool invisibleA,
             TextFontInfo *fontA, double fontSizeA, SplashFont *splashFontA,
             double colorRA, double colorGA, double colorBA, GBool isNonUnicodeGlyphA);

    static int cmpX(const void *p1, const void *p2);
    static int cmpY(const void *p1, const void *p2);

    GfxState *state;
    Unicode c;
    CharCode charCode;
    int charPos;
    int charLen;
    double xMin, yMin, xMax, yMax;
    Guchar rot;
    char clipped;
    char invisible;
    char spaceAfter;
    TextFontInfo *font;
    SplashFont *splashfont;
    GBool isNonUnicodeGlyph;
    double fontSize;
    double colorR,
            colorG,
            colorB;
};

//------------------------------------------------------------------------
// IWord
//------------------------------------------------------------------------
class IWord {
public:

    /** The id of the word (used to include and localize the image inline in the stream) */
    int idWord;

    /** The absolute object index in the stream **/
    int idx;

    double xMin, xMax;		// bounding box x coordinates
    double yMin, yMax;		// bounding box y coordinates

    /** The baseline x or y coordinate */
    double base;
    /** The yMin of the word function the rotation */
    double baseYmin;

    /** The rotation which is multiple of 90 degrees (0, 1, 2, or 3) */
    int rot;
    /** The angle value of rotation in degree (0 to 360 degree) */
    int angle;
    /** The y angle skewing value : the value can be positive or negative and it is function the angle position in the y axis */
    int angleSkewing_Y;
    /** The x angle skewing value : the value can be positive or negative and it is function the angle position in the x axis */
    int angleSkewing_X;

    /** The unicode text */
    GList *chars;			// [TextChar]
    /** "near" edge x or y coord of each char (plus one extra entry for the last char) */
    double *edge;
    /** The length of text and edge arrays */
    int len;
    /** The size of text and edge arrays */
    int size;
    /** The character position (within content stream) */
    int *charPos;			// character position (within content stream)
    //   of each char (plus one extra entry for
    //   the last char)
    /** The number of content stream characters in this word */
    int charLen;

    GBool spaceAfter;		// set if there is a space between this
    //   word and the next word on the line

    /** <code>true</code> if the current <code>TextRawWord</code> is <b>bold</b>, <code>false</code> else */
    GBool  bold;
    /** <code>true</code> if the current <code>TextRawWord</code> is <b>italic</b>, <code>false</code> else */
    GBool  italic;
    /** <code>true</code> if the current <code>TextRawWord</code> is <b>symbolic</b>, <code>false</code> else */
    GBool  symbolic;
    /** <code>true</code> if the current <code>TextRawWord</code> is <b>serif</b>, <code>false</code> else */
    GBool  serif;

    /** The font information */
    TextFontInfo *font;
    /** The font name */
    char *fontName;
    /** The font size */
    double fontSize;

    /** The value of word Red color */
    double colorR;
    /** The value of word Green color */
    double colorG;
    /** The value of word Blue color */
    double colorB;

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

    GBool isNonUnicodeGlyph;
//public:


    /** Get the absolute object index in the stream
     * @return The absolute object index in the stream */
    int getIdx(){return idx;};
    /** Get the font name of the current word
     *  @return The font name of the current word */
    char *getFontName(){return fontName;}

    double getFontSize() { return fontSize; }
    int getRotation() { return rot; }

    /** Check if the current <code>TextRawWord</code> is <b>bold</b>
     *  @return <code>true</code> if the current <code>TextRawWord</code> is <b>bold</b>, <code>false</code> else */
    GBool isBold() {return bold;}

    /** Check if the current <code>TextRawWord</code> is <b>italic</b>
     *  @return <code>true</code> if the current <code>TextRawWord</code> is <b>italic</b>, <code>false</code> else */
    GBool isItalic() {return italic;}

    /** Check if the current <code>TextRawWord</code> is <b>symbolic</b>
     *  @return <code>true</code> if the current <code>TextRawWord</code> is <b>symbolic</b>, <code>false</code> else */
    GBool isSymbloic() {return symbolic;}

    /** Get the color RGB of the word
     * @param r The Red color value
     * @param g The Green color value
     * @param b The Blue color value */
    void getColor(double *r, double *g, double *b)
    { *r = colorR; *g = colorG; *b = colorB; }

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

    int getLength() { return len; }

    Unicode getChar(int idx);

    /** Convert the RGB color to string hexadecimal color value*/
    GString *colortoString() const;

    GString *convtoX(double xcol) const;

    static int cmpX(const void *p1, const void *p2);

    static int cmpY(const void *p1, const void *p2);

    static int cmpYX(const void *p1, const void *p2);

    void getBBox(double *xMinA, double *yMinA, double *xMaxA, double *yMaxA)
    { *xMinA = xMin; *yMinA = yMin; *xMaxA = xMax; *yMaxA = yMax; }
    void getCharBBox(int charIdx, double *xMinA, double *yMinA,
                     double *xMaxA, double *yMaxA);

    GBool getSpaceAfter() { return spaceAfter; }

    GBool setSpaceAfter(GBool spaceAfterA) { return spaceAfter = spaceAfterA; }

    int getCharPos() { return charPos[0]; }
    int getCharLen() { return charPos[len] - charPos[0]; }

    Unicode getStandardBaseChar(Unicode c);

    void setContainNonUnicodeGlyph(GBool containNonUnicodeGlyph);

    GBool containNonUnicodeGlyph(){return isNonUnicodeGlyph;};
};

//------------------------------------------------------------------------
// TextWord
//------------------------------------------------------------------------

class TextWord : public IWord {
public:

    TextWord(GList *chars, int start, int lenA,
             int rotA, int dirA, GBool spaceAfterA, GfxState *state,
             TextFontInfo *fontA, double fontSizeA, int idCurrentWord,
             int index);
    ~TextWord();
    TextWord *copy() { return new TextWord(this); }

    // Get the TextFontInfo object associated with this word.
    TextFontInfo *getFontInfo() { return font; }

    Unicode getChar(int idx);
    GString *getText();
    GBool isInvisible() { return invisible; }

    int getDirection() { return dir; }
    double getBaseline();
    GBool isUnderlined() { return underlined; }
    GString *getLinkURI();

private:

    TextWord(TextWord *word);
    static int cmpYX(const void *p1, const void *p2);
    static int cmpCharPos(const void *p1, const void *p2);

    int dir;			// character direction (+1 = left-to-right;
    //   -1 = right-to-left; 0 = neither)

    GBool underlined;
    //TextLink *link;

    GBool invisible;		// set for invisible text (render mode 3)

    friend class TextBlock;
    friend class TextLine;
    friend class TextPage;
};

//------------------------------------------------------------------------
// TextRawWord
//------------------------------------------------------------------------
/**
 * TextRawWord class<br></br>
 * Represents the words as a character sequence from stream order separated by a space character (U+0020) each word pointing to next one<br></br>
 * @date 07-2018
 * @author Achraf Azhar
 * @version xpdf 3.01 */

class TextRawWord : public IWord {
public:

    /** Construct a new <code>TextRawWord</code>
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
    TextRawWord(GfxState *state, double x0, double y0,
                TextFontInfo *fontA, double fontSizeA, int idCurrentWord,
                int index);

    /** Destructor */
    ~TextRawWord();

    /** Add a character to the current word
     *  @param state The state description
     *  @param x The x value
     *  @param y The y value
     *  @param dx The dx value
     *  @param dy The dy value
     *  @param u The unicode char to add
     */
    void addChar(GfxState *state, double x, double y, double dx,
                 double dy, Unicode u, CharCode charCodeA, int charPosA, GBool overlap, TextFontInfo *fontA, double fontSizeA, SplashFont * splashFont, int rotA, int nBytes, GBool isNonUnicodeGlyph);

    /** Merge <code>word</code> onto the end of <code>this</code>
     *  @param word The current word */
    void merge(TextRawWord *word);

    /** Compares <code>this</code> to <code>word</code>, returning -1 (<), 0 (=), or +1 (>),<br></br>
     *  based on a primary-axis comparison, e.g., x ordering if rot=0
     *  @param word The current word */
    int primaryCmp(TextRawWord *word);

    /** Get the distance along the primary axis between <code>this</code> and <code>word</code>
     *  @param word The current word */
    double primaryDelta(TextRawWord *word);

    /**
     * return True if overlap with w2
     */
    GBool overlap(TextRawWord *w2);

    Unicode getChar(int idx);


//private:

    /** Rank in the original flow */
    int indexmin;
    int indexmax;

    /** The next word in line */
    TextRawWord *next;

    friend class TextPage;
};


//------------------------------------------------------------------------
// TextLine
//------------------------------------------------------------------------

class TextLine {
public:

    TextLine(GList *wordsA, double xMinA, double yMinA,
             double xMaxA, double yMaxA, double fontSizeA);
    ~TextLine();

    double getXMin() { return xMin; }
    double getYMin() { return yMin; }
    double getXMax() { return xMax; }
    double getYMax() { return yMax; }
    double getBaseline();
    int getRotation() { return rot; }
    GList *getWords() { return words; }
    int getLength() { return len; }
    double getEdge(int idx) { return edge[idx]; }

private:

    static int cmpX(const void *p1, const void *p2);

    GList *words;			// [TextWord]
    int rot;			// rotation, multiple of 90 degrees
    //   (0, 1, 2, or 3)
    double xMin, xMax;		// bounding box x coordinates
    double yMin, yMax;		// bounding box y coordinates
    double fontSize;		// main (max) font size for this line
    Unicode *text;		// Unicode text of the line, including
    //   spaces between words
    double *edge;			// "near" edge x or y coord of each char
    //   (plus one extra entry for the last char)
    int len;			// number of Unicode chars
    GBool hyphenated;		// set if last char is a hyphen
    int px;			// x offset (in characters, relative to
    //   containing column) in physical layout mode
    int pw;			// line width (in characters) in physical
    //   layout mode

    friend class TextSuperLine;
    friend class TextPage;
    friend class TextParagraph;
};

//------------------------------------------------------------------------
// TextParagraph
//------------------------------------------------------------------------

class TextParagraph {
public:

    TextParagraph(GList *linesA, GBool dropCapA);
    ~TextParagraph();

    // Get the list of TextLine objects.
    GList *getLines() { return lines; }

    GBool hasDropCap() { return dropCap; }

    double getXMin() { return xMin; }
    double getYMin() { return yMin; }
    double getXMax() { return xMax; }
    double getYMax() { return yMax; }

private:

    GList *lines;			// [TextLine]
    GBool dropCap;		// paragraph starts with a drop capital
    double xMin, xMax;		// bounding box x coordinates
    double yMin, yMax;		// bounding box y coordinates

    friend class TextPage;
};

//------------------------------------------------------------------------
// TextColumn
//------------------------------------------------------------------------

class TextColumn {
public:

    TextColumn(GList *paragraphsA, double xMinA, double yMinA,
               double xMaxA, double yMaxA);
    ~TextColumn();

    // Get the list of TextParagraph objects.
    GList *getParagraphs() { return paragraphs; }

    double getXMin() { return xMin; }
    double getYMin() { return yMin; }
    double getXMax() { return xMax; }
    double getYMax() { return yMax; }

    int getRotation();

private:

    static int cmpX(const void *p1, const void *p2);
    static int cmpY(const void *p1, const void *p2);
    static int cmpPX(const void *p1, const void *p2);

    GList *paragraphs;		// [TextParagraph]
    double xMin, xMax;		// bounding box x coordinates
    double yMin, yMax;		// bounding box y coordinates
    int px, py;			// x, y position (in characters) in physical
    //   layout mode
    int pw, ph;			// column width, height (in characters) in
    //   physical layout mode

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

    /** Add a character either to the current word or to the list of page chars to be ordered
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
                 CharCode c, int nBytes, Unicode *u, int uLen, SplashFont * splashFont, GBool isNonUnicodeGlyph);

    /** Add a character to the list of characters in the page
     *  @param state The state description
     *  @param x The x value of left bottom corner of the box character
     *  @param y The y value of left bottom corner of the box character
     *  @param dx The dx value
     *  @param dy The dy value
     *  @param c The code character
     *  @param nBytes The number of bytes
     *  @param u The unicode character value
     *  @param uLen The lenght */
    void addCharToPageChars(GfxState *state, double x, double y,
                 double dx, double dy,
                 CharCode c, int nBytes, Unicode *u, int uLen, SplashFont * splashFont, GBool isNonUnicodeGlyph);

    /** Add a character to the current word in raw order
     *  @param state The state description
     *  @param x The x value of left bottom corner of the box character
     *  @param y The y value of left bottom corner of the box character
     *  @param dx The dx value
     *  @param dy The dy value
     *  @param c The code character
     *  @param nBytes The number of bytes
     *  @param u The unicode character value
     *  @param uLen The lenght */
    void addCharToRawWord(GfxState *state, double x, double y,
                 double dx, double dy,
                 CharCode c, int nBytes, Unicode *u, int uLen, SplashFont * splashFont, GBool isNonUnicodeGlyph);

    /** End the current word, sorting it into the list of words */
    void endWord();

    /** Add a word, sorting it into the list of words
     *  @param word The current word */
    void addWord(TextRawWord *word);

    /** Dump contents of the current page
     * @param blocks To know if the blocks option is selected
     * @param fullFontName To know if the fullFontName option is selected */
    void dump(GBool blocks, GBool fullFontName);

    /** Dump contents of the current page following the reading order.
     * @param blocks To know if the blocks option is selected
     * @param fullFontName To know if the fullFontName option is selected */
    void dumpInReadingOrder(GBool blocks, GBool fullFontName);

    int rotateChars(GList *charsA);

    void unrotateChars(GList *wordsA, int rot);

    GBool checkPrimaryLR(GList *charsA);

    TextBlock *splitChars(GList *charsA);

    void insertClippedChars(GList *clippedChars, TextBlock *tree);

    TextBlock *findClippedCharLeaf(TextChar *ch, TextBlock *tree);

    TextBlock *split(GList *charsA, int rot);

    void insertLargeChars(GList *largeChars, TextBlock *blk);

    void insertLargeCharsInFirstLeaf(GList *largeChars, TextBlock *blk);

    void insertLargeCharInLeaf(TextChar *ch, TextBlock *blk);

    GList *getWords(GList *wordsA, double xMin, double yMin,
                              double xMax, double yMax);

    void findGaps(GList *charsA, int rot,
                            double *xMinOut, double *yMinOut,
                            double *xMaxOut, double *yMaxOut,
                            double *avgFontSizeOut,
                            GList *horizGaps, GList *vertGaps);

    void tagBlock(TextBlock *blk);

    GList *buildColumns(TextBlock *tree, GBool primaryLR);

    void buildColumns2(TextBlock *blk, GList *columns, GBool primaryLR);

    TextColumn *buildColumn(TextBlock *blk);

    double getLineIndent(TextLine *line, TextBlock *blk);

    double getAverageLineSpacing(GList *lines);

    double getLineSpacing(TextLine *line0, TextLine *line1);

    void buildLines(TextBlock *blk, GList *lines);

    TextLine *buildLine(TextBlock *blk);

    void getLineChars(TextBlock *blk, GList *charsA);

    double computeWordSpacingThreshold(GList *charsA, int rot);

    int getCharDirection(TextChar *ch);

    int assignPhysLayoutPositions(GList *columns);

    void assignLinePhysPositions(GList *columns);

    void computeLinePhysWidth(TextLine *line, UnicodeMap *uMap);

    int assignColumnPhysPositions(GList *columns);

    void buildSuperLines(TextBlock *blk, GList *superLines);

    void assignSimpleLayoutPositions(GList *superLines,
                                               UnicodeMap *uMap);

    void generateUnderlinesAndLinks(GList *columns);

    void insertIntoTree(TextBlock *blk, TextBlock *primaryTree);

    void insertColumnIntoTree(TextBlock *column, TextBlock *tree);

    void insertLargeWords(GList *largeWords, TextBlock *blk);

    void insertLargeWordsInFirstLeaf(GList *largeWords, TextBlock *blk);

    void insertLargeWordInLeaf(TextWord *ch, TextBlock *blk);

    /** PL: insert a block in the page's list of block nodes according to the reading order
     * @param nodeblock the block node to be inserted
     * @param lastInserted boolean indicating if the previously added block node was inserted before
     * an existing node (value is true) or append top the list (value is false)
     * @return true the node is the inserted before an existing node, false if it is append to the list
     */
    GBool addBlockChildReadingOrder(xmlNodePtr nodeblock, GBool lastInserted);

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
    const char* drawImageMask(GfxState *state, Object *ref, Stream *str,
                       int width, int height, GBool invert,
                       GBool inlineImg, GBool dumpJPEG, int imageIndex);

    /** Get the id of the current word : this method is used for include the images inline in the stream
     * @return The id of the current word */
    int getIdWORD(){return idWORD;};

    /** Get the absolute object index in the stream
     * @return The absolute object index in the stream */
    int getIdx(){return idx;};

    void endActualText(GfxState *state, SplashFont* splashFont);

    void beginActualText(GfxState *state, Unicode *u, int uLen);

    /** The list of all recognized font styles*/
    vector<TextFontStyleInfo*> fontStyles;

    // clamp to uint8
    static inline int clamp (int x)
    {
        if (x > 255) return 255;
        if (x < 0) return 0;
        return x;
    }

    TextFontInfo * getCurrentFont();

    TextRawWord * getRawWords(){ return rawWords;}

    void setupPNG(png_structp *png, png_infop *pngInfo, FILE *f,
                  int bitDepth, int colorType, double res,
                  SplashBitmap *bitmap);

    void writePNGData(png_structp png, SplashBitmap *bitmap);

    void finishPNG(png_structp *png, png_infop *pngInfo);

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

    /**   */

    void *vecOutputStream;

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
    TextRawWord *curWord;
    /** The next character position (within content stream) */
    int charPos;
    /** The current font */
    TextFontInfo *curFont;

    int curRot;			// current rotation
    GBool diagonal;		// set if rotation is not close to
    //   0/90/180/270 degrees
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
    GList *chars;			// [TextChar]
    /** The list of words */
    GList *words;
    /** The list of words, in raw order (only if rawOrder is set) */
    TextRawWord *rawWords;
    /** The last word on rawWords list */
    TextRawWord *rawLastWord;
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

    vector<Link*> linkList;

    vector<Dict*>underlineObject;
    vector<Dict*>highlightedObject;
    Links *pageLinks;


    Unicode *actualText;		// current "ActualText" span
    int actualTextLen;
    double actualTextX0,
            actualTextY0,
            actualTextX1,
            actualTextY1;
    int actualTextNBytes;

    void setupScreenParams(double hDPI, double vDPI);

    SplashScreenParams screenParams;

    Splash *splash;

    GList *getChars(GList *charsA, double xMin, double yMin, double xMax, double yMax);
//
//    friend class TextBlock;
//    friend class TextColumn;
//    friend class TextLine;
//    friend class TextRawWord;
//    friend class TextWord;
};

//------------------------------------------------------------------------
// DataGeneratorOutputDev
//------------------------------------------------------------------------
/**
 * DataGeneratorOutputDev.h (based on TextOutputDev.h, Copyright 1997-2003 Glyph & Cog, LLC)<br></br>
 * Xerox Research Centre Europe <br></br>
 * @date 04-2006
 * @author Herv� D�jean
 * @author Sophie Andrieu
 * @version xpdf 3.01
 * @see OutputDev
 */

class DataGeneratorOutputDev: public OutputDev {
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
    DataGeneratorOutputDev(GString *fileName, GString *fileNamePdf,Catalog *catalog, GBool physLayoutA, GBool verboseA, GString *nsURIA, GString *cmdA);

    /**
     *  Destructor
     */
    virtual ~DataGeneratorOutputDev();

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

    // Does this device use tilingPatternFill()?  If this returns false,
    // tiling pattern fills will be reduced to a series of other drawing
    // operations.
    virtual GBool useTilingPatternFill() { return gTrue; }

    virtual GBool needNonText();

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

    virtual void tilingPatternFill(GfxState *state, Gfx *gfx, Object *strRef,
                                   int paintType, int tilingType, Dict *resDict,
                                   double *mat, double *bbox,
                                   int x0, int y0, int x1, int y1,
                                   double xStep, double yStep);

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
                               int width, int height, GBool invert, GBool inlineImg,
                               GBool interpolate);

    virtual void drawSoftMaskedImage(GfxState *state, Object *ref,
                                               Stream *str,
                                               int width, int height,
                                               GfxImageColorMap *colorMap,
                                               Stream *maskStr,
                                               int maskWidth, int maskHeight,
                                               GfxImageColorMap *maskColorMap,
                                               double *matte, GBool interpolate);

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
                           int *maskColors, GBool inlineImg,
                           GBool interpolate);
    int dumpFragment(Unicode *text, int len, UnicodeMap *uMap, GString *s);


    /** [Utility function] looks for and returns a node
     * @param rootnode the parent node we'll start looking from
     * @param nodename the name of the node we are looking for */
    xmlNodePtr findNodeByName(xmlNodePtr rootnode, const xmlChar * nodename);

    GString* toUnicode(GString *s,UnicodeMap *uMap);

    TextPage * getText(){return text;}

    void startDoc(XRef *xrefA);

    void updateCTM(GfxState *state, double m11, double m12,
                                     double m21, double m22,
                                     double m31, double m32);

private:

    /** Generate the path
     * @param path The current path
     * @param state The state description
     * @param gattributes Style attributes to add to the current path */
    void doPath(GfxPath *path, GfxState *state, GString* gattributes);

    double curstate[6];//this is the ctm
    //double *curstate[1000];


    Catalog *myCatalog;


    /** Need to close the output file? (only if outputStream is a FILE*) */
    GBool needClose;
    /** The text of the current page */
    TextPage *text;
    /** To maintain original physical layout when dumping text */
    GBool physLayout;
    /** To keep text in content stream order */
    GBool verbose;
    /** To keep text in content stream order */
    //GBool rawOrder;
/** To make text in reading order */
    GBool readingOrder;
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
    //GString *myfilename;

    /** To set to dump native JPEG files */
    GBool dumpJPEG;

    /** The item id for each toc items */
    int idItemToc;

    GHash *unicode_map;

    void beginActualText(GfxState *state, Unicode *u, int uLen);

    void endActualText(GfxState *state);

    SplashPath *path;

    SplashFontEngine *fontEngine;

    XRef *xref;			// xref table for current document
    int nT3Fonts;
    T3FontCache *			// Type 3 font cache
            t3FontCache[splashOutT3FontCacheSize];

    GBool needFontUpdate;		// set when the font needs to be updated
    SplashFont * getSplashFont(GfxState *state, SplashCoord *matrix);

    SplashPath *convertPath(GfxState *state, GfxPath *path, GBool dropEmptySubpaths);
};

#endif
