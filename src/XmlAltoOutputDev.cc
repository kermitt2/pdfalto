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
#include <algorithm>    // std::min_element, std::max_element

#define _USE_MATH_DEFINES

#include <cmath> // PL: for using std::abs

#include <iostream>

using namespace std;

#include "ConstantsUtils.h"

using namespace ConstantsUtils;

#include "ConstantsXML.h"

using namespace ConstantsXML;

#include "AnnotsXrce.h"


#include <stdio.h>
#include <stddef.h>
#include <math.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdarg.h>

#include "unicode/normalizer2.h"

using namespace icu;

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
#include "GHash.h"
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

#include "CharCodeToUnicode.h"

#include <splash/SplashBitmap.h>
#include <splash/Splash.h>
#include <splash/SplashFontEngine.h>
#include "FoFiTrueType.h"
#include <splash/SplashFontFile.h>
#include <splash/SplashFontFileID.h>
#include <splash/SplashGlyphBitmap.h>
#include <splash/SplashPath.h>
#include <splash/SplashPattern.h>


#include "BuiltinFont.h"
#include "BuiltinFontTables.h"


// Type 3 font cache size parameters
#define type3FontCacheAssoc   8
#define type3FontCacheMaxSets 8
#define type3FontCacheSize    (128*1024)
//#ifdef MACOS
//// needed for setting type/creator of MacOS files
//#include "ICSupport.h"
//#endif

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
// Cut parameters for reading order
//------------------------------------------------------------------------

// Size of bins used for horizontal and vertical profiles is
// splitPrecisionMul * minFontSize.
#define splitPrecisionMul 0.05

// Minimum allowed split precision.
#define minSplitPrecision 0.01

// yMin and yMax (or xMin and xMax for rot=1,3) are adjusted by this
// fraction of the text height, to allow for slightly overlapping
// lines (or large ascent/descent values).
#define ascentAdjustFactor 0
#define descentAdjustFactor 0.35

// Gaps larger than max{gap} - splitGapSlack * avgFontSize are
// considered to be equivalent.
#define splitGapSlack 0.2

// The vertical gap threshold (minimum gap required to split
// vertically) depends on the (approximate) number of lines in the
// block:
//   threshold = (max + slope * nLines) * avgFontSize
// with a min value of vertGapThresholdMin * avgFontSize.
#define vertGapThresholdMin 0.8
#define vertGapThresholdMax 3
#define vertGapThresholdSlope -0.5

// Vertical gap threshold for table mode.
#define vertGapThresholdTableMin 0.2
#define vertGapThresholdTableMax 0.5
#define vertGapThresholdTableSlope -0.02

// A large character has a font size larger than
// largeCharThreshold * avgFontSize.
#define largeCharThreshold 1.5

// A block will be split vertically only if the resulting chunk
// widths are greater than vertSplitChunkThreshold * avgFontSize.
#define vertSplitChunkThreshold 2

// Max difference in primary,secondary coordinates (as a fraction of
// the font size) allowed for duplicated text (fake boldface, drop
// shadows) which is to be discarded.
#define dupMaxPriDelta 0.1
#define dupMaxSecDelta 0.2

// Inter-character spacing that varies by less than this multiple of
// font size is assumed to be equivalent.
#define uniformSpacing 0.07

// Typical word spacing, as a fraction of font size.  This will be
// added to the minimum inter-character spacing, to account for wide
// character spacing.
#define wordSpacing 0.1

// Minimum paragraph indent from left margin, as a fraction of font
// size.
#define minParagraphIndent 0.5

// If the space between two lines is greater than
// paragraphSpacingThreshold * avgLineSpacing, start a new paragraph.
#define paragraphSpacingThreshold 1.25

// If font size changes by at least this much (measured in points)
// between lines, start a new paragraph.
#define paragraphFontSizeDelta 1

// Spaces at the start of a line in physical layout mode are this wide
// (as a multiple of font size).
#define physLayoutSpaceWidth 0.33

// In simple layout mode, lines are broken at gaps larger than this
// value multiplied by font size.
#define simpleLayoutGapThreshold 0.4

// Table cells (TextColumns) are allowed to overlap by this much
// in table layout mode (as a fraction of cell width or height).
#define tableCellOverlapSlack 0.05

// Primary axis delta which will cause a line break in raw mode
// (as a fraction of font size).
#define rawModeLineDelta 0.5

// Secondary axis delta which will cause a word break in raw mode
// (as a fraction of font size).
#define rawModeWordSpacing 0.15

// Secondary axis overlap which will cause a line break in raw mode
// (as a fraction of font size).
#define rawModeCharOverlap 0.2

// Max spacing (as a multiple of font size) allowed between the end of
// a line and a clipped character to be included in that line.
#define clippedTextMaxWordSpace 0.5

// Max width of underlines (in points).
#define maxUnderlineWidth 3

// Max horizontal distance between edge of word and start of underline
// (as a fraction of font size).
#define underlineSlack 0.2

// Max vertical distance between baseline of word and start of
// underline (as a fraction of font size).
#define underlineBaselineSlack 0.2

// Max distance between edge of text and edge of link border (as a
// fraction of font size).
#define hyperlinkSlack 0.2

// Text is considered diagonal if abs(tan(angle)) > diagonalThreshold.
// (Or 1/tan(angle) for 90/270 degrees.)
#define diagonalThreshold 0.1

// This value is used as the ascent when computing selection
// rectangles, in order to work around flakey ascent values in fonts.
#define selectionAscent 0.8

//------------------------------------------------------------------------
// TextFontInfo
//------------------------------------------------------------------------

TextFontInfo::TextFontInfo(GfxState *state) {
    gfxFont = state->getFont();
    if (gfxFont) {
        fontID = *gfxFont->getID();
        ascent = gfxFont->getAscent();
        descent = gfxFont->getDescent();
        // "odd" ascent/descent values cause trouble more often than not
        // (in theory these could be legitimate values for oddly designed
        // fonts -- but they are more often due to buggy PDF generators)
        // (values that are too small are a different issue -- those seem
        // to be more commonly legitimate)
        if (ascent > 1) {
            ascent = 0.75;
        }
        if (descent < -0.5) {
            descent = -0.25;
        }
    } else {
        fontID.num = -1;
        fontID.gen = -1;
        ascent = 0.75;
        descent = -0.25;
    }
    fontName = (gfxFont && gfxFont->getName()) ? gfxFont->getName()->copy()
                                               : (GString *) NULL;
    flags = gfxFont ? gfxFont->getFlags() : 0;
    mWidth = 0;
    if (gfxFont && !gfxFont->isCIDFont()) {
        char *name;
        int code;
        for (code = 0; code < 256; ++code) {
            if ((name = ((Gfx8BitFont *) gfxFont)->getCharName(code)) &&
                name[0] == 'm' && name[1] == '\0') {
                mWidth = ((Gfx8BitFont *) gfxFont)->getWidth(code);
                break;
            }
        }
    }
}

TextFontInfo::~TextFontInfo() {
    //#if TEXTOUT_WORD_LIST
    if (fontName) {
        delete fontName;
    }
    //#endif
}

GBool TextFontInfo::matches(GfxState *state) {
    Ref *id;

    if (!state->getFont()) {
        return gFalse;
    }
    id = state->getFont()->getID();
    return id->num == fontID.num && id->gen == fontID.gen;
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
                         double height, int idWord, int idImage, GString *href, int index) {
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
// Image
//------------------------------------------------------------------------

Image::Image(double xPosition, double yPosition, double width,
             double height, GString *id, GString *sid, GString *href, GString *clipzone, GBool isinline, double rotationA, GString* typeA) {
    xPositionImage = xPosition;
    yPositionImage = yPosition;
    widthImage = width;
    heightImage = height;
    imageId = id;
    imageSid = sid;
    hrefImage = href;
    clipZone = clipzone;
    isInline = isinline;
    type = typeA;
    rotation = rotationA;
}

Image::~Image() {

}

//------------------------------------------------------------------------
// TextChar
//------------------------------------------------------------------------

TextChar::TextChar(GfxState *stateA, Unicode cA, CharCode charCodeA, int charPosA, int charLenA,
                   double xMinA, double yMinA, double xMaxA, double yMaxA,
                   int rotA, GBool clippedA, GBool invisibleA,
                   TextFontInfo *fontA, double fontSizeA, SplashFont *splashFontA,
                   double colorRA, double colorGA, double colorBA, GBool isNonUnicodeGlyphA) {
    double t;
    state = stateA;
    c = cA;
    charCode = charCodeA;
    charPos = charPosA;
    charLen = charLenA;
    xMin = xMinA;
    yMin = yMinA;
    xMax = xMaxA;
    yMax = yMaxA;
    // this can happen with vertical writing mode, or with odd values
    // for the char/word spacing parameters
    if (xMin > xMax) {
        t = xMin;
        xMin = xMax;
        xMax = t;
    }
    if (yMin > yMax) {
        t = yMin;
        yMin = yMax;
        yMax = t;
    }
    // TextPage::findGaps uses integer coordinates, so clip the char
    // bbox to fit in a 32-bit int (this is generally only a problem in
    // damaged PDF files)
    if (xMin < -1e8) {
        xMin = -1e8;
    }
    if (xMax > 1e8) {
        xMax = 1e8;
    }
    if (yMin < -1e8) {
        yMin = -1e8;
    }
    if (yMax > 1e8) {
        yMax = 1e8;
    }
    rot = (Guchar) rotA;
    clipped = (char) clippedA;
    invisible = (char) invisibleA;
    spaceAfter = (char) gFalse;
    font = fontA;
    fontSize = fontSizeA;
    splashfont = splashFontA;
    isNonUnicodeGlyph = isNonUnicodeGlyphA;
    colorR = colorRA;
    colorG = colorGA;
    colorB = colorBA;
}

int TextChar::cmpX(const void *p1, const void *p2) {
    const TextChar *ch1 = *(const TextChar **) p1;
    const TextChar *ch2 = *(const TextChar **) p2;

    if (ch1->xMin < ch2->xMin) {
        return -1;
    } else if (ch1->xMin > ch2->xMin) {
        return 1;
    } else {
        return 0;
    }
}

int TextChar::cmpY(const void *p1, const void *p2) {
    const TextChar *ch1 = *(const TextChar **) p1;
    const TextChar *ch2 = *(const TextChar **) p2;

    if (ch1->yMin < ch2->yMin) {
        return -1;
    } else if (ch1->yMin > ch2->yMin) {
        return 1;
    } else {
        return 0;
    }
}


//------------------------------------------------------------------------
// TextWord
//------------------------------------------------------------------------

TextWord::TextWord(GList *charsA, int start, int lenA,
                   int rotA, int dirA, GBool spaceAfterA, GfxState *state,
                   TextFontInfo *fontA, double fontSizeA, int idCurrentWord,
                   int index) {
    GfxFont *gfxFont;
    double ascent, descent;

    TextChar *chPrev, *ch;

    font = fontA;
    italic = gFalse;
    bold = gFalse;
    serif = gFalse;
    symbolic = gFalse;
    lineNumber = false;

    spaceAfter = spaceAfterA;
    dir = dirA;

    idWord = idCurrentWord;
    idx = index;

    double *fontm;
    double m[4];
    double m2[4];
    double tan;


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
    tan = m[2] / m[0];
    // Get the angle value in radian
    tan = atan(tan);
    // To convert radian angle to degree angle
    tan = 180 * tan / M_PI;

    angle = static_cast<int>(tan);

    // Adjust the angle value
    switch (rot) {
        case 0:
            if (angle > 0)
                angle = 360 - angle;
            else
                angle = static_cast<int>(fabs(static_cast<double>(angle)));
            break;
        case 1:
            if (angle > 0)
                angle = 180 - angle;
            else
                angle = static_cast<int>(fabs(static_cast<double>(angle)));
            break;
        case 2:
            if (angle > 0)
                angle = 180 - angle;
            else
                angle = static_cast<int>(fabs(static_cast<double>(angle))) + 180;
            break;
        case 3:
            if (angle > 0)
                angle = 360 - angle;
            else
                angle = static_cast<int>(fabs(static_cast<double>(angle))) + 180;
            break;
    }

    // Recover the skewing angle value
    if (m[1] == 0 && m[2] != 0) {
        double angSkew = atan(m[2]);
        angSkew = (180 * angSkew / M_PI) - 90;
        angleSkewing_Y = static_cast<int>(angSkew);
        if (rot == 0) {
            angle = 0;
        }
    } else
        angleSkewing_Y = 0;

    if (m[1] != 0 && m[2] == 0) {
        double angSkew = atan(m[1]);
        angSkew = 180 * angSkew / M_PI;
        angleSkewing_X = static_cast<int>(angSkew);
        if (rot == 0) {
            angle = 0;
        }
    } else
        angleSkewing_X = 0;

    //base = 0;
    //baseYmin = 0;
    fontName = NULL;

    if (fontA) {
        if (fontA->getFontName()) {
            // PDF reference guide 5.5.3 For a font subset, the PostScript name of the font�the value of the font�s
            //BaseFont entry and the font descriptor�s FontName entry�begins with a tag
            //followed by a plus sign (+). The tag consists of exactly six uppercase letters; the
            //choice of letters is arbitrary, but different subsets in the same PDF file must have
            //different tags. For example, EOODIA+Poetica is the name of a subset of Poetica�, a
            //Type 1 font. (See implementation note 62 in Appendix H.)
            fontName = strdup(fontA->getFontName()->getCString());
            char* localLowerFontName = fontA->getFontName()->lowerCase()->getCString();
            if (strstr(localLowerFontName, "bold") ||
                strstr(localLowerFontName, "_bd")) {

                bold = gTrue;
            }

            if (strstr(localLowerFontName, "italic") ||
                strstr(localLowerFontName, "oblique") ||
                    strstr(localLowerFontName, "_it")) {

                italic = gTrue;
            }
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

    len = lenA;
    chars = new GList();

    //text = (Unicode *)gmallocn(len, sizeof(Unicode));
    edge = (double *) gmallocn(len + 1, sizeof(double));
    charPos = (int *) gmallocn(len + 1, sizeof(int));
    rot = rotA;
    if (rot & 1) {
        ch = (TextChar *) charsA->get(start);
        xMin = ch->xMin;
        xMax = ch->xMax;
        yMin = ch->yMin;
        ch = (TextChar *) charsA->get(start + len - 1);
        yMax = ch->yMax;
        base = xMin - descent;
        baseYmin = xMin;
    } else {
        ch = (TextChar *) charsA->get(start);
        xMin = ch->xMin;
        yMin = ch->yMin;
        yMax = ch->yMax;
        ch = (TextChar *) charsA->get(start + len - 1);
        xMax = ch->xMax;
        base = yMin + ascent;
        baseYmin = yMin;
    }

    int i = 0, j = 0;
    GBool overlap;
    double delta;
    while (j < len) {

        GBool isUpdateAccentedChar = gFalse;
        ModifierClass leftClass = NOT_A_MODIFIER, rightClass = NOT_A_MODIFIER;

        ch = (TextChar *) charsA->get(rot >= 2 ? start + len - 1 - j : start + j);
        if (j > 0) {
            chPrev = (TextChar *) charsA->get(rot >= 2 ? start + len - 1 - j : start + j - 1);
            delta = ch->rot % 2 == 0 ? ch->xMin - chPrev->xMin : ch->yMin - chPrev->yMin;

            //compute overlaping
            overlap = fabs(delta) < dupMaxPriDelta * ch->fontSize && fabs(ch->yMin
                                                                          - chPrev->yMin) <
                                                                     dupMaxSecDelta * ch->fontSize;
            //Do char composition + Update coords and char
            //compose here somehow
            rightClass = classifyChar(ch->c);
            leftClass = classifyChar(chPrev->c);
            // if one of the left or right characters is a modifier and they overlap.
            if ((rightClass != NOT_A_MODIFIER || leftClass != NOT_A_MODIFIER) && overlap) {

                Unicode diactritic = 0;
                UnicodeString *baseChar;
                UnicodeString *diacriticChar;
                UnicodeString resultChar;

                if (rightClass != NOT_A_MODIFIER) {
                    if (leftClass == NOT_A_MODIFIER) {
                        diactritic = getCombiningDiacritic(rightClass);
                        baseChar = new UnicodeString(wchar_t(getStandardBaseChar(chPrev->c)));
                    }
                } else if (leftClass != NOT_A_MODIFIER) {
                    diactritic = getCombiningDiacritic(leftClass);
                    baseChar = new UnicodeString(wchar_t(getStandardBaseChar(ch->c)));
                }

                if (diactritic != 0) {
                    diacriticChar = new UnicodeString(wchar_t(diactritic));
                    UErrorCode errorCode = U_ZERO_ERROR;
                    const Normalizer2 *nfkc = Normalizer2::getNFKCInstance(errorCode);
                    if (!nfkc->isNormalized(*baseChar, errorCode)) {
                        resultChar = nfkc->normalize(*baseChar, errorCode);
                        resultChar = nfkc->normalizeSecondAndAppend(resultChar, *diacriticChar, errorCode);
                    } else
                        resultChar = nfkc->normalizeSecondAndAppend(*baseChar, *diacriticChar, errorCode);
                    --i;
                    //text[i] = resultChar.charAt(0);
                    ((TextChar *) chars->get(i))->c = resultChar.charAt(0);
                    isUpdateAccentedChar = gTrue;
                }
            }
        }

        if (!isUpdateAccentedChar)
            //text[i] = ch->c;
            chars->append(ch);


        charPos[i] = ch->charPos;
        if (i == len - 1) {
            charPos[len] = ch->charPos + ch->charLen;
        }
        switch (rot) {
            case 0:
            default:
                edge[i] = ch->xMin;
                if (i == len - 1) {
                    edge[len] = ch->xMax;
                }
                break;
            case 1:
                edge[i] = ch->yMin;
                if (i == len - 1) {
                    edge[len] = ch->yMax;
                }
                break;
            case 2:
                edge[i] = ch->xMax;
                if (i == len - 1) {
                    edge[len] = ch->xMin;
                }
                break;
            case 3:
                edge[i] = ch->yMax;
                if (i == len - 1) {
                    edge[len] = ch->yMin;
                }
                break;
        }
        ++j;
        ++i;
    }

    if (j == len)
        len = i;

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

Unicode TextWord::getChar(int idx) { return ((TextChar *) chars->get(idx))->c; }

void TextWord::setLineNumber(bool theBool) {
    lineNumber = theBool;
}

TextWord::TextWord(TextWord *word) {
    *this = *word;
//    text = (Unicode *)gmallocn(len, sizeof(Unicode));
//    memcpy(text, word->text, len * sizeof(Unicode));
    chars = word->chars->copy();
    edge = (double *) gmallocn(len + 1, sizeof(double));
    memcpy(edge, word->edge, (len + 1) * sizeof(double));
    charPos = (int *) gmallocn(len + 1, sizeof(int));
    memcpy(charPos, word->charPos, (len + 1) * sizeof(int));
}

TextWord::~TextWord() {
    //gfree(text);
    gfree(edge);
    gfree(charPos);
}

//------------------------------------------------------------------------
// TextRawWord
//------------------------------------------------------------------------

TextRawWord::TextRawWord(GfxState *state, double x0, double y0,
                         TextFontInfo *fontA, double fontSizeA, int idCurrentWord,
                         int index) {
    GfxFont *gfxFont;
    double x, y, ascent, descent;

    charLen = 0;
    font = fontA;
    italic = gFalse;
    bold = gFalse;
    serif = gFalse;
    symbolic = gFalse;
    lineNumber = false;

    double *fontm;
    double m[4];
    double m2[4];
    double tan;
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
    tan = m[2] / m[0];
    // Get the angle value in radian
    tan = atan(tan);
    // To convert radian angle to degree angle
    tan = 180 * tan / M_PI;

    angle = static_cast<int>(tan);

    // Adjust the angle value
    switch (rot) {
        case 0:
            if (angle > 0)
                angle = 360 - angle;
            else
                angle = static_cast<int>(fabs(static_cast<double>(angle)));
            break;
        case 1:
            if (angle > 0)
                angle = 180 - angle;
            else
                angle = static_cast<int>(fabs(static_cast<double>(angle)));
            break;
        case 2:
            if (angle > 0)
                angle = 180 - angle;
            else
                angle = static_cast<int>(fabs(static_cast<double>(angle))) + 180;
            break;
        case 3:
            if (angle > 0)
                angle = 360 - angle;
            else
                angle = static_cast<int>(fabs(static_cast<double>(angle))) + 180;
            break;
    }

    // Recover the skewing angle value
    if (m[1] == 0 && m[2] != 0) {
        double angSkew = atan(m[2]);
        angSkew = (180 * angSkew / M_PI) - 90;
        angleSkewing_Y = static_cast<int>(angSkew);
        if (rot == 0) {
            angle = 0;
        }
    }

    if (m[1] != 0 && m[2] == 0) {
        double angSkew = atan(m[1]);
        angSkew = 180 * angSkew / M_PI;
        angleSkewing_X = static_cast<int>(angSkew);
        if (rot == 0) {
            angle = 0;
        }
    }

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
            char *localLowerFontName = state->getFont()->getName()->lowerCase()->getCString();
            if (strstr(localLowerFontName, "bold") ||
                strstr(localLowerFontName, "_bd"))
                bold = gTrue;
            if (strstr(localLowerFontName, "italic") ||
                strstr(localLowerFontName, "oblique") ||
                strstr(localLowerFontName, "_it"))
                italic = gTrue;
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

    //text = NULL;
    chars = new GList();
    charPos = NULL;
    edge = NULL;
    len = size = 0;

    GfxRGB rgb;

    if ((state->getRender() & 3) == 1) {
        state->getStrokeRGB(&rgb);
    } else {
        state->getFillRGB(&rgb);
    }

    colorR = colToDbl(rgb.r);
    colorG = colToDbl(rgb.g);
    colorB = colToDbl(rgb.b);

    isNonUnicodeGlyph = gFalse;
}

TextRawWord::~TextRawWord() {
    //gfree(text);
    gfree(edge);
}

void TextRawWord::setLineNumber(bool theBool) {
    lineNumber = theBool;
}

Unicode TextRawWord::getChar(int idx) { return ((TextChar *) chars->get(idx))->c; }

void TextRawWord::addChar(GfxState *state, double x, double y, double dx,
                          double dy, Unicode u, CharCode charCodeA, int charPosA, GBool overlap, TextFontInfo *fontA,
                          double fontSizeA, SplashFont *splashFont, int rotA, int nBytes, GBool isNonUnicodeGlyph) {

    if (len == size) {
        size += 16;
        //text = (Unicode *)grealloc(text, size * sizeof(Unicode));
        edge = (double *) grealloc(edge, (size + 1) * sizeof(double));
        //charPos = (int *)grealloc(charPos, size + 1 * sizeof(int));
        //chars = new GList();
    }

    GfxRGB rgb;
    double alpha;
    GBool isUpdateAccentedChar = gFalse;

    if (overlap && len > 0) {
        ModifierClass leftClass = NOT_A_MODIFIER, rightClass = NOT_A_MODIFIER;
        //Unicode prvChar = text[len - 1];
        Unicode prvChar = ((TextChar *) chars->get(len - 1))->c;
        leftClass = classifyChar(prvChar);
        rightClass = classifyChar(u);

        if (leftClass != NOT_A_MODIFIER || rightClass != NOT_A_MODIFIER) {
            Unicode diactritic = 0;
            UnicodeString *baseChar;
            UnicodeString resultChar;
            UnicodeString *diacriticChar;

            if (leftClass != NOT_A_MODIFIER) {
                if (rightClass == NOT_A_MODIFIER) {
                    diactritic = getCombiningDiacritic(leftClass);
                    baseChar = new UnicodeString(wchar_t(getStandardBaseChar(u)));
                }
            } else if (rightClass != NOT_A_MODIFIER) {
                diactritic = getCombiningDiacritic(rightClass);
                baseChar = new UnicodeString(wchar_t(getStandardBaseChar(prvChar)));
            }

            if (diactritic != 0) {
                diacriticChar = new UnicodeString(wchar_t(diactritic));
                UErrorCode errorCode = U_ZERO_ERROR;
                const Normalizer2 *nfkc = Normalizer2::getNFKCInstance(errorCode);
                if (!nfkc->isNormalized(*baseChar, errorCode)) {
                    resultChar = nfkc->normalize(*baseChar, errorCode);
                    resultChar = nfkc->normalizeSecondAndAppend(resultChar, *diacriticChar, errorCode);
                } else
                    resultChar = nfkc->normalizeSecondAndAppend(*baseChar, *diacriticChar, errorCode);
                //text[len - 1] = resultChar.charAt(0);
                ((TextChar *) chars->get(len - 1))->c = resultChar.charAt(0);
                //here we should compare both coords and keep surrounding ones
                switch (rot) {
                    case 0:
                        if (len == 0) {
                            xMin = x;
                        }
                        edge[len] = x;
                        xMax = edge[len + 1] = x + dx;
                        break;
                    case 3:
                        if (len == 0) {
                            yMin = y;
                        }
                        edge[len] = y;
                        yMax = edge[len + 1] = y + dy;
                        break;
                    case 2:
                        if (len == 0) {
                            xMax = x;
                        }
                        edge[len] = x;
                        xMin = edge[len + 1] = x + dx;
                        break;
                    case 1:
                        if (len == 0) {
                            yMax = y;
                        }
                        edge[len] = y;
                        yMin = edge[len + 1] = y + dy;
                        break;
                }
            }
            isUpdateAccentedChar = gTrue;
        }
    }

    double xxMin, xxMax, yyMin, yyMax, ascent, descent;
    ascent = fontA->ascent * fontSizeA;
    descent = fontA->descent * fontSizeA;
    switch (rot) {
        case 0:
        default:
            xxMin = x;
            xxMax = x + dx;
            yyMin = y - ascent;
            yyMax = y - descent;
            break;
        case 1:
            xxMin = x + descent;
            xxMax = x + ascent;
            yyMin = y;
            yyMax = y + dy;
            break;
        case 2:
            xxMin = x + dx;
            xxMax = x;
            yyMin = y + descent;
            yyMax = y + ascent;
            break;
        case 3:
            xxMin = x - ascent;
            xxMax = x - descent;
            yyMin = y + dy;
            yyMax = y;
            break;
    }
    if (!isUpdateAccentedChar) {
        //text[len] = u;
        if ((state->getRender() & 3) == 1) {
            state->getStrokeRGB(&rgb);
            alpha = state->getStrokeOpacity();
        } else {
            state->getFillRGB(&rgb);
            alpha = state->getFillOpacity();
        }

        GBool clipped = gFalse;
        GfxState *charState = state->copy();
        //this is workaround since copy doesnt is not deep
        charState->setCTM(state->getCTM()[0], state->getCTM()[1], state->getCTM()[2], state->getCTM()[3],
                          state->getCTM()[4], state->getCTM()[5]);
        charState->setTextMat(state->getTextMat()[0], state->getTextMat()[1], state->getTextMat()[2],
                              state->getTextMat()[3], state->getTextMat()[4], state->getTextMat()[5]);
        charState->setHorizScaling(state->getHorizScaling());

        chars->append(new TextChar(charState, u, charCodeA, charPosA, nBytes, xxMin, yyMin, xxMax, yyMax,
                                   rotA, clipped,
                                   state->getRender() == 3 || alpha < 0.001,
                                   fontA, fontSizeA, splashFont,
                                   colToDbl(rgb.r), colToDbl(rgb.g),
                                   colToDbl(rgb.b), isNonUnicodeGlyph));
        switch (rot) {
            case 0:
                if (len == 0) {
                    xMin = x;
                }
                edge[len] = x;
                xMax = edge[len + 1] = x + dx;
                break;
            case 3:
                if (len == 0) {
                    yMin = y;
                }
                edge[len] = y;
                yMax = edge[len + 1] = y + dy;
                break;
            case 2:
                if (len == 0) {
                    xMax = x;
                }
                edge[len] = x;
                xMin = edge[len + 1] = x + dx;
                break;
            case 1:
                if (len == 0) {
                    yMax = y;
                }
                edge[len] = y;
                yMin = edge[len + 1] = y + dy;
                break;
        }
        ++len;
    }

}

void TextRawWord::merge(TextRawWord *word) {
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
        //text = (Unicode *)grealloc(text, size * sizeof(Unicode));
        edge = (double *) grealloc(edge, (size + 1) * sizeof(double));
    }
    for (i = 0; i < word->len; ++i) {
        chars->append(((TextChar *) word->chars->get(i)));
        edge[len + i] = word->edge[i];
    }
    edge[len + word->len] = word->edge[word->len];
    len += word->len;
    charLen += word->charLen;
}

inline int TextRawWord::primaryCmp(TextRawWord *word) {
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
    return cmp < 0 ? -1 : cmp > 0 ? 1 : 0;
}

double TextRawWord::primaryDelta(TextRawWord *word) {
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

GBool TextRawWord::overlap(TextRawWord *w2) {

    return gFalse;
}

void IWord::setContainNonUnicodeGlyph(GBool containNonUnicodeGlyph) {
    isNonUnicodeGlyph = containNonUnicodeGlyph;
}

//------------------------------------------------------------------------
// TextRawWord
//------------------------------------------------------------------------

int IWord::cmpX(const void *p1, const void *p2) {
    const TextRawWord *word1 = *(const TextRawWord **) p1;
    const TextRawWord *word2 = *(const TextRawWord **) p2;

    if (word1->xMin < word2->xMin) {
        return -1;
    } else if (word1->xMin > word2->xMin) {
        return 1;
    } else {
        return 0;
    }
}

int IWord::cmpY(const void *p1, const void *p2) {
    const TextRawWord *word1 = *(const TextRawWord **) p1;
    const TextRawWord *word2 = *(const TextRawWord **) p2;

    if (word1->yMin < word2->yMin) {
        return -1;
    } else if (word1->yMin > word2->yMin) {
        return 1;
    } else {
        return 0;
    }
}

int IWord::cmpYX(const void *p1, const void *p2) {
    TextRawWord *word1 = *(TextRawWord **) p1;
    TextRawWord *word2 = *(TextRawWord **) p2;
    double cmp;

    cmp = word1->yMin - word2->yMin;
    if (cmp == 0) {
        cmp = word1->xMin - word2->xMin;
    }
    return cmp < 0 ? -1 : cmp > 0 ? 1 : 0;
}

GString *IWord::convtoX(double xcol) const {
    GString *xret = new GString();
    char tmp;
    unsigned int k;
    k = static_cast<int>(xcol);
    k = k / 16;
    if ((k >= 0) && (k < 10))
        tmp = (char) ('0' + k);
    else
        tmp = (char) ('a' + k - 10);
    xret->append(tmp);
    k = static_cast<int>(xcol);
    k = k % 16;
    if ((k >= 0) && (k < 10))
        tmp = (char) ('0' + k);
    else
        tmp = (char) ('a' + k - 10);
    xret->append(tmp);

    return xret;
}


GString *IWord::colortoString() const {
    GString *tmp = new GString("#");
    GString *tmpr = convtoX(static_cast<int>(255 * colorR));
    GString *tmpg = convtoX(static_cast<int>(255 * colorG));
    GString *tmpb = convtoX(static_cast<int>(255 * colorB));
    tmp->append(tmpr);
    tmp->append(tmpg);
    tmp->append(tmpb);
    delete tmpr;
    delete tmpg;
    delete tmpb;
    return tmp;
}

const char *IWord::normalizeFontName(char *fontName) {
    string name(fontName);
    string name2;
    string name3;
    char *cstr;

    size_t position = name.find_first_of('+');
    if (position != string::npos) {
        name2 = name.substr(position + 1, name.size() - 1);
    } else {
        name2 = fontName;
    }
    position = name2.find_first_of('-');
    if (position != string::npos) {
        name3 = name2.substr(0, position);
    } else {
        name3 = name2;
    }
    cstr = new char[name3.size() + 1];
    strcpy(cstr, name3.c_str());
//        printf("\t%s\t%s\n",fontName,cstr);
    return cstr;

}


//------------------------------------------------------------------------
// TextLine
//------------------------------------------------------------------------

TextLine::TextLine() {
    xMin = yMin = xMax = yMax = 0;
}

TextLine::TextLine(GList *wordsA, double xMinA, double yMinA,
                   double xMaxA, double yMaxA, double fontSizeA) {
    TextWord *word;
    int i, j, k;

    words = wordsA;
    rot = 0;
    xMin = xMinA;
    yMin = yMinA;
    xMax = xMaxA;
    yMax = yMaxA;
    fontSize = fontSizeA;
    px = 0;
    pw = 0;

    // build the text
    len = 0;
    for (i = 0; i < wordsA->getLength(); ++i) {
        word = (TextWord *) wordsA->get(i);
        len += word->getLength();
        if (word->spaceAfter) {
            ++len;
        }
    }
    text = (Unicode *) gmallocn(len, sizeof(Unicode));
    edge = (double *) gmallocn(len + 1, sizeof(double));
    j = 0;
    for (i = 0; i < words->getLength(); ++i) {
        word = (TextWord *) words->get(i);
        if (i == 0) {
            rot = word->rot;
        }
        for (k = 0; k < word->getLength(); ++k) {
            text[j] = ((TextChar *) word->chars->get(k))->c;
            edge[j] = word->edge[k];
            ++j;
        }
        edge[j] = word->edge[word->getLength()];
        if (word->spaceAfter) {
            text[j] = (Unicode) 0x0020;
            ++j;
            edge[j] = edge[j - 1];
        }
    }
    //~ need to check for other Unicode chars used as hyphens
    hyphenated = text[len - 1] == (Unicode) '-';
}

TextLine::~TextLine() {
    deleteGList(words, TextWord);
    gfree(text);
    gfree(edge);
}

//double TextLine::getBaseline() {
//    return ((TextWord *)words->get(0))->getBaseline();
//}

int TextLine::cmpX(const void *p1, const void *p2) {
    const TextLine *line1 = *(const TextLine **) p1;
    const TextLine *line2 = *(const TextLine **) p2;

    if (line1->xMin < line2->xMin) {
        return -1;
    } else if (line1->xMin > line2->xMin) {
        return 1;
    } else {
        return 0;
    }
}

//------------------------------------------------------------------------
// TextParagraph
//------------------------------------------------------------------------

TextParagraph::TextParagraph() {
    xMin = yMin = xMax = yMax = 0;
}

TextParagraph::TextParagraph(GList *linesA, GBool dropCapA) {
    TextLine *line;
    int i;

    lines = linesA;
    dropCap = dropCapA;
    xMin = yMin = xMax = yMax = 0;
    for (i = 0; i < lines->getLength(); ++i) {
        line = (TextLine *) lines->get(i);
        if (i == 0 || line->xMin < xMin) {
            xMin = line->xMin;
        }
        if (i == 0 || line->yMin < yMin) {
            yMin = line->yMin;
        }
        if (i == 0 || line->xMax > xMax) {
            xMax = line->xMax;
        }
        if (i == 0 || line->yMax > yMax) {
            yMax = line->yMax;
        }
    }
}

TextParagraph::~TextParagraph() {
    deleteGList(lines, TextLine);
}

//------------------------------------------------------------------------
// TextColumn
//------------------------------------------------------------------------

TextColumn::TextColumn(GList *paragraphsA, double xMinA, double yMinA,
                       double xMaxA, double yMaxA) {
    paragraphs = paragraphsA;
    xMin = xMinA;
    yMin = yMinA;
    xMax = xMaxA;
    yMax = yMaxA;
    px = py = 0;
    pw = ph = 0;
}

TextColumn::~TextColumn() {
    deleteGList(paragraphs, TextParagraph);
}

int TextColumn::getRotation() {
    TextParagraph *par;
    TextLine *line;

    par = (TextParagraph *) paragraphs->get(0);
    line = (TextLine *) par->getLines()->get(0);
    return line->getRotation();
}

int TextColumn::cmpX(const void *p1, const void *p2) {
    const TextColumn *col1 = *(const TextColumn **) p1;
    const TextColumn *col2 = *(const TextColumn **) p2;

    if (col1->xMin < col2->xMin) {
        return -1;
    } else if (col1->xMin > col2->xMin) {
        return 1;
    } else {
        return 0;
    }
}

int TextColumn::cmpY(const void *p1, const void *p2) {
    const TextColumn *col1 = *(const TextColumn **) p1;
    const TextColumn *col2 = *(const TextColumn **) p2;

    if (col1->yMin < col2->yMin) {
        return -1;
    } else if (col1->yMin > col2->yMin) {
        return 1;
    } else {
        return 0;
    }
}

int TextColumn::cmpPX(const void *p1, const void *p2) {
    const TextColumn *col1 = *(const TextColumn **) p1;
    const TextColumn *col2 = *(const TextColumn **) p2;

    if (col1->px < col2->px) {
        return -1;
    } else if (col1->px > col2->px) {
        return 1;
    } else {
        return 0;
    }
}

//------------------------------------------------------------------------
// TextBlock
//------------------------------------------------------------------------

enum TextBlockType {
    blkVertSplit,
    blkHorizSplit,
    blkLeaf
};

enum TextBlockTag {
    blkTagMulticolumn,
    blkTagColumn,
    blkTagSuperLine,
    blkTagLine
};

class TextBlock {
public:

    TextBlock(TextBlockType typeA, int rotA);

    ~TextBlock();

    void addChild(TextBlock *child);

    void addChild(TextChar *child, GBool updateBox);

    void prependChild(TextChar *child);

    void updateBounds(int childIdx);

    TextBlockType type;
    TextBlockTag tag;
    int rot;
    double xMin, yMin, xMax, yMax;
    GBool smallSplit;        // true for blkVertSplit/blkHorizSplit
    //   where the gap size is small
    GList *children;        // for blkLeaf, children are TextWord;
    //   for others, children are TextBlock
};

TextBlock::TextBlock(TextBlockType typeA, int rotA) {
    type = typeA;
    tag = blkTagMulticolumn;
    rot = rotA;
    xMin = yMin = xMax = yMax = 0;
    smallSplit = gFalse;
    children = new GList();
}

TextBlock::~TextBlock() {
    if (type == blkLeaf) {
        delete children;
    } else {
        deleteGList(children, TextBlock);
    }
}

void TextBlock::addChild(TextBlock *child) {
    if (children->getLength() == 0) {
        xMin = child->xMin;
        yMin = child->yMin;
        xMax = child->xMax;
        yMax = child->yMax;
    } else {
        if (child->xMin < xMin) {
            xMin = child->xMin;
        }
        if (child->yMin < yMin) {
            yMin = child->yMin;
        }
        if (child->xMax > xMax) {
            xMax = child->xMax;
        }
        if (child->yMax > yMax) {
            yMax = child->yMax;
        }
    }
    children->append(child);
}

void TextBlock::addChild(TextChar *child, GBool updateBox) {
    if (updateBox) {
        if (children->getLength() == 0) {
            xMin = child->xMin;
            yMin = child->yMin;
            xMax = child->xMax;
            yMax = child->yMax;
        } else {
            if (child->xMin < xMin) {
                xMin = child->xMin;
            }
            if (child->yMin < yMin) {
                yMin = child->yMin;
            }
            if (child->xMax > xMax) {
                xMax = child->xMax;
            }
            if (child->yMax > yMax) {
                yMax = child->yMax;
            }
        }
    }
    children->append(child);
}

void TextBlock::prependChild(TextChar *child) {
    if (children->getLength() == 0) {
        xMin = child->xMin;
        yMin = child->yMin;
        xMax = child->xMax;
        yMax = child->yMax;
    } else {
        if (child->xMin < xMin) {
            xMin = child->xMin;
        }
        if (child->yMin < yMin) {
            yMin = child->yMin;
        }
        if (child->xMax > xMax) {
            xMax = child->xMax;
        }
        if (child->yMax > yMax) {
            yMax = child->yMax;
        }
    }
    children->insert(0, child);
}

void TextBlock::updateBounds(int childIdx) {
    TextBlock *child;

    child = (TextBlock *) children->get(childIdx);
    if (child->xMin < xMin) {
        xMin = child->xMin;
    }
    if (child->yMin < yMin) {
        yMin = child->yMin;
    }
    if (child->xMax > xMax) {
        xMax = child->xMax;
    }
    if (child->yMax > yMax) {
        yMax = child->yMax;
    }
}
//------------------------------------------------------------------------
// TextPage
//------------------------------------------------------------------------

TextPage::TextPage(GBool verboseA, Catalog *catalog, xmlNodePtr node,
                   GString *dir, GString *base, GString *nsURIA) {

    root = node;
    verbose = verboseA;
    //rawOrder = 1;

    // PL: to modify block order according to reading order
    if (parameters->getReadingOrder() == gTrue) {
        readingOrder = 1;
    } else {
        readingOrder = 0;
    }
    chars = new GList();
    words = new GList();

    curRot = 0;
    diagonal = gFalse;

    actualText = NULL;
    actualTextLen = 0;
    actualTextX0 = 0;
    actualTextY0 = 0;
    actualTextX1 = 0;
    actualTextY1 = 0;
    actualTextNBytes = 0;

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

//    rawWords = NULL;
//    rawLastWord = NULL;
    fonts = new GList();
    lastFindXMin = lastFindYMin = 0;
    haveLastFind = gFalse;

    if (parameters->getDisplayImage()) {
        GString *cp;
        cp = dir->copy();
        for (int i = 0; i < cp->getLength(); i++) {
            if (cp->getChar(i) == ' ') {
                cp->del(i);
                cp->insert(i, "%20");
            }
        }
        dataDirectory = new GString(cp);
    }
}

TextPage::~TextPage() {
    clear();
    delete fonts;
    if (namespaceURI) {
        delete namespaceURI;
    }
}

/** Set if the page contains a column of line numbers*/
void TextPage::setLineNumber(bool theBool) {
    lineNumber = theBool;
}

/** get the presence of a column of line numbers in the text page */
bool TextPage::getLineNumber() {
    return lineNumber;
}

void TextPage::startPage(int pageNum, GfxState *state, GBool cut) {
    clear();
    words = new GList();
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

    page = xmlNewNode(NULL, (const xmlChar *) TAG_PAGE);
    page->type = XML_ELEMENT_NODE;
    if (state) {
        pageWidth = state->getPageWidth();
        pageHeight = state->getPageHeight();
    } else {
        pageWidth = pageHeight = 0;
    }
    curstate = state;

    tmp = (char *) malloc(20 * sizeof(char));

    sprintf(tmp, "%d", pageNum);

    GString *id;
    id = new GString("Page");
    id->append(tmp);
    xmlNewProp(page, (const xmlChar *) ATTR_ID, (const xmlChar *) id->getCString());
    delete id;

    xmlNewProp(page, (const xmlChar *) ATTR_PHYSICAL_IMG_NR, (const xmlChar *) tmp);

    snprintf(tmp, sizeof(tmp), ATTR_NUMFORMAT, pageWidth);
    xmlNewProp(page, (const xmlChar *) ATTR_WIDTH, (const xmlChar *) tmp);
    snprintf(tmp, sizeof(tmp), ATTR_NUMFORMAT, pageHeight);
    xmlNewProp(page, (const xmlChar *) ATTR_HEIGHT, (const xmlChar *) tmp);

    // Cut all pages OK
    if (cutter) {
        docPage = xmlNewDoc((const xmlChar *) VERSION);
        globalParams->setTextEncoding((char *) ENCODING_UTF8);
        docPage->encoding = xmlStrdup((const xmlChar *) ENCODING_UTF8);
        xmlDocSetRootElement(docPage, page);
    } else {
        xmlAddChild(root, page);
    }

    //  	fprintf(stderr, "Page %d\n",pageNum);
    //  	fflush(stderr);

    // New file for vectorials instructions
    vecdoc = xmlNewDoc((const xmlChar *) VERSION);
    globalParams->setTextEncoding((char *) ENCODING_UTF8);
    vecdoc->encoding = xmlStrdup((const xmlChar *) ENCODING_UTF8);
    vecroot = xmlNewNode(NULL, (const xmlChar *) TAG_SVG);
    xmlNewNs(vecroot, (const xmlChar *) "http://www.w3.org/2000/svg", NULL);

    // Add the namespace DS of the vectorial instructions file
    if (namespaceURI) {
        xmlNewNs(vecroot, (const xmlChar *) namespaceURI->getCString(), NULL);
    }

    xmlDocSetRootElement(vecdoc, vecroot);

    svg_xmax=0, svg_xmin=0, svg_ymax=0, svg_ymin =0;
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

    // annotations
    currentPage = myCat->getPage(num);
    currentPage->getAnnots(&objAnnot);
    //pageLinks = currentPage->getLinks(myCat);
    //pageLinks = currentPage->getLinks();

    // Annotation's objects list
    if (objAnnot.isArray()) {
        for (int i = 0; i < objAnnot.arrayGetLength(); ++i) {
            objAnnot.arrayGet(i, &kid);
            if (kid.isDict()) {
                Dict *dict;
                dict = kid.getDict();
                // Get the annotation's type
                if (dict->lookup("Subtype", &objSubtype)->isName()) {
                    // It can be 'Highlight' or 'Underline' or 'Link' (Subtype 'Squiggly' or 'StrikeOut' are not supported)
                    if (!strcmp(objSubtype.getName(), "Highlight")) {
                        highlightedObject.push_back(dict);
                    }
                    if (!strcmp(objSubtype.getName(), "Underline")) {
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
    if (!readingOrder) {
        primaryRot = 0;
    }
}

void TextPage::endPage(GString *dataDir) {
    if (curWord) {
        endWord();
    }
    highlightedObject.clear();
    underlineObject.clear();
}

void TextPage::clear() {
    //TextRawWord *word;

    if (curWord) {
        delete curWord;
        curWord = NULL;
    }
    if (chars->getLength() > 0) {
        deleteGList(chars, TextChar);
        chars = new GList();
    }

    if (words->getLength() > 0) {
        deleteGList(words, TextRawWord);
        words = new GList();
//        while (rawWords) {
//            word = rawWords;
//            rawWords = rawWords->next;
//            delete word;
//        }
    }

    curRot = 0;
    diagonal = gFalse;
    deleteGList(fonts, TextFontInfo);

    gfree(actualText);
    actualText = NULL;
    actualTextLen = 0;
    actualTextNBytes = 0;

    curWord = NULL;
    charPos = 0;
    curFont = NULL;
    curFontSize = 0;
    nest = 0;
    nTinyChars = 0;

//    rawWords = NULL;
//    rawLastWord = NULL;
    fonts = new GList();

    // Clear the vector which contain images inline objects
    int nb = listeImageInline.size();
    for (int i = 0; i < nb; i++) {
        delete listeImageInline[i];
    }
    listeImageInline.clear();

    // Clear the vector which contain images objects
    nb = listeImages.size();
    for (int i = 0; i < nb; i++) {
        delete listeImages[i];
    }
    listeImages.clear();
}

void TextPage::beginActualText(GfxState *state, Unicode *u, int uLen) {
    if (actualText) {
        gfree(actualText);
    }
    actualText = (Unicode *) gmallocn(uLen, sizeof(Unicode));
    memcpy(actualText, u, uLen * sizeof(Unicode));
    actualTextLen = uLen;
    actualTextNBytes = 0;
}

void TextPage::endActualText(GfxState *state, SplashFont *splashFont) {
    Unicode *u;

    u = actualText;
    actualText = NULL;  // so we can call TextPage::addChar()
    if (actualTextNBytes) {
        // now that we have the position info for all of the text inside
        // the marked content span, we feed the "ActualText" back through
        // addChar()
        addChar(state, actualTextX0, actualTextY0,
                actualTextX1 - actualTextX0, actualTextY1 - actualTextY0,
                0, actualTextNBytes, u, actualTextLen, splashFont, gFalse);
    }
    gfree(u);
    actualText = NULL;
    actualTextLen = 0;
    actualTextNBytes = gFalse;
}

void TextPage::updateFont(GfxState *state) {

    GfxFont *gfxFont;
    double *fm;
    char *name;
    int code, mCode, letterCode, anyCode;
    double m[4], m2[4];
    double w;
    int i;

    // get the font info object
    curFont = NULL;
    for (i = 0; i < fonts->getLength(); ++i) {
        curFont = (TextFontInfo *) fonts->get(i);

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
            name = ((Gfx8BitFont *) gfxFont)->getCharName(code);
            if (name && name[0] == 'm' && name[1] == '\0') {
                mCode = code;
            }
            if (letterCode < 0 && name && name[1] == '\0' && ((name[0] >= 'A'
                                                               && name[0] <= 'Z') ||
                                                              (name[0] >= 'a' && name[0] <= 'z'))) {
                letterCode = code;
            }
            if (anyCode < 0 && name && ((Gfx8BitFont *) gfxFont)->getWidth(code) > 0) {
                anyCode = code;
            }
        }
        if (mCode >= 0 &&
            (w = ((Gfx8BitFont *) gfxFont)->getWidth(mCode)) > 0) {
            // 0.6 is a generic average 'm' width -- yes, this is a hack
            curFontSize *= w / 0.6;
        } else if (letterCode >= 0 &&
                   (w = ((Gfx8BitFont *) gfxFont)->getWidth(letterCode)) > 0) {
            // even more of a hack: 0.5 is a generic letter width
            curFontSize *= w / 0.5;
        } else if (anyCode >= 0 &&
                   (w = ((Gfx8BitFont *) gfxFont)->getWidth(anyCode)) > 0) {
            // better than nothing: 0.5 is a generic character width
            curFontSize *= w / 0.5;
        }
        fm = gfxFont->getFontMatrix();
        if (fm[0] != 0) {
            curFontSize *= fabs(fm[3] / fm[0]);
        }
    }

    // compute the rotation
    state->getFontTransMat(&m[0], &m[1], &m[2], &m[3]);
    if (gfxFont && gfxFont->getType() == fontType3) {
        fm = gfxFont->getFontMatrix();
        m2[0] = fm[0] * m[0] + fm[1] * m[2];
        m2[1] = fm[0] * m[1] + fm[1] * m[3];
        m2[2] = fm[2] * m[0] + fm[3] * m[2];
        m2[3] = fm[2] * m[1] + fm[3] * m[3];
        m[0] = m2[0];
        m[1] = m2[1];
        m[2] = m2[2];
        m[3] = m2[3];
    }
    if (fabs(m[0]) >= fabs(m[1])) {
        if (m[0] > 0) {
            curRot = 0;
        } else {
            curRot = 2;
        }
        diagonal = fabs(m[1]) > diagonalThreshold * fabs(m[0]);
    } else {
        if (m[1] > 0) {
            curRot = 1;
        } else {
            curRot = 3;
        }
        diagonal = fabs(m[0]) > diagonalThreshold * fabs(m[1]);
    }
}

void TextPage::beginWord(GfxState *state, double x0, double y0) {

    // This check is needed because Type 3 characters can contain
    // text-drawing operations (when TextPage is being used via
    // {X,Win}SplashOutputDev rather than TextOutputDev).
    if (curWord) {
        ++nest;
        return;
    }

    // Increment the absolute object index
    idx++;
    curWord = new TextRawWord(state, x0, y0, curFont, curFontSize, getIdWORD(), getIdx());
}

ModifierClass TextPage::classifyChar(Unicode u) {
    switch (u) {
        case (Unicode) 776: //COMBINING DIAERESIS
        case (Unicode) 168: //DIAERESIS
            return DIAERESIS;

        case 833:
        case 779: // COMBINING DOUBLE_ACUTE_ACCENT
        case 733:
            return DOUBLE_ACUTE_ACCENT;
        case 180:
        case 769: //COMBINING
        case 714:
            return ACUTE_ACCENT;

        case 768: //COMBINING
        case 832:
        case 715:
        case 96:
            return GRAVE_ACCENT;

        case 783: //COMBINING
            return DOUBLE_GRAVE_ACCENT;

        case 774: //COMBINING
        case 728:
            //case '\uA67C':
            return BREVE_ACCENT;

        case 785: //COMBINING
        case 1156:
        case 1159:
            return INVERTED_BREVE_ACCENT;


        case 770: //COMBINING
        case 94:
        case 710:
            return CIRCUMFLEX;


        case 771: //COMBINING
        case 126:
        case 732:
            return TILDE;

        case 778: //COMBINING
        case 176:
        case 730:
            return NORDIC_RING;//LOOK AT UNICODE RING BELOW...

        case 780: //COMBINING
        case 711:
            return CZECH_CARON;

        case 807: //COMBINING
        case 184:
            return CEDILLA;

        case 775: //COMBINING
        case 729:
            return DOT_ABOVE;

        case 777: //COMBINING
        case 704:
            return HOOK;

        case 795: //COMBINING
            return HORN;

        case 808: //COMBINING
        case 731:
            //case '\u1AB7':// combining open mark below
            return OGONEK;

        case 772: //COMBINING
        case 175:
        case 713:
            return MACRON;
        default:
            return NOT_A_MODIFIER;
    }

}

/*
 * Returns the correct base char for composition with icu4c following unicode standard.
 */
Unicode IWord::getStandardBaseChar(Unicode c) {
    switch (c) {
        case 305:
            return 105;
        default:
            return c;
    }
}

Unicode TextPage::getCombiningDiacritic(ModifierClass modifierClass) {

    Unicode diactritic = 0;
    switch (modifierClass) {
        case DIAERESIS:
            diactritic = 776;
            break;
        case ACUTE_ACCENT:
            diactritic = 769;
            break;
        case GRAVE_ACCENT:
            diactritic = 768;
            break;
        case CIRCUMFLEX:
            diactritic = 770;
            break;
        case TILDE:
            diactritic = 771;
            break;
        case NORDIC_RING:
            diactritic = 778;
            break;
        case CZECH_CARON:
            diactritic = 780;
            break;
        case CEDILLA:
            diactritic = 807;
            break;
        case DOUBLE_ACUTE_ACCENT:
            diactritic = 779;
            break;
        case DOUBLE_GRAVE_ACCENT:
            diactritic = 783;
            break;
        case BREVE_ACCENT:
            diactritic = 785;
            break;
        case INVERTED_BREVE_ACCENT:
            diactritic = 785;
            break;
        case DOT_ABOVE:
            diactritic = 775;
            break;
        case HOOK:
            diactritic = 777;
            break;
        case HORN:
            diactritic = 795;
            break;
        case OGONEK:
            diactritic = 808;
            break;
        case MACRON:
            diactritic = 772;
            break;
        default:
            break;
    }
    return diactritic;
}

ModifierClass IWord::classifyChar(Unicode u) {
    switch (u) {
        case (Unicode) 776: //COMBINING DIAERESIS
        case (Unicode) 168: //DIAERESIS
            return DIAERESIS;

        case 833:
        case 779: // COMBINING DOUBLE_ACUTE_ACCENT
        case 733:
            return DOUBLE_ACUTE_ACCENT;
        case 180:
        case 769: //COMBINING
        case 714:
            return ACUTE_ACCENT;

        case 768: //COMBINING
        case 832:
        case 715:
        case 96:
            return GRAVE_ACCENT;

        case 783: //COMBINING
            return DOUBLE_GRAVE_ACCENT;

        case 774: //COMBINING
        case 728:
            //case '\uA67C':
            return BREVE_ACCENT;

        case 785: //COMBINING
        case 1156:
        case 1159:
            return INVERTED_BREVE_ACCENT;


        case 770: //COMBINING
        case 94:
        case 710:
            return CIRCUMFLEX;


        case 771: //COMBINING
        case 126:
        case 732:
            return TILDE;

        case 778: //COMBINING
            //case 176:
        case 730:
            return NORDIC_RING;//LOOK AT UNICODE RING BELOW...

        case 780: //COMBINING
        case 711:
            return CZECH_CARON;

        case 807: //COMBINING
        case 184:
            return CEDILLA;

        case 775: //COMBINING
        case 729:
            return DOT_ABOVE;

        case 777: //COMBINING
        case 704:
            return HOOK;

        case 795: //COMBINING
            return HORN;

        case 808: //COMBINING
        case 731:
            //case '\u1AB7':// combining open mark below
            return OGONEK;

        case 772: //COMBINING
        case 175:
        case 713:
            return MACRON;
        default:
            return NOT_A_MODIFIER;
    }

}

Unicode IWord::getCombiningDiacritic(ModifierClass modifierClass) {

    Unicode diactritic = 0;
    switch (modifierClass) {
        case DIAERESIS:
            diactritic = 776;
            break;
        case ACUTE_ACCENT:
            diactritic = 769;
            break;
        case GRAVE_ACCENT:
            diactritic = 768;
            break;
        case CIRCUMFLEX:
            diactritic = 770;
            break;
        case TILDE:
            diactritic = 771;
            break;
        case NORDIC_RING:
            diactritic = 778;
            break;
        case CZECH_CARON:
            diactritic = 780;
            break;
        case CEDILLA:
            diactritic = 807;
            break;
        case DOUBLE_ACUTE_ACCENT:
            diactritic = 779;
            break;
        case DOUBLE_GRAVE_ACCENT:
            diactritic = 783;
            break;
        case BREVE_ACCENT:
            diactritic = 785;
            break;
        case INVERTED_BREVE_ACCENT:
            diactritic = 785;
            break;
        case DOT_ABOVE:
            diactritic = 775;
            break;
        case HOOK:
            diactritic = 777;
            break;
        case HORN:
            diactritic = 795;
            break;
        case OGONEK:
            diactritic = 808;
            break;
        case MACRON:
            diactritic = 772;
            break;
        default:
            break;
    }
    return diactritic;
}


void TextPage::addCharToPageChars(GfxState *state, double x, double y, double dx,
                                  double dy, CharCode c, int nBytes, Unicode *u, int uLen, SplashFont *splashFont,
                                  GBool isNonUnicodeGlyph) {
    double x1, y1, x2, y2, w1, h1, dx2, dy2, ascent, descent, sp;
    double xMin, yMin, xMax, yMax, xMid, yMid;
    double clipXMin, clipYMin, clipXMax, clipYMax;
    GfxRGB rgb;
    double alpha;
    GBool clipped, rtl;
    int i, j;

    // if we're in an ActualText span, save the position info (the
    // ActualText chars will be added by TextPage::endActualText()).
    if (actualText) {
        if (!actualTextNBytes) {
            actualTextX0 = x;
            actualTextY0 = y;
        }
        actualTextX1 = x + dx;
        actualTextY1 = y + dy;
        actualTextNBytes += nBytes;
        return;
    }

    // throw away diagonal chars
//        if (control.discardDiagonalText && diagonal) {
//            charPos += nBytes;
//            return;
//        }

    // subtract char and word spacing from the dx,dy values
    sp = state->getCharSpace();
    if (c == (CharCode) 0x20) {
        sp += state->getWordSpace();
    }
    state->textTransformDelta(sp * state->getHorizScaling(), 0, &dx2, &dy2);
    dx -= dx2;
    dy -= dy2;
    state->transformDelta(dx, dy, &w1, &h1);

    // throw away chars that aren't inside the page bounds
    // (and also do a sanity check on the character size)
    state->transform(x, y, &x1, &y1);
    if (x1 + w1 < 0 || x1 > pageWidth ||
        y1 + h1 < 0 || y1 > pageHeight ||
        w1 > pageWidth || h1 > pageHeight) {
        charPos += nBytes;
        return;
    }

    // check the tiny chars limit
    if (!globalParams->getTextKeepTinyChars() &&
        fabs(w1) < 3 && fabs(h1) < 3) {
        if (++nTinyChars > 50000) {
            charPos += nBytes;
            return;
        }
    }

    // skip space, tab, and non-breaking space characters
    if (uLen == 1 && (u[0] == (Unicode) 0x20 ||
                      u[0] == (Unicode) 0x09 ||
                      u[0] == (Unicode) 0xa0)) {
        charPos += nBytes;
        if (chars->getLength() > 0) {
            ((TextChar *) chars->get(chars->getLength() - 1))->spaceAfter =
                    (char) gTrue;
        }
        return;
    }

    // add the characters
    if (uLen > 0) {

        // handle right-to-left ligatures: if there are multiple Unicode
        // characters, and they're all right-to-left, insert them in
        // right-to-left order
        if (uLen > 1) {
            rtl = gTrue;
            for (i = 0; i < uLen; ++i) {
                if (!unicodeTypeR(u[i])) {
                    rtl = gFalse;
                    break;
                }
            }
        } else {
            rtl = gFalse;
        }

        // compute the bounding box
        w1 /= uLen;
        h1 /= uLen;
        ascent = curFont->ascent * curFontSize;
        descent = curFont->descent * curFontSize;
        for (i = 0; i < uLen; ++i) {
            x2 = x1 + i * w1;
            y2 = y1 + i * h1;
            switch (curRot) {
                case 0:
                default:
                    xMin = x2;
                    xMax = x2 + w1;
                    yMin = y2 - ascent;
                    yMax = y2 - descent;
                    break;
                case 1:
                    xMin = x2 + descent;
                    xMax = x2 + ascent;
                    yMin = y2;
                    yMax = y2 + h1;
                    break;
                case 2:
                    xMin = x2 + w1;
                    xMax = x2;
                    yMin = y2 + descent;
                    yMax = y2 + ascent;
                    break;
                case 3:
                    xMin = x2 - ascent;
                    xMax = x2 - descent;
                    yMin = y2 + h1;
                    yMax = y2;
                    break;
            }

            // check for clipping
            clipped = gFalse;
//                if (control.clipText || control.discardClippedText) {
//                    state->getClipBBox(&clipXMin, &clipYMin, &clipXMax, &clipYMax);
//                    xMid = 0.5 * (xMin + xMax);
//                    yMid = 0.5 * (yMin + yMax);
//                    if (xMid < clipXMin || xMid > clipXMax ||
//                        yMid < clipYMin || yMid > clipYMax) {
//                        clipped = gTrue;
//                    }
//                }

            if ((state->getRender() & 3) == 1) {
                state->getStrokeRGB(&rgb);
                alpha = state->getStrokeOpacity();
            } else {
                state->getFillRGB(&rgb);
                alpha = state->getFillOpacity();
            }
            if (rtl) {
                j = uLen - 1 - i;
            } else {
                j = i;
            }
            chars->append(new TextChar(state->copy(), u[j], c, charPos, nBytes, xMin, yMin, xMax, yMax,
                                       curRot, clipped,
                                       state->getRender() == 3 || alpha < 0.001,
                                       curFont, curFontSize, splashFont,
                                       colToDbl(rgb.r), colToDbl(rgb.g),
                                       colToDbl(rgb.b), isNonUnicodeGlyph));
        }
    }

    charPos += nBytes;
}

void TextPage::addCharToRawWord(GfxState *state, double x, double y, double dx,
                                double dy, CharCode c, int nBytes, Unicode *u, int uLen, SplashFont *splashFont,
                                GBool isNonUnicodeGlyph) {
    //cout << "addCharToRawWord" << endl;
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
        y1 < 0 || y1 > pageHeight) {
        charPos += nBytes;
        endWord();
        return;
    }

    // subtract char and word spacing from the dx,dy values 
    sp = state->getCharSpace();
    if (c == (CharCode) 0x20) {
        sp += state->getWordSpace();
    }
    state->textTransformDelta(sp * state->getHorizScaling(), 0, &dx2, &dy2);
    dx -= dx2; 
    dy -= dy2; 
    state->transformDelta(dx, dy, &w1, &h1);

    // check the tiny chars limit
    if (!globalParams->getTextKeepTinyChars() && fabs(w1) < 3 && fabs(h1) < 3) {
        if (++nTinyChars > 50000) {
            charPos += nBytes;
            return;
        }
    }

    // break words at space character
    if (uLen == 1 && (u[0] == (Unicode) 0x20 ||
                      u[0] == (Unicode) 0x09 ||
                      u[0] == (Unicode) 0xa0)) {
        if (curWord) {
            ++curWord->charLen;
            curWord->setSpaceAfter(gTrue);
            if (curWord->chars->getLength() > 0)
                ((TextChar *) curWord->chars->get(curWord->chars->getLength() - 1))->spaceAfter =
                        (char) gTrue;
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
                                                                           - curWord->base) <
                                                                      dupMaxSecDelta * curWord->fontSize;

        // avoid splitting token when overlaping is surrounded by diacritic
        ModifierClass modifierClass = NOT_A_MODIFIER;
        if (curWord->len > 0)
            modifierClass = classifyChar(((TextChar *) curWord->chars->get(curWord->getLength() - 1))->c);
        if (modifierClass == NOT_A_MODIFIER)
            modifierClass = classifyChar(u[0]);
        GBool space = sp > minWordBreakSpace * curWord->fontSize;

        if(space){
            curWord->setSpaceAfter(gTrue);
            if (curWord->chars->getLength() > 0)
                ((TextChar *) curWord->chars->get(curWord->chars->getLength() - 1))->spaceAfter =
                        (char) gTrue;
        }
        // take into account rotation angle ??
        if (((overlap || fabs(base - curWord->base) > 1 ||
                space ||
              (sp < -minDupBreakOverlap * curWord->fontSize)) && modifierClass == NOT_A_MODIFIER)) {
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
            if (isNonUnicodeGlyph)
                curWord->setContainNonUnicodeGlyph(isNonUnicodeGlyph);
            curWord->addChar(state, x1 + i * w1, y1 + i * h1, w1, h1, u[i], c, charPos,
                             (overlap || sp < -minDupBreakOverlap * curWord->fontSize), curFont, curFontSize,
                             splashFont, nBytes, curRot, isNonUnicodeGlyph);
        }
    }

    if (curWord) {
        curWord->charLen += nBytes;
    }
    charPos += nBytes;

    // reinitiate word if no char added
    if(curWord->getLength()==0)
        curWord = NULL;
}

void TextPage::addChar(GfxState *state, double x, double y, double dx,
                       double dy, CharCode c, int nBytes, Unicode *u, int uLen, SplashFont *splashFont,
                       GBool isNonUnicodeGlyph) {
//    if (parameters->getReadingOrder() == gTrue)
//        addCharToPageChars(state, x, y, dx, dy, c, nBytes, u, uLen, splashFont, isNonUnicodeGlyph);
//    else
    addCharToRawWord(state, x, y, dx, dy, c, nBytes, u, uLen, splashFont, isNonUnicodeGlyph);
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
        //Here once the word is finished check, diacritics
        addWord(curWord);
        curWord = NULL;
        idWORD++;
    }
}

void TextPage::addWord(TextRawWord *word) {
    // throw away zero-length words -- they don't have valid xMin/xMax
    // values, and they're useless anyway
    if (word->len == 0) {
        delete word;
        return;
    }

//    if(readingOrder) {
    words->append(word);
//    } else {
//        if (rawLastWord) {
//            rawLastWord->next = word;
//        } else {
//            rawWords = word;
//        }
//        rawLastWord = word;
//    }
}

void TextPage::addAttributTypeReadingOrder(xmlNodePtr node, char *tmp,
                                           IWord *word) {
    if (parameters->getCharReadingOrderAttr() == gFalse) {
        return;
    }

    int nbLeft = 0;
    int nbRight = 0;

    // Recover the reading order for each characters of the word
    for (int i = 0; i < word->len; ++i) {
        if (unicodeTypeR(((TextChar *) word->chars->get(i))->c)) {
            nbLeft++;
        } else {
            nbRight++;
        }
    }
    // IF there is more character where the reading order is left to right
    // then we add the type attribute with a true value
    if (nbRight < nbLeft) {
        sprintf(tmp, "%d", gTrue);
        xmlNewProp(node, (const xmlChar *) ATTR_TYPE, (const xmlChar *) tmp);
    }
}

void TextPage::addAttributsNodeVerbose(xmlNodePtr node, char *tmp,
                                       IWord *word) {
    GString *id = new GString("p");
    xmlNewProp(node, (const xmlChar *) ATTR_SID, (const xmlChar *) buildSID(num, word->getIdx(), id)->getCString());
    delete id;
    sprintf(tmp, "%d", word->rot);
    xmlNewProp(node, (const xmlChar *) ATTR_ROTATION, (const xmlChar *) tmp);
    sprintf(tmp, "%d", word->angle);
    xmlNewProp(node, (const xmlChar *) ATTR_ANGLE, (const xmlChar *) tmp);
    sprintf(tmp, "%d", word->angleSkewing_Y);
    xmlNewProp(node, (const xmlChar *) ATTR_ANGLE_SKEWING_Y, (const xmlChar *) tmp);
    sprintf(tmp, "%d", word->angleSkewing_X);
    xmlNewProp(node, (const xmlChar *) ATTR_ANGLE_SKEWING_X, (const xmlChar *) tmp);
    sprintf(tmp, ATTR_NUMFORMAT, word->leading);
    xmlNewProp(node, (const xmlChar *) ATTR_LEADING, (const xmlChar *) tmp);
    sprintf(tmp, ATTR_NUMFORMAT, word->render);
    xmlNewProp(node, (const xmlChar *) ATTR_RENDER, (const xmlChar *) tmp);
    sprintf(tmp, ATTR_NUMFORMAT, word->rise);
    xmlNewProp(node, (const xmlChar *) ATTR_RISE, (const xmlChar *) tmp);
    sprintf(tmp, ATTR_NUMFORMAT, word->horizScaling);
    xmlNewProp(node, (const xmlChar *) ATTR_HORIZ_SCALING, (const xmlChar *) tmp);
    sprintf(tmp, ATTR_NUMFORMAT, word->wordSpace);
    xmlNewProp(node, (const xmlChar *) ATTR_WORD_SPACE, (const xmlChar *) tmp);
    sprintf(tmp, ATTR_NUMFORMAT, word->charSpace);
    xmlNewProp(node, (const xmlChar *) ATTR_CHAR_SPACE, (const xmlChar *) tmp);
}

void TextPage::addAttributsNode(xmlNodePtr node, IWord *word, TextFontStyleInfo *fontStyleInfo, UnicodeMap *uMap,
                                GBool fullFontName) {
    char *tmp;
    tmp = (char *) malloc(10 * sizeof(char));

    GString *id;
    GString *stringTemp;

    id = new GString("p");
    xmlNewProp(node, (const xmlChar *) ATTR_ID, (const xmlChar *) buildIdToken(num, numToken, id)->getCString());
    delete id;
    numToken = numToken + 1;

    stringTemp = new GString();

    Unicode *text = NULL;
    text = (Unicode *) grealloc(text, word->len * sizeof(Unicode));
    for (int i = 0; i < word->len; i++) {
        text[i] = ((TextChar *) word->chars->get(i))->c;
    }
    primaryLR = checkPrimaryLR(word->chars);
    dumpFragment(text, word->len, uMap, stringTemp);
    if (verbose) {
        printf("token : %s\n", stringTemp->getCString());
    }

    gfree(text);

    xmlNewProp(node, (const xmlChar *) ATTR_TOKEN_CONTENT,
               (const xmlChar *) stringTemp->getCString());
    delete stringTemp;

    GString *gsFontName = new GString();
    if (word->getFontName()) {
        xmlChar *xcFontName;
        // If the font name normalization option is selected
        if (fullFontName) {
            xcFontName = (xmlChar *) word->getFontName();
        } else {
            xcFontName = (xmlChar *) word->normalizeFontName(word->getFontName());
        }
        //ugly code because I don't know how all these types...
        //convert to a Unicode*
        int size = xmlStrlen(xcFontName);
        Unicode *uncdFontName = (Unicode *) malloc((size + 1)
                                                   * sizeof(Unicode));
        for (int i = 0; i < size; i++) {
            uncdFontName[i] = (Unicode) xcFontName[i];
        }
        uncdFontName[size] = (Unicode) 0;
        dumpFragment(uncdFontName, size, uMap, gsFontName);

    }

    fontStyleInfo->setFontName(gsFontName->lowerCase());

    if (word->font != NULL) {
        fontStyleInfo->setFontType(word->font->isSerif());
        fontStyleInfo->setFontWidth(word->font->isFixedWidth());
    }

    fontStyleInfo->setIsBold(word->isBold());
    fontStyleInfo->setIsItalic(word->isItalic());
    fontStyleInfo->setFontSize(word->fontSize);
    fontStyleInfo->setFontColor(word->colortoString());

    snprintf(tmp, sizeof(tmp), ATTR_NUMFORMAT, word->xMin);
    xmlNewProp(node, (const xmlChar *) ATTR_X, (const xmlChar *) tmp);

    snprintf(tmp, sizeof(tmp), ATTR_NUMFORMAT, word->yMin);
    xmlNewProp(node, (const xmlChar *) ATTR_Y, (const xmlChar *) tmp);

    snprintf(tmp, sizeof(tmp), ATTR_NUMFORMAT, word->xMax - word->xMin);
    xmlNewProp(node, (const xmlChar *) ATTR_WIDTH, (const xmlChar *) tmp);

    snprintf(tmp, sizeof(tmp), ATTR_NUMFORMAT, word->yMax - word->yMin);
    xmlNewProp(node, (const xmlChar *) ATTR_HEIGHT, (const xmlChar *) tmp);

    GBool contains = gFalse;

    for (int x = 0; x < fontStyles.size(); x++) {
        if (fontStyleInfo->cmp(fontStyles[x])) {
            contains = gTrue;
            fontStyleInfo->setId(x);
            break;
        }
    }

    if (!contains) {
        fontStyleInfo->setId(fontStyles.size());
        fontStyles.push_back(fontStyleInfo);
    }

    // PL: this should be present only of !contain ???
    // otherwise duplicated fonts
    sprintf(tmp, "font%d", fontStyleInfo->getId());
    xmlNewProp(node, (const xmlChar *) ATTR_STYLEREFS, (const xmlChar *) tmp);

    free(tmp);
}

/*
 * Deprecated all annotation are under separated file
 */
void TextPage::testLinkedText(xmlNodePtr node, double xMin, double yMin, double xMax, double yMax) {
    /*
	 * first test if overlap
	 * then create stuff for ml node:
	 * if uri:  ad @href= value
	 * if goto:  add @hlink = ...??what !! = page and position values
	 */
    GString idvalue = new GString("X");
    Link *link;
    LinkAction *action;

    char *tmp;
    tmp = (char *) malloc(50 * sizeof(char));

    for (int j = 0; j < pageLinks->getNumLinks(); ++j) {
//		printf("link num:%d\n",j);
        //get rectangle
        link = pageLinks->getLink(j);
        if (link->isOk()) {
            GBool isOverlap;
            double x1, y1, x2, y2;
            double xa1, xa2, ya1, ya2;
            double tmpx;
            action = link->getAction();
            if (action->isOk()) {
                link->getRect(&x1, &y1, &x2, &y2);
                curstate->transform(x1, y1, &xa1, &ya1);
                curstate->transform(x2, y2, &xa2, &ya2);
                tmpx = ya1;
                ya1 = min(ya1, ya2);
                ya2 = max(tmpx, ya2);
                //		printf("link %d %g %g %g %g\n",action->getKind(),xa1,ya1,xa2,ya2);
                isOverlap = testOverlap(xa1, ya1, xa2, ya2, xMin, yMin, xMax, yMax);
                // test overlap
                if (isOverlap) {
                    switch (action->getKind()) {
                        case actionURI: {
                            LinkURI *uri = (LinkURI *) action;
                            if (uri->isOk()) {
                                GString *dest = uri->getURI();
                                if (dest != NULL) {
                                    // PL: the uri here must be well formed to avoid not wellformed XML!
                                    xmlChar *name_uri = xmlURIEscape((const xmlChar *) dest->getCString());
                                    //xmlNewProp(node, (const xmlChar*)ATTR_URILINK,(const xmlChar*)dest->getCString());
                                    xmlNewProp(node, (const xmlChar *) ATTR_URILINK, name_uri);
                                    free(name_uri); // PL
                                    return;
                                }
                            }
                            break;
                        }
                        case actionGoTo: {
                            LinkGoTo *goto_link = (LinkGoTo *) action;
                            if (goto_link->isOk()) {
                                bool newlink = false;
                                LinkDest *link_dest = goto_link->getDest();
                                GString *name_dest = goto_link->getNamedDest();
                                if (name_dest != NULL && myCat != NULL) {
                                    link_dest = myCat->findDest(name_dest);
                                    newlink = true;
                                }
                                if (link_dest != NULL && link_dest->isOk()) {
                                    // find the destination page number (counted from 1)
                                    int page;
                                    if (link_dest->isPageRef()) {
                                        Ref pref = link_dest->getPageRef();
                                        page = myCat->findPage(pref.num, pref.gen);
                                    } else
                                        page = link_dest->getPageNum();

                                    // other data depend in the link type
                                    switch (link_dest->getKind()) {
                                        case destXYZ: {
                                            // find the location on the destination page
                                            //if (link_dest->getChangeLeft() && link_dest->getChangeTop()){
                                            // TODO FH 25/01/2006 apply transform matrix of destination page, not current page
                                            double x, y;
                                            curstate->transform(link_dest->getLeft(), link_dest->getTop(), &x, &y);
                                            snprintf(tmp, sizeof(tmp), "p-%d %g %g", page, x, y);
                                            xmlNewProp(node, (const xmlChar *) ATTR_GOTOLINK, (const xmlChar *) tmp);
//												printf("link %d %g %g\n",page,x,y);
                                            free(tmp); // PL
                                            return;
                                            //}
                                        }
                                            break;

                                            // link to the page, without a specific location. PDF Data Destruction has hit again!
                                        case destFit:
                                        case destFitH:
                                        case destFitV:
                                        case destFitR:
                                        case destFitB:
                                        case destFitBH:
                                        case destFitBV:
                                            sprintf(tmp, "p-%d", page);
                                            xmlNewProp(node, (const xmlChar *) ATTR_GOTOLINK, (const xmlChar *) tmp);
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
                        case actionGoToR: {
                            break;
                        }
                        case actionLaunch: {
                            break;
                        }
                        case actionNamed: {
                            break;
                        }
                        case actionMovie: {
                            break;
                        }
                        case actionJavaScript: {
                            break;
                        }
                        case actionSubmitForm: {
                            break;
                        }
                        case actionHide: {
                            break;
                        }
                        case actionUnknown: {
                            break;
                        }
                    }//switch
                }
            } //action isOk
        } // link isOk
    }

}

GBool TextPage::testOverlap(double x11, double y11, double x12, double y12, double x21, double y21, double x22, double y22) {
    return ((min(x12, x22) >= max(x11, x21)) &&
            (min(y12, y22) >= max(y11, y21)));
}

/**
 * Checks whether there are highlighted text.
 */
GBool TextPage::testAnnotatedText(double xMin, double yMin, double xMax, double yMax) {
    Object objQuadPoints;
    Dict *dict;

    int nb = highlightedObject.size();
    for (int i = 0; i < nb; i++) {
        dict = highlightedObject[i];
        // Get the localization (points series) of the annotation into the page
        if ((*dict).lookup("QuadPoints", &objQuadPoints)->isArray()) {
            Object point;
            double xmax2 = 0;
            double ymax2 = 0;
            double xmin2 = 0;
            double ymin2 = 0;
            double x, y, x1, y1;
            for (int i = 0; i < objQuadPoints.arrayGetLength(); ++i) {
                objQuadPoints.arrayGet(i, &point);
                if (i % 2 == 0) {
                    //even = x
                    if (point.isReal()) {
                        x = point.getReal();
                    }
                } else {
                    //odd= y

                    if (point.isReal()) {
                        y = point.getReal();

                        curstate->transform(x, y, &x1, &y1);
//						printf("POINT : %g %g\t %g %g\n",x,y,x1,y1);
                        if (xmin2 == 0) { xmin2 = x1; }
                        else { xmin2 = min(xmin2, x1); }
                        xmax2 = max(xmax2, x1);

                        if (ymin2 == 0) { ymin2 = y1; }
                        else { ymin2 = min(ymin2, y1); }
                        ymax2 = max(ymax2, y1);
                    }
                }
                point.free();

            }
            if ((min(xMax, xmax2) >= max(xMin, xmin2)) && (min(yMax, ymax2) >= max(yMin, ymin2))) {
                return gTrue;
            }
        }
        objQuadPoints.free();

    }
    return gFalse;
}

GBool TextFontStyleInfo::cmp(TextFontStyleInfo *tsi) {
    if (((fontName && fontName->cmp(tsi->getFontName()) == 0)
         && (fontSize == tsi->getFontSize())
         && (fontColor && fontColor->cmp(tsi->getFontColor()) == 0)
         && (fontType == tsi->getFontType())
         && (fontWidth == tsi->getFontWidth())
         && (isbold == tsi->isBold())
         && (isitalic == tsi->isItalic())
         && (issubscript == tsi->isSubscript())
         && (issuperscript == tsi->isSuperscript())
    )
            )
        return gTrue;
    else 
        return gFalse;
}

//------------------------------------------------------------------------
// TextGap
//------------------------------------------------------------------------
class TextGap {

public:
    TextGap(double aXY, double aW) : xy(aXY), w(aW) {}

    double xy;  // center of gap: x for vertical gaps,
                //   y for horizontal gaps
    double w;   // width of gap
};

//------------------------------------------------------------------------
// TextPage: layout analysis
//------------------------------------------------------------------------
// Determine primary (most common) rotation value.  Rotate all chars
// to that primary rotation.
int TextPage::rotateChars(GList *charsA) {
    TextChar *ch;
    int nChars[4];
    double xMin, yMin, xMax, yMax, t;
    int rot, i;

    // determine primary rotation
    nChars[0] = nChars[1] = nChars[2] = nChars[3] = 0;
    for (i = 0; i < charsA->getLength(); ++i) {
        ch = (TextChar *) charsA->get(i);
        ++nChars[ch->rot];
    }
    rot = 0;
    for (i = 1; i < 4; ++i) {
        if (nChars[i] > nChars[rot]) {
            rot = i;
        }
    }

    // rotate
    switch (rot) {
        case 0:
        default:
            break;
        case 1:
            for (i = 0; i < charsA->getLength(); ++i) {
                ch = (TextChar *) charsA->get(i);
                xMin = ch->yMin;
                xMax = ch->yMax;
                yMin = pageWidth - ch->xMax;
                yMax = pageWidth - ch->xMin;
                ch->xMin = xMin;
                ch->xMax = xMax;
                ch->yMin = yMin;
                ch->yMax = yMax;
                ch->rot = (ch->rot + 3) & 3;
            }
            t = pageWidth;
            pageWidth = pageHeight;
            pageHeight = t;
            break;
        case 2:
            for (i = 0; i < charsA->getLength(); ++i) {
                ch = (TextChar *) charsA->get(i);
                xMin = pageWidth - ch->xMax;
                xMax = pageWidth - ch->xMin;
                yMin = pageHeight - ch->yMax;
                yMax = pageHeight - ch->yMin;
                ch->xMin = xMin;
                ch->xMax = xMax;
                ch->yMin = yMin;
                ch->yMax = yMax;
                ch->rot = (ch->rot + 2) & 3;
            }
            break;
        case 3:
            for (i = 0; i < charsA->getLength(); ++i) {
                ch = (TextChar *) charsA->get(i);
                xMin = pageHeight - ch->yMax;
                xMax = pageHeight - ch->yMin;
                yMin = ch->xMin;
                yMax = ch->xMax;
                ch->xMin = xMin;
                ch->xMax = xMax;
                ch->yMin = yMin;
                ch->yMax = yMax;
                ch->rot = (ch->rot + 1) & 3;
            }
            t = pageWidth;
            pageWidth = pageHeight;
            pageHeight = t;
            break;
    }

    return rot;
}

// Undo the coordinate transform performed by rotateChars().
void TextPage::unrotateChars(GList *wordsA, int rot) {
    //TextChar *ch;
    TextWord *word;
    double xMin, yMin, xMax, yMax, t;
    int i;

    switch (rot) {
        case 0:
        default:
            // no transform
            break;
        case 1:
            t = pageWidth;
            pageWidth = pageHeight;
            pageHeight = t;
            for (i = 0; i < wordsA->getLength(); ++i) {
                word = (TextWord *) wordsA->get(i);
                xMin = pageWidth - word->yMax;
                xMax = pageWidth - word->yMin;
                yMin = word->xMin;
                yMax = word->xMax;
                word->xMin = xMin;
                word->xMax = xMax;
                word->yMin = yMin;
                word->yMax = yMax;
                word->rot = (word->rot + 1) & 3;
            }
            break;
        case 2:
            for (i = 0; i < wordsA->getLength(); ++i) {
                word = (TextWord *) wordsA->get(i);
                xMin = pageWidth - word->xMax;
                xMax = pageWidth - word->xMin;
                yMin = pageHeight - word->yMax;
                yMax = pageHeight - word->yMin;
                word->xMin = xMin;
                word->xMax = xMax;
                word->yMin = yMin;
                word->yMax = yMax;
                word->rot = (word->rot + 2) & 3;
            }
            break;
        case 3:
            t = pageWidth;
            pageWidth = pageHeight;
            pageHeight = t;
            for (i = 0; i < wordsA->getLength(); ++i) {
                word = (TextWord *) wordsA->get(i);
                xMin = word->yMin;
                xMax = word->yMax;
                yMin = pageHeight - word->xMax;
                yMax = pageHeight - word->xMin;
                word->xMin = xMin;
                word->xMax = xMax;
                word->yMin = yMin;
                word->yMax = yMax;
                word->rot = (word->rot + 3) & 3;
            }
            break;
    }
}

// Determine the primary text direction (LR vs RL).  Returns true for
// LR, false for RL.
GBool TextPage::checkPrimaryLR(GList *charsA) {
    TextChar *ch;
    int i, lrCount;

    lrCount = 0;
    for (i = 0; i < charsA->getLength(); ++i) {
        ch = (TextChar *) charsA->get(i);
        if (unicodeTypeL(ch->c)) {
            ++lrCount;
        } else if (unicodeTypeR(ch->c)) {
            --lrCount;
        }
    }
    return lrCount >= 0;
}

// Split the characters into trees of TextBlocks, one tree for each
// rotation.  Merge into a single tree (with the primary rotation).
TextBlock *TextPage::splitChars(GList *charsA) {
    TextBlock *tree[4];
    TextBlock *blk;
    GList *chars2, *clippedChars;
    TextChar *ch;
    int rot, i;

    // split: build a tree of TextBlocks for each rotation
    clippedChars = new GList();
    for (rot = 0; rot < 4; ++rot) {
        chars2 = new GList();
        for (i = 0; i < charsA->getLength(); ++i) {
            ch = (TextChar *) charsA->get(i);
            if (ch->rot == rot) {
                chars2->append(ch);
            }
        }
        tree[rot] = NULL;
        if (chars2->getLength() > 0) {
            chars2->sort((rot & 1) ? &TextChar::cmpY : &TextChar::cmpX);
//            removeDuplicates(chars2, rot);
//            if (control.clipText) {
//                i = 0;
//                while (i < chars2->getLength()) {
//                    ch = (TextChar *)chars2->get(i);
//                    if (ch->clipped) {
//                        ch = (TextChar *)chars2->del(i);
//                        clippedChars->append(ch);
//                    } else {
//                        ++i;
//                    }
//                }
//            }
            if (chars2->getLength() > 0) {
                tree[rot] = split(chars2, rot);
            }
        }
        delete chars2;
    }

    // if the page contains no (unclipped) text, just leave an empty
    // column list
    if (!tree[0]) {
        // normally tree[0] is empty only if there is no text at all, but
        // if the caller didn't do rotation, the rotated trees may be
        // non-empty, so we need to free them
        for (rot = 1; rot < 4; ++rot) {
            if (tree[rot]) {
                delete tree[rot];
            }
        }
        delete clippedChars;
        return NULL;
    }

    // if the main tree is not a multicolumn node, insert one so that
    // rotated text has somewhere to go
    if (tree[0]->tag != blkTagMulticolumn) {
        blk = new TextBlock(blkHorizSplit, 0);
        blk->addChild(tree[0]);
        blk->tag = blkTagMulticolumn;
        tree[0] = blk;
    }

    // merge non-primary-rotation text into the primary-rotation tree
    for (rot = 1; rot < 4; ++rot) {
        if (tree[rot]) {
            insertIntoTree(tree[rot], tree[0]);
            tree[rot] = NULL;
        }
    }

    if (clippedChars->getLength()) {
        insertClippedChars(clippedChars, tree[0]);
    }
    delete clippedChars;

#if 0 //~debug
    dumpTree(tree[0]);
#endif

    return tree[0];
}

// Insert clipped characters back into the TextBlock tree.
void TextPage::insertClippedChars(GList *clippedChars, TextBlock *tree) {
    TextChar *ch, *ch2;
    TextBlock *leaf;
    double y;
    int i;

    //~ this currently works only for characters in the primary rotation

    clippedChars->sort(TextChar::cmpX);
    while (clippedChars->getLength()) {
        ch = (TextChar *) clippedChars->del(0);
        if (ch->rot != 0) {
            continue;
        }
        if (!(leaf = findClippedCharLeaf(ch, tree))) {
            continue;
        }
        leaf->addChild(ch, gFalse);
        i = 0;
        while (i < clippedChars->getLength()) {
            ch2 = (TextChar *) clippedChars->get(i);
            if (ch2->xMin > ch->xMax + clippedTextMaxWordSpace * ch->fontSize) {
                break;
            }
            y = 0.5 * (ch2->yMin + ch2->yMax);
            if (y > leaf->yMin && y < leaf->yMax) {
                ch2 = (TextChar *) clippedChars->del(i);
                leaf->addChild(ch2, gFalse);
                ch = ch2;
            } else {
                ++i;
            }
        }
    }
}

// Find the leaf in <tree> to which clipped char <ch> can be appended.
// Returns NULL if there is no appropriate append point.
TextBlock *TextPage::findClippedCharLeaf(TextChar *ch, TextBlock *tree) {
    TextBlock *ret, *child;
    double y;
    int i;

    //~ this currently works only for characters in the primary rotation

    y = 0.5 * (ch->yMin + ch->yMax);
    if (tree->type == blkLeaf) {
        if (tree->rot == 0) {
            if (y > tree->yMin && y < tree->yMax &&
                ch->xMin <= tree->xMax + clippedTextMaxWordSpace * ch->fontSize) {
                return tree;
            }
        }
    } else {
        for (i = 0; i < tree->children->getLength(); ++i) {
            child = (TextBlock *) tree->children->get(i);
            if ((ret = findClippedCharLeaf(ch, child))) {
                return ret;
            }
        }
    }
    return NULL;
}

// Generate a tree of TextBlocks, marked as columns, lines, and words.
TextBlock *TextPage::split(GList *charsA, int rot) {
    TextBlock *blk;
    GList *chars2, *chars3;
    GList *horizGaps, *vertGaps;
    TextGap *gap;
    TextChar *ch;
    double xMin, yMin, xMax, yMax, avgFontSize;
    double horizGapSize, vertGapSize, minHorizChunkWidth, minVertChunkWidth;
    double nLines, vertGapThreshold, minChunk;
    double largeCharSize;
    double x0, x1, y0, y1;
    int nHorizGaps, nVertGaps, nLargeChars;
    int i;
    GBool doHorizSplit, doVertSplit, smallSplit;

    //----- find all horizontal and vertical gaps

    horizGaps = new GList();
    vertGaps = new GList();
    findGaps(charsA, rot, &xMin, &yMin, &xMax, &yMax, &avgFontSize,
             horizGaps, vertGaps);

    //----- find the largest horizontal and vertical gaps

    horizGapSize = 0;
    for (i = 0; i < horizGaps->getLength(); ++i) {
        gap = (TextGap *) horizGaps->get(i);
        if (gap->w > horizGapSize) {
            horizGapSize = gap->w;
        }
    }
    vertGapSize = 0;
    for (i = 0; i < vertGaps->getLength(); ++i) {
        gap = (TextGap *) vertGaps->get(i);
        if (gap->w > vertGapSize) {
            vertGapSize = gap->w;
        }
    }

    //----- count horiz/vert gaps equivalent to largest gaps

    minHorizChunkWidth = yMax - yMin;
    nHorizGaps = 0;
    if (horizGaps->getLength() > 0) {
        y0 = yMin;
        for (i = 0; i < horizGaps->getLength(); ++i) {
            gap = (TextGap *) horizGaps->get(i);
            if (gap->w > horizGapSize - splitGapSlack * avgFontSize) {
                ++nHorizGaps;
                y1 = gap->xy - 0.5 * gap->w;
                if (y1 - y0 < minHorizChunkWidth) {
                    minHorizChunkWidth = y1 - y0;
                }
                y0 = y1 + gap->w;
            }
        }
        y1 = yMax;
        if (y1 - y0 < minHorizChunkWidth) {
            minHorizChunkWidth = y1 - y0;
        }
    }
    minVertChunkWidth = xMax - xMin;
    nVertGaps = 0;
    if (vertGaps->getLength() > 0) {
        x0 = xMin;
        for (i = 0; i < vertGaps->getLength(); ++i) {
            gap = (TextGap *) vertGaps->get(i);
            if (gap->w > vertGapSize - splitGapSlack * avgFontSize) {
                ++nVertGaps;
                x1 = gap->xy - 0.5 * gap->w;
                if (x1 - x0 < minVertChunkWidth) {
                    minVertChunkWidth = x1 - x0;
                }
                x0 = x1 + gap->w;
            }
        }
        x1 = xMax;
        if (x1 - x0 < minVertChunkWidth) {
            minVertChunkWidth = x1 - x0;
        }
    }

    //----- compute splitting parameters

    // approximation of number of lines in block
    if (fabs(avgFontSize) < 0.001) {
        nLines = 1;
    } else if (rot & 1) {
        nLines = (xMax - xMin) / avgFontSize;
    } else {
        nLines = (yMax - yMin) / avgFontSize;
    }

    // compute the minimum allowed vertical gap size
    // (this is a horizontal gap threshold for rot=1,3
    vertGapThreshold = vertGapThresholdMax + vertGapThresholdSlope * nLines;
    if (vertGapThreshold < vertGapThresholdMin) {
        vertGapThreshold = vertGapThresholdMin;
    }
    //}
    vertGapThreshold = vertGapThreshold * avgFontSize;

    // compute the minimum allowed chunk width
    minChunk = vertSplitChunkThreshold * avgFontSize;
    //}

    // look for large chars
    // -- this kludge (multiply by 256, convert to int, divide by 256.0)
    //    prevents floating point stability issues on x86 with gcc, where
    //    largeCharSize could otherwise have slightly different values
    //    here and where it's used below to do the large char partition
    //    (because it gets truncated from 80 to 64 bits when spilled)
    largeCharSize = (int) (largeCharThreshold * avgFontSize * 256) / 256.0;
    nLargeChars = 0;
    for (i = 0; i < charsA->getLength(); ++i) {
        ch = (TextChar *) charsA->get(i);
        if (ch->fontSize > largeCharSize) {
            ++nLargeChars;
        }
    }

    // figure out which type of split to do
    doHorizSplit = doVertSplit = gFalse;
    smallSplit = gFalse;
    if (rot & 1) {
        if (nHorizGaps > 0 &&
            horizGapSize > vertGapSize &&
            horizGapSize > vertGapThreshold &&
            (minHorizChunkWidth > minChunk ||
             nVertGaps == 0)) {
            doHorizSplit = gTrue;
        } else if (nVertGaps > 0) {
            doVertSplit = gTrue;
        } else if (nLargeChars == 0 && nHorizGaps > 0) {
            doHorizSplit = gTrue;
            smallSplit = gTrue;
        }
    } else {
        if (nVertGaps > 0 &&
            vertGapSize > horizGapSize &&
            vertGapSize > vertGapThreshold &&
            (minVertChunkWidth > minChunk ||
             nHorizGaps == 0)) {
            doVertSplit = gTrue;
        } else if (nHorizGaps > 0) {
            doHorizSplit = gTrue;
        } else if (nLargeChars == 0 && nVertGaps > 0) {
            doVertSplit = gTrue;
            smallSplit = gTrue;
        }
    }

    //----- split the block

    //~ this could use "other content" (vector graphics, rotated text) --
    //~ presence of other content in a gap means we should definitely split

    // split vertically
    if (doVertSplit) {
#if 0 //~debug
        printf("vert split xMin=%g yMin=%g xMax=%g yMax=%g small=%d\n",
       xMin, pageHeight - yMax, xMax, pageHeight - yMin, smallSplit);
    for (i = 0; i < vertGaps->getLength(); ++i) {
      gap = (TextGap *)vertGaps->get(i);
      if (gap->w > vertGapSize - splitGapSlack * avgFontSize) {
    printf("    x=%g\n", gap->xy);
      }
    }
#endif
        blk = new TextBlock(blkVertSplit, rot);
        blk->smallSplit = smallSplit;
        x0 = xMin - 1;
        for (i = 0; i < vertGaps->getLength(); ++i) {
            gap = (TextGap *) vertGaps->get(i);
            if (gap->w > vertGapSize - splitGapSlack * avgFontSize) {
                x1 = gap->xy;
                chars2 = getChars(charsA, x0, yMin - 1, x1, yMax + 1);
                blk->addChild(split(chars2, rot));
                delete chars2;
                x0 = x1;
            }
        }
        chars2 = getChars(charsA, x0, yMin - 1, xMax + 1, yMax + 1);
        blk->addChild(split(chars2, rot));
        delete chars2;

        // split horizontally
    } else if (doHorizSplit) {
#if 0 //~debug
        printf("horiz split xMin=%g yMin=%g xMax=%g yMax=%g small=%d\n",
       xMin, pageHeight - yMax, xMax, pageHeight - yMin, smallSplit);
    for (i = 0; i < horizGaps->getLength(); ++i) {
      gap = (TextGap *)horizGaps->get(i);
      if (gap->w > horizGapSize - splitGapSlack * avgFontSize) {
    printf("    y=%g\n", pageHeight - gap->xy);
      }
    }
#endif
        blk = new TextBlock(blkHorizSplit, rot);
        blk->smallSplit = smallSplit;
        y0 = yMin - 1;
        for (i = 0; i < horizGaps->getLength(); ++i) {
            gap = (TextGap *) horizGaps->get(i);
            if (gap->w > horizGapSize - splitGapSlack * avgFontSize) {
                y1 = gap->xy;
                chars2 = getChars(charsA, xMin - 1, y0, xMax + 1, y1);
                blk->addChild(split(chars2, rot));
                delete chars2;
                y0 = y1;
            }
        }
        chars2 = getChars(charsA, xMin - 1, y0, xMax + 1, yMax + 1);
        blk->addChild(split(chars2, rot));
        delete chars2;

        // split into larger and smaller chars
    } else if (nLargeChars > 0) {
#if 0 //~debug
        printf("large char split xMin=%g yMin=%g xMax=%g yMax=%g\n",
       xMin, pageHeight - yMax, xMax, pageHeight - yMin);
#endif
        chars2 = new GList();
        chars3 = new GList();
        for (i = 0; i < charsA->getLength(); ++i) {
            ch = (TextChar *) charsA->get(i);
            if (ch->fontSize > largeCharSize) {
                chars2->append(ch);
            } else {
                chars3->append(ch);
            }
        }
        blk = split(chars3, rot);
        insertLargeChars(chars2, blk);
        delete chars2;
        delete chars3;

        // create a leaf node
    } else {
#if 0 //~debug
        printf("leaf xMin=%g yMin=%g xMax=%g yMax=%g\n",
       xMin, pageHeight - yMax, xMax, pageHeight - yMin);
#endif
        blk = new TextBlock(blkLeaf, rot);
        for (i = 0; i < charsA->getLength(); ++i) {
            blk->addChild((TextChar *) charsA->get(i), gTrue);
        }
    }

    deleteGList(horizGaps, TextGap);
    deleteGList(vertGaps, TextGap);

    tagBlock(blk);

    return blk;
}

// Return the subset of chars inside a rectangle.
GList *TextPage::getChars(GList *charsA, double xMin, double yMin,
                          double xMax, double yMax) {
    GList *ret;
    TextChar *ch;
    double x, y;
    int i;

    ret = new GList();
    for (i = 0; i < charsA->getLength(); ++i) {
        ch = (TextChar *) charsA->get(i);
        // because of {ascent,descent}AdjustFactor, the y coords (or x
        // coords for rot 1,3) for the gaps will be a little bit tight --
        // so we use the center of the character here
        x = 0.5 * (ch->xMin + ch->xMax);
        y = 0.5 * (ch->yMin + ch->yMax);
        if (x > xMin && x < xMax && y > yMin && y < yMax) {
            ret->append(ch);
        }
    }
    return ret;
}

// Insert a list of large characters into a tree.
void TextPage::insertLargeChars(GList *largeChars, TextBlock *blk) {
    TextChar *ch, *ch2;
    GBool singleLine;
    double minOverlap;
    int i;

    //~ this currently works only for characters in the primary rotation

    // check to see if the large chars are a single line
    singleLine = gTrue;
    for (i = 1; i < largeChars->getLength(); ++i) {
        ch = (TextChar *) largeChars->get(i - 1);
        ch2 = (TextChar *) largeChars->get(i);
        minOverlap = 0.5 * (ch->fontSize < ch2->fontSize ? ch->fontSize
                                                         : ch2->fontSize);
        if (ch->yMax - ch2->yMin < minOverlap ||
            ch2->yMax - ch->yMin < minOverlap) {
            singleLine = gFalse;
            break;
        }
    }

    if (singleLine) {
        // if the large chars are a single line, prepend them to the first
        // leaf node in blk
        insertLargeCharsInFirstLeaf(largeChars, blk);
    } else {
        // if the large chars are not a single line, prepend each one to
        // the appropriate leaf node -- this handles cases like bullets
        // drawn in a large font, on the left edge of a column
        for (i = largeChars->getLength() - 1; i >= 0; --i) {
            ch = (TextChar *) largeChars->get(i);
            insertLargeCharInLeaf(ch, blk);
        }
    }
}

// Find the first leaf (in depth-first order) in blk, and prepend a
// list of large chars.
void TextPage::insertLargeCharsInFirstLeaf(GList *largeChars, TextBlock *blk) {
    TextChar *ch;
    int i;

    if (blk->type == blkLeaf) {
        for (i = largeChars->getLength() - 1; i >= 0; --i) {
            ch = (TextChar *) largeChars->get(i);
            blk->prependChild(ch);
        }
    } else {
        insertLargeCharsInFirstLeaf(largeChars, (TextBlock *) blk->children->get(0));
        blk->updateBounds(0);
    }
}

// Find the leaf in <blk> where large char <ch> belongs, and prepend
// it.
void TextPage::insertLargeCharInLeaf(TextChar *ch, TextBlock *blk) {
    TextBlock *child;
    double y;
    int i;

    //~ this currently works only for characters in the primary rotation

    //~ this currently just looks down the left edge of blk
    //~   -- it could be extended to do more

    // estimate the baseline of ch
    y = ch->yMin + 0.75 * (ch->yMax - ch->yMin);

    if (blk->type == blkLeaf) {
        blk->prependChild(ch);
    } else if (blk->type == blkHorizSplit) {
        for (i = 0; i < blk->children->getLength(); ++i) {
            child = (TextBlock *) blk->children->get(i);
            if (y < child->yMax || i == blk->children->getLength() - 1) {
                insertLargeCharInLeaf(ch, child);
                blk->updateBounds(i);
                break;
            }
        }
    } else {
        insertLargeCharInLeaf(ch, (TextBlock *) blk->children->get(0));
        blk->updateBounds(0);
    }
}

void TextPage::findGaps(GList *charsA, int rot,
                        double *xMinOut, double *yMinOut,
                        double *xMaxOut, double *yMaxOut,
                        double *avgFontSizeOut,
                        GList *horizGaps, GList *vertGaps) {
    TextChar *ch;
    int *horizProfile, *vertProfile;
    double xMin, yMin, xMax, yMax, w;
    double minFontSize, avgFontSize, splitPrecision, ascentAdjust, descentAdjust;
    int xMinI, yMinI, xMaxI, yMaxI, xMinI2, yMinI2, xMaxI2, yMaxI2;
    int start, x, y, i;

    //----- compute bbox, min font size, average font size, and split precision

    xMin = yMin = xMax = yMax = 0; // make gcc happy
    minFontSize = avgFontSize = 0;
    for (i = 0; i < charsA->getLength(); ++i) {
        ch = (TextChar *) charsA->get(i);
        if (i == 0 || ch->xMin < xMin) {
            xMin = ch->xMin;
        }
        if (i == 0 || ch->yMin < yMin) {
            yMin = ch->yMin;
        }
        if (i == 0 || ch->xMax > xMax) {
            xMax = ch->xMax;
        }
        if (i == 0 || ch->yMax > yMax) {
            yMax = ch->yMax;
        }
        avgFontSize += ch->fontSize;
        if (i == 0 || ch->fontSize < minFontSize) {
            minFontSize = ch->fontSize;
        }
    }
    avgFontSize /= charsA->getLength();
    splitPrecision = splitPrecisionMul * minFontSize;
    if (splitPrecision < minSplitPrecision) {
        splitPrecision = minSplitPrecision;
    }
    *xMinOut = xMin;
    *yMinOut = yMin;
    *xMaxOut = xMax;
    *yMaxOut = yMax;
    *avgFontSizeOut = avgFontSize;

    //----- compute the horizontal and vertical profiles

    // add some slack to the array bounds to avoid floating point
    // precision problems
    xMinI = (int) floor(xMin / splitPrecision) - 1;
    yMinI = (int) floor(yMin / splitPrecision) - 1;
    xMaxI = (int) floor(xMax / splitPrecision) + 1;
    yMaxI = (int) floor(yMax / splitPrecision) + 1;
    horizProfile = (int *) gmallocn(yMaxI - yMinI + 1, sizeof(int));
    vertProfile = (int *) gmallocn(xMaxI - xMinI + 1, sizeof(int));
    memset(horizProfile, 0, (yMaxI - yMinI + 1) * sizeof(int));
    memset(vertProfile, 0, (xMaxI - xMinI + 1) * sizeof(int));
    for (i = 0; i < charsA->getLength(); ++i) {
        ch = (TextChar *) charsA->get(i);
        // yMinI2 and yMaxI2 are adjusted to allow for slightly overlapping lines
        switch (rot) {
            case 0:
            default:
                xMinI2 = (int) floor(ch->xMin / splitPrecision);
                xMaxI2 = (int) floor(ch->xMax / splitPrecision);
                ascentAdjust = ascentAdjustFactor * (ch->yMax - ch->yMin);
                yMinI2 = (int) floor((ch->yMin + ascentAdjust) / splitPrecision);
                descentAdjust = descentAdjustFactor * (ch->yMax - ch->yMin);
                yMaxI2 = (int) floor((ch->yMax - descentAdjust) / splitPrecision);
                break;
            case 1:
                descentAdjust = descentAdjustFactor * (ch->xMax - ch->xMin);
                xMinI2 = (int) floor((ch->xMin + descentAdjust) / splitPrecision);
                ascentAdjust = ascentAdjustFactor * (ch->xMax - ch->xMin);
                xMaxI2 = (int) floor((ch->xMax - ascentAdjust) / splitPrecision);
                yMinI2 = (int) floor(ch->yMin / splitPrecision);
                yMaxI2 = (int) floor(ch->yMax / splitPrecision);
                break;
            case 2:
                xMinI2 = (int) floor(ch->xMin / splitPrecision);
                xMaxI2 = (int) floor(ch->xMax / splitPrecision);
                descentAdjust = descentAdjustFactor * (ch->yMax - ch->yMin);
                yMinI2 = (int) floor((ch->yMin + descentAdjust) / splitPrecision);
                ascentAdjust = ascentAdjustFactor * (ch->yMax - ch->yMin);
                yMaxI2 = (int) floor((ch->yMax - ascentAdjust) / splitPrecision);
                break;
            case 3:
                ascentAdjust = ascentAdjustFactor * (ch->xMax - ch->xMin);
                xMinI2 = (int) floor((ch->xMin + ascentAdjust) / splitPrecision);
                descentAdjust = descentAdjustFactor * (ch->xMax - ch->xMin);
                xMaxI2 = (int) floor((ch->xMax - descentAdjust) / splitPrecision);
                yMinI2 = (int) floor(ch->yMin / splitPrecision);
                yMaxI2 = (int) floor(ch->yMax / splitPrecision);
                break;
        }
        for (y = yMinI2; y <= yMaxI2; ++y) {
            ++horizProfile[y - yMinI];
        }
        for (x = xMinI2; x <= xMaxI2; ++x) {
            ++vertProfile[x - xMinI];
        }
    }

    //----- build the list of horizontal gaps

    for (start = yMinI; start < yMaxI && !horizProfile[start - yMinI]; ++start);
    for (y = start; y < yMaxI; ++y) {
        if (horizProfile[y - yMinI]) {
            if (!horizProfile[y + 1 - yMinI]) {
                start = y;
            }
        } else {
            if (horizProfile[y + 1 - yMinI]) {
                w = (y - start) * splitPrecision;
                horizGaps->append(new TextGap((start + 1) * splitPrecision + 0.5 * w,
                                              w));
            }
        }
    }

    //----- build the list of vertical gaps

    for (start = xMinI; start < xMaxI && !vertProfile[start - xMinI]; ++start);
    for (x = start; x < xMaxI; ++x) {
        if (vertProfile[x - xMinI]) {
            if (!vertProfile[x + 1 - xMinI]) {
                start = x;
            }
        } else {
            if (vertProfile[x + 1 - xMinI]) {
                w = (x - start) * splitPrecision;
                vertGaps->append(new TextGap((start + 1) * splitPrecision + 0.5 * w,
                                             w));
            }
        }
    }

    gfree(horizProfile);
    gfree(vertProfile);
}

// Decide whether this block is a line, column, or multiple columns:
// - all leaf nodes are lines
// - horiz split nodes whose children are lines or columns are columns
// - other horiz split nodes are multiple columns
// - vert split nodes, with small gaps, whose children are lines are lines
// - other vert split nodes are multiple columns
// (for rot=1,3: the horiz and vert splits are swapped)
// In table layout mode:
// - all leaf nodes are lines
// - vert split nodes, with small gaps, whose children are lines are lines
// - everything else is multiple columns
// In simple layout mode:
// - all leaf nodes are lines
// - vert split nodes with small gaps are lines
// - vert split nodes with large gaps are super-lines
// - horiz split nodes are columns
void TextPage::tagBlock(TextBlock *blk) {
    TextBlock *child;
    int i;

    if (blk->type == blkLeaf) {
        blk->tag = blkTagLine;

    } else {
        if (blk->type == ((blk->rot & 1) ? blkVertSplit : blkHorizSplit)) {
            blk->tag = blkTagColumn;
            for (i = 0; i < blk->children->getLength(); ++i) {
                child = (TextBlock *) blk->children->get(i);
                if (child->tag != blkTagColumn && child->tag != blkTagLine) {
                    blk->tag = blkTagMulticolumn;
                    break;
                }
            }
        } else {
            if (blk->smallSplit) {
                blk->tag = blkTagLine;
                for (i = 0; i < blk->children->getLength(); ++i) {
                    child = (TextBlock *) blk->children->get(i);
                    if (child->tag != blkTagLine) {
                        blk->tag = blkTagMulticolumn;
                        break;
                    }
                }
            } else {
                blk->tag = blkTagMulticolumn;
            }
        }
    }
}

// Convert the tree of TextBlocks into a list of TextColumns.
GList *TextPage::buildColumns(TextBlock *tree, GBool primaryLR) {
    GList *columns;

    columns = new GList();
    buildColumns2(tree, columns, primaryLR);
    return columns;
}

//Recursive loop through the blocks tree
void TextPage::buildColumns2(TextBlock *blk, GList *columns, GBool primaryLR) {
    TextColumn *col;
    int i;

    switch (blk->tag) {
        case blkTagSuperLine: // should never happen
        case blkTagLine:
        case blkTagColumn:
            col = buildColumn(blk);
            columns->append(col);
            break;
        case blkTagMulticolumn:
#if 0 //~tmp
            if (!primaryLR && blk->type == blkVertSplit) {
      for (i = blk->children->getLength() - 1; i >= 0; --i) {
    buildColumns2((TextBlock *)blk->children->get(i), columns, primaryLR);
      }
    } else {
#endif
            for (i = 0; i < blk->children->getLength(); ++i) {
                buildColumns2((TextBlock *) blk->children->get(i), columns, primaryLR);
            }
#if 0 //~tmp
            }
#endif
            break;
    }
}

TextColumn *TextPage::buildColumn(TextBlock *blk) {
    GList *lines, *parLines;
    GList *paragraphs;
    TextLine *line0, *line1;
    GBool dropCap;
    double spaceThresh, indent0, indent1, fontSize0, fontSize1;
    int i;

    lines = new GList();
    buildLines(blk, lines);

    spaceThresh = paragraphSpacingThreshold * getAverageLineSpacing(lines);

    //~ could look for bulleted lists here: look for the case where
    //~   all out-dented lines start with the same char

    //~ this doesn't handle right-to-left scripts (need to look for indents
    //~   on the right instead of left, etc.)

    // build the paragraphs
    paragraphs = new GList();
    i = 0;
    while (i < lines->getLength()) {

        // get the first line of the paragraph
        parLines = new GList();
        dropCap = gFalse;
        line0 = (TextLine *) lines->get(i);
        parLines->append(line0);
        ++i;

        if (i < lines->getLength()) {
            line1 = (TextLine *) lines->get(i);
            indent0 = getLineIndent(line0, blk);
            indent1 = getLineIndent(line1, blk);
            fontSize0 = line0->fontSize;
            fontSize1 = line1->fontSize;

            // inverted indent
            if (indent1 - indent0 > minParagraphIndent * fontSize0 &&
                fabs(fontSize0 - fontSize1) <= paragraphFontSizeDelta &&
                getLineSpacing(line0, line1) <= spaceThresh) {
                parLines->append(line1);
                indent0 = indent1;
                for (++i; i < lines->getLength(); ++i) {
                    line1 = (TextLine *) lines->get(i);
                    indent1 = getLineIndent(line1, blk);
                    fontSize1 = line1->fontSize;
                    if (indent0 - indent1 > minParagraphIndent * fontSize0) {
                        break;
                    }
                    if (fabs(fontSize0 - fontSize1) > paragraphFontSizeDelta) {
                        break;
                    }
                    if (getLineSpacing((TextLine *) lines->get(i - 1), line1)
                        > spaceThresh) {
                        break;
                    }
                    parLines->append(line1);
                }

                // drop cap
            } else if (fontSize0 > largeCharThreshold * fontSize1 &&
                       indent1 - indent0 > minParagraphIndent * fontSize1 &&
                       getLineSpacing(line0, line1) < 0) {
                dropCap = gTrue;
                parLines->append(line1);
                fontSize0 = fontSize1;
                for (++i; i < lines->getLength(); ++i) {
                    line1 = (TextLine *) lines->get(i);
                    indent1 = getLineIndent(line1, blk);
                    if (indent1 - indent0 <= minParagraphIndent * fontSize0) {
                        break;
                    }
                    if (getLineSpacing((TextLine *) lines->get(i - 1), line1)
                        > spaceThresh) {
                        break;
                    }
                    parLines->append(line1);
                }
                for (; i < lines->getLength(); ++i) {
                    line1 = (TextLine *) lines->get(i);
                    indent1 = getLineIndent(line1, blk);
                    fontSize1 = line1->fontSize;
                    if (indent1 - indent0 > minParagraphIndent * fontSize0) {
                        break;
                    }
                    if (fabs(fontSize0 - fontSize1) > paragraphFontSizeDelta) {
                        break;
                    }
                    if (getLineSpacing((TextLine *) lines->get(i - 1), line1)
                        > spaceThresh) {
                        break;
                    }
                    parLines->append(line1);
                }

                // regular indent or no indent
            } else if (fabs(fontSize0 - fontSize1) <= paragraphFontSizeDelta &&
                       getLineSpacing(line0, line1) <= spaceThresh) {
                parLines->append(line1);
                indent0 = indent1;
                for (++i; i < lines->getLength(); ++i) {
                    line1 = (TextLine *) lines->get(i);
                    indent1 = getLineIndent(line1, blk);
                    fontSize1 = line1->fontSize;
                    if (indent1 - indent0 > minParagraphIndent * fontSize0) {
                        break;
                    }
                    if (fabs(fontSize0 - fontSize1) > paragraphFontSizeDelta) {
                        break;
                    }
                    if (getLineSpacing((TextLine *) lines->get(i - 1), line1)
                        > spaceThresh) {
                        break;
                    }
                    parLines->append(line1);
                }
            }
        }

        paragraphs->append(new TextParagraph(parLines, dropCap));
    }

    delete lines;

    return new TextColumn(paragraphs, blk->xMin, blk->yMin,
                          blk->xMax, blk->yMax);
}

double TextPage::getLineIndent(TextLine *line, TextBlock *blk) {
    double indent;

    switch (line->rot) {
        case 0:
        default:
            indent = line->xMin - blk->xMin;
            break;
        case 1:
            indent = line->yMin - blk->yMin;
            break;
        case 2:
            indent = blk->xMax - line->xMax;
            break;
        case 3:
            indent = blk->yMax - line->yMax;
            break;
    }
    return indent;
}

// Compute average line spacing in column.
double TextPage::getAverageLineSpacing(GList *lines) {
    double avg, sp;
    int n, i;

    avg = 0;
    n = 0;
    for (i = 1; i < lines->getLength(); ++i) {
        sp = getLineSpacing((TextLine *) lines->get(i - 1),
                            (TextLine *) lines->get(i));
        if (sp > 0) {
            avg += sp;
            ++n;
        }
    }
    if (n > 0) {
        avg /= n;
    }
    return avg;
}

// Compute the space between two lines.
double TextPage::getLineSpacing(TextLine *line0, TextLine *line1) {
    double sp;

    switch (line0->rot) {
        case 0:
        default:
            sp = line1->yMin - line0->yMax;
            break;
        case 1:
            sp = line0->xMin - line1->xMax;
            break;
        case 2:
            sp = line0->yMin - line1->yMin;
            break;
        case 3:
            sp = line1->xMin - line1->xMax;
            break;
    }
    return sp;
}

// recusive call to column block until blkTagLine is matched
void TextPage::buildLines(TextBlock *blk, GList *lines) {
    TextLine *line;
    int i;

    switch (blk->tag) {
        case blkTagLine:
            line = buildLine(blk);
            if (blk->rot == 1 || blk->rot == 2) {
                lines->insert(0, line);
            } else {
                lines->append(line);
            }
            break;
        case blkTagSuperLine:
        case blkTagColumn:
        case blkTagMulticolumn: // multicolumn should never happen here
            for (i = 0; i < blk->children->getLength(); ++i) {
                buildLines((TextBlock *) blk->children->get(i), lines);
            }
            break;
    }
}

TextLine *TextPage::buildLine(TextBlock *blk) {
    GList *charsA;
    GList *words;
    TextChar *ch, *ch2;
    TextWord *word;
    double wordSp, lineFontSize, sp;
    int dir, dir2;
    GBool spaceAfter, spaceBefore;
    int i, j;

    charsA = new GList();
    getLineChars(blk, charsA);

    wordSp = computeWordSpacingThreshold(charsA, blk->rot);

    words = new GList();
    lineFontSize = 0;
    spaceBefore = gFalse;
    i = 0;
    while (i < charsA->getLength()) {
        sp = wordSp - 1;
        spaceAfter = gFalse;
        dir = getCharDirection((TextChar *) charsA->get(i));
        for (j = i + 1; j < charsA->getLength(); ++j) {
            ch = (TextChar *) charsA->get(j - 1);
            ch2 = (TextChar *) charsA->get(j);
            sp = (blk->rot & 1) ? (ch2->yMin - ch->yMax) : (ch2->xMin - ch->xMax);
            if (sp > wordSp) {
                spaceAfter = gTrue;
                break;
            }
            // look for significant overlaps, which can happen with clipped
            // characters (among other things)
            if (sp < -ch->fontSize) {
                spaceAfter = gTrue;
                break;
            }
            dir2 = getCharDirection(ch2);
            if (ch->font != ch2->font ||
                fabs(ch->fontSize - ch2->fontSize) > 0.01 ||
                (dir && dir2 && dir2 != dir)) {
                break;
            }
            if (!dir && dir2) {
                dir = dir2;
            }
            sp = wordSp - 1;
        }
        // Increment the absolute object index
        idx++;
        idWORD++;
        word = new TextWord(charsA, i, j - i, blk->rot, dir,
                            (blk->rot >= 2) ? spaceBefore : spaceAfter, ((TextChar *) charsA->get(i))->state,
                            ((TextChar *) charsA->get(i))->font, ((TextChar *) charsA->get(i))->fontSize, getIdWORD(),
                            getIdx());

        spaceBefore = spaceAfter;
        i = j;
        if (blk->rot >= 2) {
            words->insert(0, word);
        } else {
            words->append(word);
        }
        if (i == 0 || word->fontSize > lineFontSize) {
            lineFontSize = word->fontSize;
        }
    }

    delete charsA;

    return new TextLine(words, blk->xMin, blk->yMin, blk->xMax, blk->yMax,
                        lineFontSize);
}

void TextPage::getLineChars(TextBlock *blk, GList *charsA) {
    int i;

    if (blk->type == blkLeaf) {
        charsA->append(blk->children);
    } else {
        for (i = 0; i < blk->children->getLength(); ++i) {
            getLineChars((TextBlock *) blk->children->get(i), charsA);
        }
    }
}

// Compute the inter-word spacing threshold for a line of chars.
// Spaces greater than this threshold will be considered inter-word
// spaces.
double TextPage::computeWordSpacingThreshold(GList *charsA, int rot) {
    TextChar *ch, *ch2;
    double avgFontSize;
    double minAdjGap, maxAdjGap, minSpGap, maxSpGap, minGap, maxGap, gap, gap2;
    int i;

    avgFontSize = 0;
    minGap = maxGap = 0;
    minAdjGap = minSpGap = 1;
    maxAdjGap = maxSpGap = 0;
    for (i = 0; i < charsA->getLength(); ++i) {
        ch = (TextChar *) charsA->get(i);
        avgFontSize += ch->fontSize;
        if (i < charsA->getLength() - 1) {
            ch2 = (TextChar *) charsA->get(i + 1);
            gap = (rot & 1) ? (ch2->yMin - ch->yMax) : (ch2->xMin - ch->xMax);
            if (ch->spaceAfter) {
                if (minSpGap > maxSpGap) {
                    minSpGap = maxSpGap = gap;
                } else if (gap < minSpGap) {
                    minSpGap = gap;
                } else if (gap > maxSpGap) {
                    maxSpGap = gap;
                }
            } else {
                if (minAdjGap > maxAdjGap) {
                    minAdjGap = maxAdjGap = gap;
                } else if (gap < minAdjGap) {
                    minAdjGap = gap;
                } else if (gap > maxAdjGap) {
                    maxAdjGap = gap;
                }
            }
            if (i == 0 || gap < minGap) {
                minGap = gap;
            }
            if (gap > maxGap) {
                maxGap = gap;
            }
        }
    }
    avgFontSize /= charsA->getLength();
    if (minGap < 0) {
        minGap = 0;
    }

    // if spacing is nearly uniform (minGap is close to maxGap), use the
    // SpGap/AdjGap values if available, otherwise assume it's a single
    // word (technically it could be either "ABC" or "A B C", but it's
    // essentially impossible to tell)
    if (maxGap - minGap < uniformSpacing * avgFontSize) {
        if (minAdjGap <= maxAdjGap &&
            minSpGap <= maxSpGap &&
            minSpGap - maxAdjGap > 0.01) {
            return 0.5 * (maxAdjGap + minSpGap);
        } else {
            return maxGap + 1;
        }

        // if there is some variation in spacing, but it's small, assume
        // there are some inter-word spaces
    } else if (maxGap - minGap < wordSpacing * avgFontSize) {
        return 0.5 * (minGap + maxGap);

        // if there is a large variation in spacing, use the SpGap/AdjGap
        // values if they look reasonable, otherwise, assume a reasonable
        // threshold for inter-word spacing (we can't use something like
        // 0.5*(minGap+maxGap) here because there can be outliers at the
        // high end)
    } else {
        if (minAdjGap <= maxAdjGap &&
            minSpGap <= maxSpGap &&
            minSpGap - maxAdjGap > uniformSpacing * avgFontSize) {
            gap = wordSpacing * avgFontSize;
            gap2 = 0.5 * (minSpGap - minGap);
            return minGap + (gap < gap2 ? gap : gap2);
        } else {
            return minGap + wordSpacing * avgFontSize;
        }
    }
}

// Check the characters direction: returns 1 for L or Num; -1 for R; 0
// for others.
int TextPage::getCharDirection(TextChar *ch) {
    if (unicodeTypeL(ch->c) || unicodeTypeNum(ch->c)) {
        return 1;
    }
    if (unicodeTypeR(ch->c)) {
        return -1;
    }
    return 0;
}

//int TextPage::assignPhysLayoutPositions(GList *columns) {
//    assignLinePhysPositions(columns);
//    return assignColumnPhysPositions(columns);
//}

void TextPage::computeLinePhysWidth(TextLine *line, UnicodeMap *uMap) {
    char buf[8];
    int n, i;

    if (uMap->isUnicode()) {
        line->pw = line->len;
    } else {
        line->pw = 0;
        for (i = 0; i < line->len; ++i) {
            n = uMap->mapUnicode(line->text[i], buf, sizeof(buf));
            line->pw += n;
        }
    }
}

// Merge blk (rot != 0) into primaryTree (rot == 0).
void TextPage::insertIntoTree(TextBlock *blk, TextBlock *primaryTree) {
    TextBlock *child;

    // we insert a whole column at a time - so call insertIntoTree
    // recursively until we get to a column (or line)

    if (blk->tag == blkTagMulticolumn) {
        while (blk->children->getLength()) {
            child = (TextBlock *) blk->children->del(0);
            insertIntoTree(child, primaryTree);
        }
        delete blk;
    } else {
        insertColumnIntoTree(blk, primaryTree);
    }
}

// Insert a column (as an atomic subtree) into tree.
// Requirement: tree is not a leaf node.
void TextPage::insertColumnIntoTree(TextBlock *column, TextBlock *tree) {
    TextBlock *child;
    int i;

    for (i = 0; i < tree->children->getLength(); ++i) {
        child = (TextBlock *) tree->children->get(i);
        if (child->tag == blkTagMulticolumn &&
            column->xMin >= child->xMin &&
            column->yMin >= child->yMin &&
            column->xMax <= child->xMax &&
            column->yMax <= child->yMax) {
            insertColumnIntoTree(column, child);
            tree->tag = blkTagMulticolumn;
            return;
        }
    }

    if (tree->type == blkVertSplit) {
        if (tree->rot == 1 || tree->rot == 2) {
            for (i = 0; i < tree->children->getLength(); ++i) {
                child = (TextBlock *) tree->children->get(i);
                if (column->xMax > 0.5 * (child->xMin + child->xMax)) {
                    break;
                }
            }
        } else {
            for (i = 0; i < tree->children->getLength(); ++i) {
                child = (TextBlock *) tree->children->get(i);
                if (column->xMin < 0.5 * (child->xMin + child->xMax)) {
                    break;
                }
            }
        }
    } else if (tree->type == blkHorizSplit) {
        if (tree->rot >= 2) {
            for (i = 0; i < tree->children->getLength(); ++i) {
                child = (TextBlock *) tree->children->get(i);
                if (column->yMax > 0.5 * (child->yMin + child->yMax)) {
                    break;
                }
            }
        } else {
            for (i = 0; i < tree->children->getLength(); ++i) {
                child = (TextBlock *) tree->children->get(i);
                if (column->yMin < 0.5 * (child->yMin + child->yMax)) {
                    break;
                }
            }
        }
    } else {
        // this should never happen
        return;
    }
    tree->children->insert(i, column);
    tree->tag = blkTagMulticolumn;
}

// PL: this is not used
void TextPage::dumpInReadingOrder(GBool noLineNumbers, GBool fullFontName) {
    TextBlock *tree;
    TextColumn *col;
    TextParagraph *par;
    TextLine *line;
    TextWord *word;
    TextWord *nextWord;
    GList *columns;
    //GBool primaryLR;

    int colIdx, parIdx, lineIdx, wordI, rot, n;

    UnicodeMap *uMap;
    TextFontStyleInfo *fontStyleInfo;

    // get the output encoding
    if (!(uMap = globalParams->getTextEncoding())) {
        return;
    }

    rot = rotateChars(chars);
    primaryLR = checkPrimaryLR(chars);
    tree = splitChars(chars);
#if 0 //~debug
    dumpTree(tree);
#endif
    if (!tree) {
        // no text
        unrotateChars(chars, rot);
        return;
    }
    columns = buildColumns(tree, primaryLR);
    delete tree;
    unrotateChars(chars, rot);
#if 0 //~debug
    dumpColumns(columns);
#endif

    xmlNodePtr node = NULL;
    xmlNodePtr nodeline = NULL;
    xmlNodePtr nodeblocks = NULL;
    xmlNodePtr nodeImageInline = NULL;

    GString *id;

    printSpace = xmlNewNode(NULL, (const xmlChar *) TAG_PRINTSPACE);
    printSpace->type = XML_ELEMENT_NODE;
    xmlAddChild(page, printSpace);

    for (colIdx = 0; colIdx < columns->getLength(); ++colIdx) {
        col = (TextColumn *) columns->get(colIdx);
        for (parIdx = 0; parIdx < col->paragraphs->getLength(); ++parIdx) {

            //if (useBlocks) 
            {
                nodeblocks = xmlNewNode(NULL, (const xmlChar *) TAG_BLOCK);
                nodeblocks->type = XML_ELEMENT_NODE;

                id = new GString("p");
                xmlNewProp(nodeblocks, (const xmlChar *) ATTR_ID,
                           (const xmlChar *) buildIdBlock(num, numBlock, id)->getCString());
                delete id;
                numBlock = numBlock + 1;
            }

            // for superscript/subscript determination
            double previousWordBaseLine = 0;
            double previousWordYmin = 0;
            double previousWordYmax = 0;
            double currentLineBaseLine = 0;
            double currentLineYmin = 0;
            double currentLineYmax = 0;

            par = (TextParagraph *) col->paragraphs->get(parIdx);
            for (lineIdx = 0; lineIdx < par->lines->getLength(); ++lineIdx) {
                line = (TextLine *) par->lines->get(lineIdx);

                // this is the max font size for the line
                double lineFontSize = line->fontSize;

                nodeline = xmlNewNode(NULL, (const xmlChar *) TAG_TEXT);
                nodeline->type = XML_ELEMENT_NODE;

                n = line->len;
                if (line->hyphenated && lineIdx + 1 < par->lines->getLength()) {
                    --n;
                }

                for (wordI = 0; wordI < line->words->getLength(); ++wordI) {

                    word = (TextWord *) line->words->get(wordI);
                    if (wordI < line->words->getLength() - 1)
                        nextWord = (TextWord *) line->words->get(wordI + 1);

                    char *tmp;

                    tmp = (char *) malloc(10 * sizeof(char));

                    fontStyleInfo = new TextFontStyleInfo;

                    node = xmlNewNode(NULL, (const xmlChar *) TAG_TOKEN);

                    node->type = XML_ELEMENT_NODE;

                    // determine if the current token is superscript of subscript: a general constraint for superscript and subscript
                    // is that the scripted tokens have a font size smaller than the tokens on the line baseline.

                    // superscript
                    if (currentLineBaseLine != 0 &&
                        wordI > 0 &&
                        word->base < currentLineBaseLine &&
                        word->yMax > currentLineYmin &&
                        word->fontSize < lineFontSize) {
                        // superscript: general case, not at the beginning of a line
                        fontStyleInfo->setIsSuperscript(gTrue);
                    }
                    else if (wordI == 0 &&
                        wordI < line->words->getLength() - 1 &&
                        nextWord != NULL &&
                        word->base < nextWord->base &&
                        word->yMax > nextWord->yMin &&
                        word->fontSize < lineFontSize) {
                        // superscript: case first token of the line, check if the current token is the first token of the line 
                        // and use next tokens to see if we have a vertical shift
                        // note: it won't work when we have several tokens in superscript at the beginning of the line...
                        // actually it might screw all the rest :/
                        // superscript as first token of a line is common for declaring affiliations (and sometime references)
                        fontStyleInfo->setIsSuperscript(gTrue);
                        currentLineBaseLine = nextWord->base;
                        currentLineYmin = nextWord->yMin;
                        currentLineYmax = nextWord->yMax;
                    }
                    else if (wordI > 0 &&
                        word->base > currentLineBaseLine &&
                        word->yMin < currentLineYmax &&
                        word->fontSize < lineFontSize) {
                        // common subscript, not at the beginning of a line
                        fontStyleInfo->setIsSubscript(gTrue);
                    }
                    else if (wordI == 0 &&
                        wordI < line->words->getLength() - 1 &&
                        nextWord != NULL &&
                        word->base > nextWord->base &&
                        word->yMin < nextWord->yMax &&
                        word->fontSize < lineFontSize) {
                        // subscript: case first token of the line, check if the current token is the first token of the line 
                        // and use next tokens to see if we have a vertical shift
                        // note: it won't work when we have several tokens in subscripts at the beginning of the line...
                        // actually it might screw all the rest :/
                        // subscript as first token of a line should never appear, but it's better to cover this case to 
                        // avoid having the rest of the line detected as superscript... 
                        fontStyleInfo->setIsSubscript(gTrue);
                        currentLineBaseLine = nextWord->base;
                        currentLineYmin = nextWord->yMin;
                        currentLineYmax = nextWord->yMax;
                    }
                    // PL: above, we need to pay attention to the font style of the previous token and consider the whole line, 
                    // because otherwise the token next to a subscript is always superscript even when normal, in addition for 
                    // several tokens as superscript or subscript, only the first one will be set as superscript or subscript

                    // If option verbose is selected
                    if (verbose) {
                        addAttributsNodeVerbose(node, tmp, word);
                    }

                    addAttributsNode(node, word, fontStyleInfo, uMap, fullFontName);
                    addAttributTypeReadingOrder(node, tmp, word);

//                    encodeFragment(line->text, n, uMap, primaryLR, s);
//                    if (lineIdx + 1 < par->lines->getLength() && !line->hyphenated) {
//                        s->append(space, spaceLen);
//                    }

                    // Add next images inline whithin the current line if the noImageInline option is not selected
                    if (parameters->getImageInline()) {
                        if (indiceImage != -1) {
                            int nb = listeImageInline.size();
                            for (; indiceImage < nb; indiceImage++) {
                                if (idWORDBefore
                                    == listeImageInline[indiceImage]->idWordBefore) {
                                    nodeImageInline = xmlNewNode(NULL,
                                                                 (const xmlChar *) TAG_TOKEN);
                                    nodeImageInline->type = XML_ELEMENT_NODE;
                                    id = new GString("p");
                                    xmlNewProp(nodeImageInline, (const xmlChar *) ATTR_ID,
                                               (const xmlChar *) buildIdToken(num, numToken, id)->getCString());
                                    delete id;
                                    numToken = numToken + 1;
                                    id = new GString("p");
                                    xmlNewProp(
                                            nodeImageInline,
                                            (const xmlChar *) ATTR_SID,
                                            (const xmlChar *) buildSID(num, listeImageInline[indiceImage]->getIdx(),
                                                                       id)->getCString());
                                    delete id;
                                    snprintf(tmp, sizeof(tmp), ATTR_NUMFORMAT,
                                            listeImageInline[indiceImage]->getXPositionImage());
                                    xmlNewProp(nodeImageInline, (const xmlChar *) ATTR_X,
                                               (const xmlChar *) tmp);
                                    snprintf(tmp, sizeof(tmp), ATTR_NUMFORMAT,
                                            listeImageInline[indiceImage]->getYPositionImage());
                                    xmlNewProp(nodeImageInline, (const xmlChar *) ATTR_Y,
                                               (const xmlChar *) tmp);
                                    snprintf(tmp, sizeof(tmp), ATTR_NUMFORMAT,
                                             listeImageInline[indiceImage]->getWidthImage());
                                    xmlNewProp(nodeImageInline, (const xmlChar *) ATTR_WIDTH,
                                               (const xmlChar *) tmp);
                                    snprintf(tmp, sizeof(tmp), ATTR_NUMFORMAT,
                                             listeImageInline[indiceImage]->getHeightImage());
                                    xmlNewProp(nodeImageInline,
                                               (const xmlChar *) ATTR_HEIGHT,
                                               (const xmlChar *) tmp);
                                    xmlNewProp(
                                            nodeImageInline,
                                            (const xmlChar *) ATTR_HREF,
                                            (const xmlChar *) listeImageInline[indiceImage]->getHrefImage()->getCString());
                                    xmlAddChild(nodeline, nodeImageInline);
                                }
                            }
                        }
                    }

                    xmlAddChild(nodeline, node);

                    if (wordI < line->words->getLength() - 1 and word->spaceAfter) {
                        xmlNodePtr spacingNode = xmlNewNode(NULL, (const xmlChar *) TAG_SPACING);
                        spacingNode->type = XML_ELEMENT_NODE;
                        snprintf(tmp, sizeof(tmp), ATTR_NUMFORMAT, (nextWord->xMin - word->xMax));
                        xmlNewProp(spacingNode, (const xmlChar *) ATTR_WIDTH,
                                   (const xmlChar *) tmp);
                        snprintf(tmp, sizeof(tmp), ATTR_NUMFORMAT, (word->yMin));
                        xmlNewProp(spacingNode, (const xmlChar *) ATTR_Y,
                                   (const xmlChar *) tmp);
                        snprintf(tmp, sizeof(tmp), ATTR_NUMFORMAT, (word->xMax));
                        xmlNewProp(spacingNode, (const xmlChar *) ATTR_X,
                                   (const xmlChar *) tmp);

                        xmlAddChild(nodeline, spacingNode);
                    }

                    if (!fontStyleInfo->isSuperscript() && !fontStyleInfo->isSubscript()) {
                        currentLineBaseLine = word->base;
                        currentLineYmin = word->yMin;
                        currentLineYmax = word->yMax;
                    }
                    previousWordBaseLine = word->base;
                    previousWordYmin = word->yMin;
                    previousWordYmax = word->yMax;

                    free(tmp);
                }

                //if (useBlocks)
                xmlAddChild(nodeblocks, nodeline);
                //else
                //    xmlAddChild(printSpace, nodeline);
            }

            //if (useBlocks)
            xmlAddChild(printSpace, nodeblocks);
        }
        //(*outputFunc)(outputStream, eol, eolLen);
    }

    deleteGList(columns, TextColumn);

    int imageCount = listeImages.size();
    for (int i = 0; i < imageCount; ++i) {

        char *tmp;

        tmp = (char *) malloc(10 * sizeof(char));
        node = xmlNewNode(NULL, (const xmlChar *) TAG_IMAGE);
        xmlNewProp(node, (const xmlChar *) ATTR_ID, (const xmlChar *) listeImages[i]->getImageId()->getCString());


        //xmlNewProp(node, (const xmlChar *) ATTR_SID,(const xmlChar*)listeImages[i]->getImageSid()->getCString());


        snprintf(tmp, sizeof(tmp), ATTR_NUMFORMAT, listeImages[i]->getXPositionImage());
        xmlNewProp(node, (const xmlChar *) ATTR_X, (const xmlChar *) tmp);
        snprintf(tmp, sizeof(tmp), ATTR_NUMFORMAT, listeImages[i]->getYPositionImage());
        xmlNewProp(node, (const xmlChar *) ATTR_Y, (const xmlChar *) tmp);
        snprintf(tmp, sizeof(tmp), ATTR_NUMFORMAT, listeImages[i]->getWidthImage());
        xmlNewProp(node, (const xmlChar *) ATTR_WIDTH, (const xmlChar *) tmp);
        snprintf(tmp, sizeof(tmp), ATTR_NUMFORMAT, listeImages[i]->getHeightImage());
        xmlNewProp(node, (const xmlChar *) ATTR_HEIGHT, (const xmlChar *) tmp);

        std::string rotation = std::to_string(listeImages[i]->getRotation());
        xmlNewProp(node,(const xmlChar*)ATTR_ROTATION,(const xmlChar*)rotation.c_str());
        //if (listeImages[i]->getRotation() > 0){
        //    xmlNewProp(node,(const xmlChar*)ATTR_ROTATION,(const xmlChar*)sTRUE);
        //}
        //else{
        //    xmlNewProp(node,(const xmlChar*)ATTR_ROTATION,(const xmlChar*)sFALSE);
        //}

//        if (listeImages[i]->isImageInline()) {
//            xmlNewProp(node, (const xmlChar *) ATTR_INLINE, (const xmlChar *) sTRUE);
//        }
        xmlNewProp(node, (const xmlChar *) ATTR_HREF,
                   (const xmlChar *) listeImages[i]->getHrefImage()->getCString());

        xmlNewProp(node, (const xmlChar *) ATTR_TYPE,
                   (const xmlChar *) listeImages[i]->getType()->getCString());

        xmlNewProp(node, (const xmlChar *) ATTR_CLIPZONE,
                   (const xmlChar *) listeImages[i]->getClipZone()->getCString());
        xmlAddChild(printSpace, node);
        free(tmp);
    }
}

bool is_digit(Unicode u) {
    // simply match Unicode for ASCII digit... should we add more unicode variants of numbers?
    if (u == (Unicode) 48 ||
        u == (Unicode) 49 ||
        u == (Unicode) 50 ||
        u == (Unicode) 51 ||
        u == (Unicode) 52 ||
        u == (Unicode) 53 ||
        u == (Unicode) 54 ||
        u == (Unicode) 55 ||
        u == (Unicode) 56 ||
        u == (Unicode) 57)
        return true;
    else 
        return false;
}

bool is_number(TextWord *word) {
    Unicode *text = NULL;
    text = (Unicode *) grealloc(text, word->len * sizeof(Unicode));
    for (int i = 0; i < word->len; i++) {
        Unicode theU = ((TextChar *) word->chars->get(i))->c;
        if (!is_digit(theU)) {
            return false;
        }
    }
    return true;
}

int find_index(vector<double> positions, double val) {
    // simple double look-up, return index of val or -1
    int index = -1;
    for (int i = 0; i < positions.size(); i++) {
        if (positions[i] == val) {
            index = i;
            break;
        }
    }
    return index;
}

int find_index(vector<int> positions, int val) {
    // simple int look-up, return index of val or -1
    int index = -1;
    for (int i = 0; i < positions.size(); i++) {
        if (positions[i] == val) {
            index = i;
            break;
        }
    }
    return index;
}

bool TextPage::markLineNumber() {
    // Detect the presence of line number column in the page and mark the corresponding TextWord objects for further appropriate handling

    // Line number conditions:
    // - left-most or right-most alignment
    // - only made of digits
    // - vertical alignment along the page content
    // - number increment (same increment, but not necessarly by +1)
    // - same font (same font name and same font size)
    // - immediate vertical gap with next token vertically aligned (note that this is actually complicated with the PDF stream order) 

    int wordId = 0;
    //int nextVpos = 0;
    //int increment = 0;
    
    bool hasLineNumber = false;

    // at this stage we already have a block and line segmentation
    int parIdx, lineIdx, wordI, n;
    TextParagraph *par;
    TextLine *line1;
    TextWord *word;
    TextWord *nextWord;
    TextWord *previousWord;

    vector<TextWord*> lineNumberWords;
    vector<TextWord*> textWords;

    int rightMostBoundary = 0;
    int leftMostBoundary = 999990;

    // first pass is for number detection, it should stop very early in case of no line number
    // we also keep track of the left and right text boundaries for the page
    for (parIdx = 0; parIdx < blocks->getLength(); parIdx++) {
        par = (TextParagraph *) blocks->get(parIdx);

        for (lineIdx = 0; lineIdx < par->lines->getLength(); lineIdx++) {
            line1 = (TextLine *) par->lines->get(lineIdx);

            for (wordI = 0; wordI < line1->words->getLength(); wordI++) {
                word = (TextWord *) line1->words->get(wordI);
                if (wordI < line1->words->getLength() - 1)
                    nextWord = (TextWord *) line1->words->get(wordI + 1);
                else 
                    nextWord = NULL;
                if (wordId != 0)
                    previousWord = (TextWord *) words->get(wordId - 1);
                else 
                    previousWord = NULL;

                // first or last token in the line?
                if (previousWord != NULL && nextWord != NULL)
                    continue;

                // numerical token?
                if (is_number(word)) {
                    //lineNumberWords->append(word);
                    lineNumberWords.push_back(word);
                    hasLineNumber = true;
                } else {
                    //textWords->append(word);
                    textWords.push_back(word);
                }
            }
            if (line1->xMin < leftMostBoundary)
                leftMostBoundary = line1->xMin;
            if (line1->xMax > rightMostBoundary)
                rightMostBoundary = line1->xMax;
        }

        if (par->xMin < leftMostBoundary)
            leftMostBoundary = par->xMin;
        if (par->xMax > rightMostBoundary)
            rightMostBoundary = par->xMax;
    }

    if (!hasLineNumber) {
        return false;
    }

    // define the x alignment by clustering identified number tokens by x position
    vector<vector<int>> clusters;
    vector<int> positions;
    for (wordI = 0; wordI < lineNumberWords.size(); wordI++) {
        word = lineNumberWords[wordI];
        int vpos1 = word->xMin;
        int vpos2 = word->xMax;

        // we cluster by xMin and xMax positions
        int index1 = find_index(positions, vpos1);;
        int index2 = find_index(positions, vpos2);

        if (index1 != -1) {
            vector<int> the_cluster = clusters[index1];
            the_cluster.push_back(wordI);
            clusters[index1] = the_cluster;
        } else {
            // new cluster init
            vector<int> the_cluster;
            the_cluster.push_back(wordI);
            clusters.push_back(the_cluster);
            positions.push_back(vpos1);
        }

        if (index2 != -1) {
            vector<int> the_cluster = clusters[index2];
            the_cluster.push_back(wordI);
            clusters[index2] = the_cluster;
        } else {
            // new cluster init
            vector<int> the_cluster;
            the_cluster.push_back(wordI);
            clusters.push_back(the_cluster);
            positions.push_back(vpos2);
        }
    }

    // apply a similar clustering for selected non-numerical tokens 
    vector<vector<int>> textClusters;
    vector<int> textPositions;
    for (wordI = 0; wordI < textWords.size(); wordI++) {
        word = textWords[wordI];
        int vpos1 = word->xMin;
        int vpos2 = word->xMax;

        // we cluster by xMin and xMax positions
        int index1 = find_index(textPositions, vpos1);;
        int index2 = find_index(textPositions, vpos2);

        if (index1 != -1) {
            vector<int> the_cluster = textClusters[index1];
            the_cluster.push_back(wordI);
            textClusters[index1] = the_cluster;
        } else {
            // new cluster init
            vector<int> the_cluster;
            the_cluster.push_back(wordI);
            textClusters.push_back(the_cluster);
            textPositions.push_back(vpos1);
        }

        if (index2 != -1) {
            vector<int> the_cluster = textClusters[index2];
            the_cluster.push_back(wordI);
            textClusters[index2] = the_cluster;
        } else {
            // new cluster init
            vector<int> the_cluster;
            the_cluster.push_back(wordI);
            textClusters.push_back(the_cluster);
            textPositions.push_back(vpos2);
        }
    }

    vector<int> bestClusterIndex;
    vector<int> largestClusterSize;
    int nonTrivialClusterSize = 3;
    // select largest cluster of numbers, which will give the best x alignments and the corresponding lists of number tokens
    for (int i = 0; i < clusters.size(); i++) {
        vector<int> theCluster = clusters[i];

        if (theCluster.size() < nonTrivialClusterSize)
            continue;

        if (bestClusterIndex.size() == 0) {
            bestClusterIndex.push_back(i);
            largestClusterSize.push_back(theCluster.size());
        } else {
            vector<int>::iterator it1 = bestClusterIndex.begin();
            vector<int>::iterator it2 = largestClusterSize.begin();
            vector<int>::iterator last = bestClusterIndex.end();
            int j = 0;
            while(it1 != last) {
                if (theCluster.size() < largestClusterSize[j]) {
                    if (j == bestClusterIndex.size()-1) {
                        bestClusterIndex.push_back(i);
                        largestClusterSize.push_back(theCluster.size());
                        break;
                    }            
                } else {
                    // this is sorted based on the cluster siez, so we insert just before j
                    bestClusterIndex.insert(it1, i);
                    largestClusterSize.insert(it2, theCluster.size());
                    break;
                } 
                j++;
                ++it1;
                ++it2;
            }
        }
    }

    if (bestClusterIndex.size() == 0) {
        return false;
    }

    vector<int> bestCluster = clusters[bestClusterIndex[0]];
    double final_vpos = positions[bestClusterIndex[0]];

    //cout << "best alignment vpos: " << final_vpos << endl;
    //cout << "nb numbers best cluster: " << bestCluster.size() << endl;

    // check the remaining constraints: 

    // same font? we normally never have line number using different font
    /*
    for (int i = 0; i < bestCluster.size(); i++) {
        int index = bestCluster[i];
        word = (TextWord *)lineNumberWords->get(index);
        int font_size = 0;
        xmlChar *xcFontName;
        font_size = word->fontSize;    
        if (word->getFontName())
            xcFontName = (xmlChar *) word->getFontName();
        // to be finished if needed...
    }
    if (!hasLineNumber)
        return false;
    */

    // Do we have text areas at same alignment or positioned more on the side than the number cluster?
    // -> see the left-most and right-most non trivial text block with the text token clusters
    nonTrivialClusterSize = largestClusterSize[0] / 4;
    if (nonTrivialClusterSize == 0)
        nonTrivialClusterSize = 1;

    for(int j=0; j<bestClusterIndex.size(); j++) {
        bestCluster = clusters[bestClusterIndex[j]];
        final_vpos = positions[bestClusterIndex[j]];
        hasLineNumber = true;

        //cout << "move to next best" << endl;
        //cout << "     final alignment vpos: " << final_vpos << endl;
        //cout << "     nb numbers best cluster: " << bestCluster.size() << endl;

        for (int i = 0; i < textClusters.size(); i++) {
            vector<int> theCluster = textClusters[i];
            if (theCluster.size() >= nonTrivialClusterSize) {
                //word = (TextWord *)textWords->get(theCluster[0]);
                word = textWords[theCluster[0]];
                int vpos1 = word->xMin;
                int vpos2 = word->xMax;
                //int vpos1 = word.xMin;
                //int vpos2 = word.xMax;
                if (vpos1 <= final_vpos && vpos2 >= final_vpos) {
                    hasLineNumber = false;
                    break;
                }
            }
        }
        if (hasLineNumber)
            break;
    }

    if (!hasLineNumber) {
        return false;
    }

    // neutralize candidate line numbers in the middle of a page with 2 columns 
    // (these are ref numbers in the biblio or something else, but can't be line numbers)
    int quarterWidth = (rightMostBoundary - leftMostBoundary) / 4;
    if (quarterWidth+leftMostBoundary < final_vpos && final_vpos < leftMostBoundary+(quarterWidth*3))
        hasLineNumber = false;
    
    if (!hasLineNumber) {
        return false;
    }

    // increment? it's not possible to suppose any particular increments, it could be 1 by 1 or 
    // 5 by 5 for instance, however number should be growing!
    // to be done if needed...

    // immediate vertigal gap? 

    // final marking of TextWord corresponding to line numbers
    for (int i = 0; i < bestCluster.size(); i++) {
        int index = bestCluster[i];
        word = lineNumberWords[index];
        // consider only aligned tokens
        word->setLineNumber(true);
    }

    return hasLineNumber;
}

void TextPage::dump(GBool noLineNumbers, GBool fullFontName, vector<bool> lineNumberStatus) {
    // Output the page in raw (content stream) order
    blocks = new GList(); //these are blocks in alto schema

    UnicodeMap *uMap;

    TextFontStyleInfo *fontStyleInfo;

    GString *id;

    // For TEXT tag attributes
    double xMin = 0;
    double yMin = 0;
    double xMax = 0;
    double yMax = 0;
    double yMaxRot = 0;
    double yMinRot = 0;
    double xMaxRot = 0;
    double xMinRot = 0;
    int firstword = 1; // firstword of a TEXT tag

    // x and y for nodeline: min (xi) min(yi) whatever the text rotation is
    double minLineX = 0;
    double minLineY = 0;

    GBool lineFinish = gFalse;
    GBool newBlock = gFalse;
    GBool endPage = gFalse;
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

    double minBlockX = 0;
    double maxBlockLineWidth = 0;

    // Get the output encoding
    if (!(uMap = globalParams->getTextEncoding())) {
        return;
    }

    numText = 1;
    numBlock = 1;

    lineFontSize = 0;

    TextLine *line = new TextLine;

    GList *lineWords = new GList();
    line->setWords(lineWords);

    GList *parLines = new GList();

    TextParagraph *paragraph = new TextParagraph;
    paragraph->setLines(parLines);

    minBlockX = 999999;
    maxBlockLineWidth = 0;

    xMin = yMin = xMax = yMax = 0;
    minLineX = 999999;
    minLineY = 999999;

    int wordId = 0;
    for (wordId = 0; wordId < words->getLength(); wordId++) {

        TextRawWord *word, *nextWord, *prvWord;
        word = (TextRawWord *) words->get(wordId);
        if (wordId + 1 < words->getLength())
            nextWord = (TextRawWord *) words->get(wordId + 1);
        else nextWord = NULL;
        if (wordId != 0)
            prvWord = (TextRawWord *) words->get(wordId - 1);

        char *tmp;

        tmp = (char *) malloc(10 * sizeof(char));
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

        if (word->fontSize > lineFontSize) {
            lineFontSize = word->fontSize;
        }

        if (word->xMax > xMax) {
            xMax = word->xMax;
        }
        if (word->xMin < xMinRot) {
            xMinRot = word->xMin;
        }
        if (word->xMax > xMaxRot) {
            xMaxRot = word->xMax;
        }
        if (word->yMax > yMax) {
            yMax = word->yMax;
        }
        if (word->yMin < yMinRot) {
            yMinRot = word->yMin;
        }
        if (word->yMax > yMaxRot) {
            yMaxRot = word->yMax;
        }

        // If option verbose is selected
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
                if (nextWord) {
                    xxMinNext = nextWord->xMin;
                    yyMinNext = nextWord->yMin;
                }
                break;

            case 3:
                xxMin = word->yMin;
                xxMax = word->yMax;
                yyMin = word->xMax;
                yyMax = word->xMin;
                if (nextWord) {
                    xxMinNext = nextWord->yMin;
                    yyMinNext = nextWord->xMax;
                }
                break;

            case 2:
                xxMin = word->xMax;
                xxMax = word->xMin;
                yyMin = word->yMax;
                yyMax = word->yMin;
                if (nextWord) {
                    xxMinNext = nextWord->xMax;
                    yyMinNext = nextWord->yMax;
                }
                break;

            case 1:
                xxMin = word->yMax;
                xxMax = word->yMin;
                yyMin = word->xMax;
                yyMax = word->xMin;
                if (nextWord) {
                    xxMinNext = nextWord->yMax;
                    yyMinNext = nextWord->xMax;
                }
                break;
        }

        // Get the rotation into four axis
        int rotation = -1;
        if (word->rot == 0 && word->angle == 0) {
            rotation = 0;
        }
        if (word->rot == 1 && word->angle == 90) {
            rotation = 1;
        }
        if (word->rot == 2 && word->angle == 180) {
            rotation = 2;
        }
        if (word->rot == 3 && word->angle == 270) {
            rotation = 3;
        }

        // The line is finished IF :
        // 		- there is no next word
        //		- and IF the rotation of current word is different of the rotation next word
        //		- and IF the difference between the base of current word and the yMin next word is superior to the maxSpacingWordsBetweenTwoLines
        //		-      or IF the difference between the base of current word and the base next word is superior to maxIntraLineDelta * lineFontSize
        //		- and IF the xMax current word ++ maxWordSpacing * lineFontSize is superior to the xMin next word.
        //		HD 24/07/09 ?? - or if the font size of the current word is far different from the next word
        // PL: note that the second criteria create an artificial new line for every superscript sequences whithin a line (but subscript are okay), we
        // revised it by taking the max of lineFontSize and nextWord fontSize, instead of the min (it means we have a new line when the shift if greater
        // than the main font size, a smaller shift does not introduce a new line)
        if (nextWord != NULL && (word->rot == nextWord->rot) && (
                (
                    (fabs(word->base - nextWord->baseYmin) < maxSpacingWordsBetweenTwoLines) ||
                    //(fabs(nextWord->base - word->base) < maxIntraLineDelta * min(lineFontSize, nextWord->fontSize))
                    //(fabs(nextWord->base - word->base) < maxIntraLineDelta * lineFontSize)
                    (fabs(nextWord->base - word->base) < maxIntraLineDelta * max(lineFontSize, nextWord->fontSize))
                )
                && (nextWord->xMin <= word->xMax + maxWordSpacing * lineFontSize)
            )
        ) {

            // IF - switch the rotation :
            //			base word and yMin word are inferior to yMin next word
            //			xMin word is superior to xMin next word
            //			xMax word is superior to xMin next word and the difference between the base of current word and the next word is superior to maxIntraLineDelta*lineFontSize
            //			xMin next word is superior to xMax word + maxWordSpacing * lineFontSize
            //THEN if one of these tests is true, the line is finish
            if (
                ((rotation == -1) ? ((word->base < nextWord->yMin)
                                     && (word->yMin < nextWord->yMin)) : (word->rot == 0
                                                                          || word->rot == 1) ? (
                                                                                 (word->base < yyMinNext) && (yyMin
                                                                                                              <
                                                                                                              yyMinNext))
                                                                                             : (
                                                                                 (word->base > yyMinNext) && (yyMin
                                                                                                              >
                                                                                                              yyMinNext)))
                || ((rotation == -1) ? (nextWord->xMin
                                     < word->xMin) : (word->rot == 0) ? (xxMinNext < xxMin)
                                                                      : (word->rot == 1 ? xxMinNext > xxMin
                                                                                        : (word->rot == 2 ? xxMinNext >
                                                                                                            xxMin :
                                                                                           xxMinNext
                                                                                           < xxMin)))
                || ((rotation == -1) ? (nextWord->xMin < word->xMax)
                                       && (fabs(nextWord->base - word->base)
                                           > maxIntraLineDelta * lineFontSize)
                                     : (word->rot == 0 || word->rot == 3) ? ((xxMinNext < xxMax)
                                                                             && (fabs(nextWord->base - word->base)
                                                                                 > maxIntraLineDelta * lineFontSize))
                                                                          : ((xxMinNext > xxMax)
                                                                             && (fabs(nextWord->base
                                                                                      - word->base)
                                                                                 > maxIntraLineDelta * lineFontSize)))
                || ((rotation == -1) ? (nextWord->xMin > word->xMax
                                                         + maxWordSpacing * lineFontSize) : (word->rot == 0
                                                                                             || word->rot == 3) ? (
                                                                                                    xxMinNext > xxMax
                                                                                                                +
                                                                                                                maxWordSpacing *
                                                                                                                lineFontSize)
                                                                                                                : (
                                                                                                    xxMinNext
                                                                                                    < xxMax -
                                                                                                      maxWordSpacing *
                                                                                                      lineFontSize))
            ) {
                lineWords->append(word); // here we first add this last word to line then create new line
                if (word->rot == 2) {
                    lineWidth = fabs(xMaxRot - xMinRot);
                } else {
                    lineWidth = xMax - xMin;
                }

                if (word->rot == 0 || word->rot == 2) {
                    lineHeight = yMax - yMin;
                }

                if (word->rot == 1 || word->rot == 3) {
                    lineHeight = yMaxRot - yMinRot;
                }
                if (word->fontSize > lineFontSize) {
                    lineFontSize = word->fontSize;
                }
                line->setXMin(minLineX);
                line->setXMax(minLineX + lineWidth);
                line->setYMax(minLineY + lineHeight);
                line->setYMin(minLineY);
                line->setFontSize(lineFontSize);

                if (nextWord != NULL) {
                    firstword = 1;
                    // PL: blocks are now always considered at this stage
                    lineFinish = gTrue;
                } else {
                    endPage = gTrue;
                }
                xMin = yMin = xMax = yMax = yMinRot = yMaxRot = xMaxRot
                        = xMinRot = 0;
            } else {
                lineWords->append(word);
            }
        } else {
            // node to be added to nodeline
            if (word->rot == 2) {
                lineWidth = fabs(xMaxRot - xMinRot);
            } else {
                lineWidth = xMax - xMin;
            }

            if (word->rot == 0 || word->rot == 2) {
                lineHeight = yMax - yMin;
            }

            if (word->rot == 1 || word->rot == 3) {
                lineHeight = yMaxRot - yMinRot;
            }

            lineWords->append(word);

            if (word->fontSize > lineFontSize) {
                lineFontSize = word->fontSize;
            }

            line->setXMin(minLineX);
            line->setXMax(minLineX + lineWidth);
            line->setYMax(minLineY + lineHeight);
            line->setYMin(minLineY);
            line->setFontSize(lineFontSize);

            if (nextWord != NULL) {
                // PL: blocks are now always considered at this stage
                lineFinish = gTrue;
            } else {
                endPage = gTrue;
            }

            firstword = 1;
            xMin = yMin = xMax = yMax = yMinRot = yMaxRot = xMaxRot = xMinRot
                    = 0;
            minLineY = 999999;
            minLineX = 999999;
        }

        // IF it's the end of line or the end of page
        if ((lineFinish) || (endPage)) {
            // IF it's the first line
            if (linePreviousX == 0) {
                if (nextWord != NULL) {
                    if (nextWord->xMin > (lineX + lineWidth)
                                         + (maxColSpacing * lineFontSize)) {
                        newBlock = gTrue;
                    }
                }

                // PL: check if X and Y coordinates of the current block are present
                // and set them if it's not the case
                if (paragraph != NULL) {
                    // get block X and Y
                    if (paragraph->getXMin() == 0) {
                        if (lineX != 0  && minBlockX > lineX) {
                            minBlockX = lineX;
                            paragraph->setXMin(lineX);
                        }
                    }
                    if (paragraph->getYMin() == 0) {
                        if (lineYmin != 0)
                            paragraph->setYMin(lineYmin);
                    }
                }
                if(maxBlockLineWidth < lineWidth)
                    maxBlockLineWidth = lineWidth;
                parLines->append(line);
            } else {
                if (newBlock) {
                    // PL: previous block height and width
                    if (paragraph != NULL) {
                        // get block X and Y
                        double blockX = 0;
                        double blockY = 0;
                        if (paragraph->getXMin() != 0) {
                            blockX = paragraph->getXMin();
                        }
                        if (paragraph->getYMin() != 0) {
                            blockY = paragraph->getYMin();
                        }

                        //double blockWidth = std::abs((linePreviousX + linePreviousWidth) - blockX);
                        double blockHeight = std::abs((linePreviousYmin + linePreviousHeight) - blockY);

                        paragraph->setYMax(paragraph->getYMin() + blockHeight);
                        paragraph->setXMax(paragraph->getXMin() + maxBlockLineWidth);

                        // adding previous block to the page element
                        if(readingOrder)
                            lastBlockInserted = addBlockInReadingOrder(paragraph, lineFontSize, lastBlockInserted);
                        else
                            blocks->append(paragraph);
                    }

                    paragraph = new TextParagraph;
                    parLines = new GList();
                    paragraph->setLines(parLines);

                    minBlockX = 999999;
                    maxBlockLineWidth = 0;

                    if (lineX != 0 && minBlockX > lineX) {
                        minBlockX = lineX;
                        paragraph->setXMin(lineX);
                    }
                    if (lineYmin != 0)
                        paragraph->setYMin(lineYmin);

                    if(maxBlockLineWidth < lineWidth)
                        maxBlockLineWidth = lineWidth;

                    parLines->append(line);
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
                            if (paragraph != NULL) {
                                if (paragraph->getYMax() == 0) {
                                    double blockY = paragraph->getYMin();
                                    double blockHeight = std::abs((linePreviousYmin + linePreviousHeight) - blockY);
                                    paragraph->setYMax(blockY + blockHeight);
                                }
                                if (paragraph->getXMax() == 0) {
                                    double blockX = paragraph->getXMin();
                                    //double blockWidth = std::abs((linePreviousX + linePreviousWidth) - blockX);
                                    paragraph->setXMax(blockX + maxBlockLineWidth);
                                }
                            }
                        }

                        if (lineX != 0 && minBlockX > lineX) {
                            minBlockX = lineX;
                            paragraph->setXMin(lineX);
                        }

                        if(maxBlockLineWidth < lineWidth)
                            maxBlockLineWidth = lineWidth;

                        parLines->append(line);
                    } else {
                        // PL: previous block height and width
                        if (paragraph != NULL) {
                            // get block X and Y

                            //double blockWidth = std::abs((linePreviousX + linePreviousWidth) - paragraph->getXMin());
                            double blockHeight = std::abs((linePreviousYmin + linePreviousHeight) - paragraph->getYMin());

                            paragraph->setXMax(paragraph->getXMin() + maxBlockLineWidth);
                            paragraph->setYMax(paragraph->getYMin() + blockHeight);

                            // adding previous block to the page element
                            if(readingOrder)
                                lastBlockInserted = addBlockInReadingOrder(paragraph, lineFontSize, lastBlockInserted);
                            else
                                blocks->append(paragraph);
                        }

                        paragraph = new TextParagraph;
                        parLines = new GList();
                        paragraph->setLines(parLines);

                        minBlockX = 999999;
                        maxBlockLineWidth = 0;

                        // PL: new block X and Y
                        if (lineX != 0 && minBlockX > lineX) {
                            minBlockX = lineX;
                            paragraph->setXMin(lineX);
                        }
                        if (lineYmin != 0) {
                            paragraph->setYMin(lineYmin);
                        }

                        if(maxBlockLineWidth < lineWidth)
                            maxBlockLineWidth = lineWidth;

                        parLines->append(line);
                    }
                }
            }
            if (endPage) {
                endPage = gFalse;

                if (paragraph != NULL) {
                    if(readingOrder)
                        lastBlockInserted = addBlockInReadingOrder(paragraph, lineFontSize, lastBlockInserted);
                    else
                        blocks->append(paragraph);
                    lastBlockInserted = gFalse;
                }
            }

            if(minLineX < minBlockX)
                minBlockX = minLineX;
            if(lineWidth > maxBlockLineWidth)
                maxBlockLineWidth = lineWidth;

            // We save informations about the future previous line
            linePreviousX = lineX;
            linePreviousYmin = lineYmin;
            linePreviousWidth = lineWidth;
            linePreviousHeight = lineHeight;
            linePreviousFontSize = lineFontSize;
            minLineY = 999999;
            minLineX = 999999;
            line = new TextLine;
            lineWords = new GList();
            line->setWords(lineWords);
        }

        free(tmp);
    } // end FOR

    // if no line number was found in the first half of the document and the number of pages of the document is large enough,
    // we do don't need to look for line numbers any more
    int nbTotalPage = myCat->getNumPages();
    int currentPageNumber = getPageNumber();

    //cout << "nbTotalPage: " << nbTotalPage << endl;
    //cout << "currentPageNumber: " << currentPageNumber << endl;

    // check if the previous pages have line numbers
    bool previousLineNumber = true;
    for(int i=0; i<lineNumberStatus.size(); i++) {
        previousLineNumber = lineNumberStatus[i];
        if (previousLineNumber)
            break;
    }
    
    bool hasLineNumber = false;
    if ( (currentPageNumber < nbTotalPage/2) || (previousLineNumber && nbTotalPage>4)) {
        hasLineNumber = markLineNumber();
        //cout << "result markLineNumber: " << hasLineNumber << endl;
    }
    setLineNumber(hasLineNumber);

    xmlNodePtr node = NULL;
    xmlNodePtr nodeline = NULL;
    xmlNodePtr nodeblocks = NULL;
    xmlNodePtr nodeImageInline = NULL;

    TextParagraph *par;
    TextLine *line1;
    TextWord *word;
    TextWord *nextWord;

    int parIdx, lineIdx, wordI, n;

    printSpace = xmlNewNode(NULL, (const xmlChar*)TAG_PRINTSPACE);
    printSpace->type = XML_ELEMENT_NODE;
    xmlAddChild(page, printSpace);

    // for superscript/subscript determination
    double previousWordBaseLine = 0;
    double previousWordYmin = 0;
    double previousWordYmax = 0;
    double currentLineBaseLine = 0;
    double currentLineYmin = 0;
    double currentLineYmax = 0;

    if (hasLineNumber && !noLineNumbers) {
        // we preliminary introduce a block for line numbers, if any and if we have to display them

        // first grab the line numbers
        GList *lineNumberWords = new GList();
        for(parIdx = 0; parIdx < blocks->getLength(); parIdx++) {
            par = (TextParagraph *) blocks->get(parIdx);

            for(lineIdx = 0; lineIdx < par->lines->getLength(); lineIdx++) {
                line1 = (TextLine *) par->lines->get(lineIdx);

                for(wordI = 0; wordI < line1->words->getLength(); wordI++) {
                    word = (TextWord *) line1->words->get(wordI);

                    if (word->lineNumber && (is_number(word))) {
                        lineNumberWords->append(word);
                    }
                }
            }
        }

        if (lineNumberWords->getLength() > 0) {

            // compute the dim of the block
            double blockXMin = 999999;
            double blockXMax = 0;
            double blockYMin = 999999;
            double blockYMax = 0;

            for(wordI = 0; wordI < lineNumberWords->getLength(); wordI++) {
                word = (TextWord *) lineNumberWords->get(wordI);

                if (word->xMin < blockXMin)
                    blockXMin = word->xMin;
                if (word->yMin < blockYMin)
                    blockYMin = word->yMin;

                if (word->xMax > blockXMax)
                    blockXMax = word->xMax;
                if (word->yMax > blockYMax)
                    blockYMax = word->yMax;
            }

            // create custom block for line numbers
            nodeblocks = xmlNewNode(NULL, (const xmlChar *) TAG_BLOCK);
            nodeblocks->type = XML_ELEMENT_NODE;

            id = new GString("p");
            xmlNewProp(nodeblocks, (const xmlChar *) ATTR_ID,
                       (const xmlChar *) buildIdBlock(num, numBlock, id)->getCString());
            delete id;
            numBlock = numBlock + 1;

            char *tmp;
            tmp = (char *) malloc(10 * sizeof(char));
            snprintf(tmp, sizeof(tmp),ATTR_NUMFORMAT, blockXMin);
            xmlNewProp(nodeblocks, (const xmlChar*)ATTR_X, (const xmlChar*)tmp);
            snprintf(tmp, sizeof(tmp),ATTR_NUMFORMAT, blockYMin);
            xmlNewProp(nodeblocks, (const xmlChar*)ATTR_Y, (const xmlChar*)tmp);
            snprintf(tmp, sizeof(tmp),ATTR_NUMFORMAT, blockYMax - blockYMin);
            xmlNewProp(nodeblocks, (const xmlChar*)ATTR_HEIGHT, (const xmlChar*)tmp);
            snprintf(tmp, sizeof(tmp),ATTR_NUMFORMAT, blockXMax - blockXMin);
            xmlNewProp(nodeblocks, (const xmlChar*)ATTR_WIDTH, (const xmlChar*)tmp);
            free(tmp);

            for(wordI = 0; wordI < lineNumberWords->getLength(); wordI++) {
                word = (TextWord *) lineNumberWords->get(wordI);
                
                // create lines with one number
                nodeline = xmlNewNode(NULL, (const xmlChar *) TAG_TEXT);
                nodeline->type = XML_ELEMENT_NODE;

                id = new GString("p");
                xmlNewProp(nodeline, (const xmlChar*)ATTR_ID,
                           (const xmlChar*)buildIdText(num, numText, id)->getCString());
                delete id;
                numText = numText + 1;

                char *tmp;
                tmp = (char *) malloc(10 * sizeof(char));
                snprintf(tmp, sizeof(tmp),ATTR_NUMFORMAT, word->xMax - word->xMin);
                xmlNewProp(nodeline, (const xmlChar*)ATTR_WIDTH, (const xmlChar*)tmp);
                snprintf(tmp, sizeof(tmp),ATTR_NUMFORMAT, word->yMax - word->yMin);
                xmlNewProp(nodeline, (const xmlChar*)ATTR_HEIGHT, (const xmlChar*)tmp);
                snprintf(tmp, sizeof(tmp),ATTR_NUMFORMAT, word->xMin);
                xmlNewProp(nodeline, (const xmlChar*)ATTR_X, (const xmlChar*)tmp);
                snprintf(tmp, sizeof(tmp),ATTR_NUMFORMAT, word->yMin);
                xmlNewProp(nodeline, (const xmlChar*)ATTR_Y, (const xmlChar*)tmp);
                free(tmp);

                // create the number token
                node = xmlNewNode(NULL, (const xmlChar *) TAG_TOKEN);
                node->type = XML_ELEMENT_NODE;

                fontStyleInfo = new TextFontStyleInfo;

                tmp = (char *) malloc(10 * sizeof(char));

                // if option verbose is selected
                if (verbose) {
                    addAttributsNodeVerbose(node, tmp, word);
                }
                addAttributsNode(node, word, fontStyleInfo, uMap, fullFontName);
                addAttributTypeReadingOrder(node, tmp, word);    
                free(tmp);

                xmlAddChild(nodeline, node);
                xmlAddChild(nodeblocks, nodeline);
            }

            xmlAddChild(printSpace, nodeblocks);
        }

        delete lineNumberWords;
    }

    for(parIdx = 0; parIdx < blocks->getLength(); parIdx++) {
        par = (TextParagraph *) blocks->get(parIdx);

        nodeblocks = xmlNewNode(NULL, (const xmlChar *) TAG_BLOCK);
        nodeblocks->type = XML_ELEMENT_NODE;

        id = new GString("p");
        xmlNewProp(nodeblocks, (const xmlChar *) ATTR_ID,
                   (const xmlChar *) buildIdBlock(num, numBlock, id)->getCString());
        delete id;
        numBlock = numBlock + 1;

        char *tmp;
        tmp = (char *) malloc(10 * sizeof(char));
        snprintf(tmp, sizeof(tmp),ATTR_NUMFORMAT, par->getXMin());
        xmlNewProp(nodeblocks, (const xmlChar*)ATTR_X, (const xmlChar*)tmp);
        snprintf(tmp, sizeof(tmp),ATTR_NUMFORMAT, par->getYMin());
        xmlNewProp(nodeblocks, (const xmlChar*)ATTR_Y, (const xmlChar*)tmp);
        snprintf(tmp, sizeof(tmp),ATTR_NUMFORMAT, par->getYMax() - par->getYMin());
        xmlNewProp(nodeblocks, (const xmlChar*)ATTR_HEIGHT, (const xmlChar*)tmp);
        snprintf(tmp, sizeof(tmp),ATTR_NUMFORMAT, par->getXMax() - par->getXMin());
        xmlNewProp(nodeblocks, (const xmlChar*)ATTR_WIDTH, (const xmlChar*)tmp);
        free(tmp);

        for (lineIdx = 0; lineIdx < par->lines->getLength(); lineIdx++) {
            line1 = (TextLine *) par->lines->get(lineIdx);

            // this is the max font size for the line
            double lineFontSize = line1->fontSize;

            nodeline = xmlNewNode(NULL, (const xmlChar *) TAG_TEXT);
            nodeline->type = XML_ELEMENT_NODE;

            char *tmp;

            tmp = (char *) malloc(10 * sizeof(char));

            snprintf(tmp, sizeof(tmp),ATTR_NUMFORMAT, line1->getXMax() - line1->getXMin());
            xmlNewProp(nodeline, (const xmlChar*)ATTR_WIDTH, (const xmlChar*)tmp);
            snprintf(tmp, sizeof(tmp),ATTR_NUMFORMAT, line1->getYMax() - line1->getYMin());
            xmlNewProp(nodeline, (const xmlChar*)ATTR_HEIGHT, (const xmlChar*)tmp);


            // Add the ID attribute for the TEXT tag
            id = new GString("p");
            xmlNewProp(nodeline, (const xmlChar*)ATTR_ID,
                       (const xmlChar*)buildIdText(num, numText, id)->getCString());
            delete id;
            numText = numText + 1;

            snprintf(tmp, sizeof(tmp),ATTR_NUMFORMAT, line1->getXMin());
            xmlNewProp(nodeline, (const xmlChar*)ATTR_X, (const xmlChar*)tmp);

            snprintf(tmp, sizeof(tmp),ATTR_NUMFORMAT, line1->getYMin());
            xmlNewProp(nodeline, (const xmlChar*)ATTR_Y, (const xmlChar*)tmp);

            free(tmp);

            n = line1->len;
            if (line1->hyphenated && lineIdx + 1 < par->lines->getLength()) {
                --n;
            }

            for (wordI = 0; wordI < line1->words->getLength(); wordI++) {
                word = (TextWord *) line1->words->get(wordI);
                if (wordI < line1->words->getLength() - 1)
                    nextWord = (TextWord *) line1->words->get(wordI + 1);

                char *tmp;

                tmp = (char *) malloc(10 * sizeof(char));

                fontStyleInfo = new TextFontStyleInfo;

                node = xmlNewNode(NULL, (const xmlChar *) TAG_TOKEN);

                node->type = XML_ELEMENT_NODE;

                // determine if the current token is superscript of subscript: a general constraint for superscript and subscript
                // is that the scripted tokens have a font size smaller than the tokens on the line baseline.

                // superscript
                if (currentLineBaseLine != 0 &&
                    wordI > 0 &&
                    word->base < currentLineBaseLine &&
                    word->yMax > currentLineYmin &&
                    word->fontSize < lineFontSize) {
                    // superscript: general case, not at the beginning of a line
                    fontStyleInfo->setIsSuperscript(gTrue);
                }
                else if (wordI == 0 &&
                    wordI < line1->words->getLength() - 1 &&
                    nextWord != NULL &&
                    word->base < nextWord->base &&
                    word->yMax > nextWord->yMin &&
                    word->fontSize < lineFontSize) {
                    // superscript: case first token of the line, check if the current token is the first token of the line 
                    // and use next tokens to see if we have a vertical shift
                    // note: it won't work when we have several tokens in superscript at the beginning of the line...
                    // actually it might screw all the rest :/
                    // superscript as first token of a line is common for declaring affiliations (and sometime references)
                    fontStyleInfo->setIsSuperscript(gTrue);
                    currentLineBaseLine = nextWord->base;
                    currentLineYmin = nextWord->yMin;
                    currentLineYmax = nextWord->yMax;
                }
                else if (wordI > 0 &&
                    word->base > currentLineBaseLine &&
                    word->yMin < currentLineYmax &&
                    word->fontSize < lineFontSize) {
                    // common subscript, not at the beginning of a line
                    fontStyleInfo->setIsSubscript(gTrue);
                }
                else if (wordI == 0 &&
                    wordI < line1->words->getLength() - 1 &&
                    nextWord != NULL &&
                    word->base > nextWord->base &&
                    word->yMin < nextWord->yMax &&
                    word->fontSize < lineFontSize) {
                    // subscript: case first token of the line, check if the current token is the first token of the line 
                    // and use next tokens to see if we have a vertical shift
                    // note: it won't work when we have several tokens in subscripts at the beginning of the line...
                    // actually it might screw all the rest :/
                    // subscript as first token of a line should never appear, but it's better to cover this case to 
                    // avoid having the rest of the line detected as superscript... 
                    fontStyleInfo->setIsSubscript(gTrue);
                    currentLineBaseLine = nextWord->base;
                    currentLineYmin = nextWord->yMin;
                    currentLineYmax = nextWord->yMax;
                }
                // PL: above, we need to pay attention to the font style of the previous token and consider the whole line, 
                // because otherwise the token next to a subscript is always superscript even when normal, in addition for 
                // several tokens as superscript or subscript, only the first one will be set as superscript or subscript

                // if option verbose is selected
                if (verbose) {
                    addAttributsNodeVerbose(node, tmp, word);
                }
                addAttributsNode(node, word, fontStyleInfo, uMap, fullFontName);
                addAttributTypeReadingOrder(node, tmp, word);

//                    encodeFragment(line->text, n, uMap, primaryLR, s);
//                    if (lineIdx + 1 < par->lines->getLength() && !line->hyphenated) {
//                        s->append(space, spaceLen);
//                    }

                // Add next images inline whithin the current line if the noImageInline option is not selected
                if (parameters->getImageInline()) {
                    if (indiceImage != -1) {
                        int nb = listeImageInline.size();
                        for (; indiceImage < nb; indiceImage++) {
                            if (idWORDBefore
                                == listeImageInline[indiceImage]->idWordBefore) {
                                nodeImageInline = xmlNewNode(NULL,
                                                             (const xmlChar *) TAG_TOKEN);
                                nodeImageInline->type = XML_ELEMENT_NODE;
                                id = new GString("p");
                                xmlNewProp(nodeImageInline, (const xmlChar *) ATTR_ID,
                                           (const xmlChar *) buildIdToken(num, numToken, id)->getCString());
                                delete id;
                                numToken = numToken + 1;
                                id = new GString("p");
                                xmlNewProp(
                                        nodeImageInline,
                                        (const xmlChar *) ATTR_SID,
                                        (const xmlChar *) buildSID(num, listeImageInline[indiceImage]->getIdx(),
                                                                   id)->getCString());
                                delete id;
                                snprintf(tmp, sizeof(tmp), ATTR_NUMFORMAT,
                                        listeImageInline[indiceImage]->getXPositionImage());
                                xmlNewProp(nodeImageInline, (const xmlChar *) ATTR_X,
                                           (const xmlChar *) tmp);
                                snprintf(tmp, sizeof(tmp), ATTR_NUMFORMAT,
                                        listeImageInline[indiceImage]->getYPositionImage());
                                xmlNewProp(nodeImageInline, (const xmlChar *) ATTR_Y,
                                           (const xmlChar *) tmp);
                                snprintf(tmp, sizeof(tmp), ATTR_NUMFORMAT,
                                         listeImageInline[indiceImage]->getWidthImage());
                                xmlNewProp(nodeImageInline, (const xmlChar *) ATTR_WIDTH,
                                           (const xmlChar *) tmp);
                                snprintf(tmp, sizeof(tmp), ATTR_NUMFORMAT,
                                         listeImageInline[indiceImage]->getHeightImage());
                                xmlNewProp(nodeImageInline,
                                           (const xmlChar *) ATTR_HEIGHT,
                                           (const xmlChar *) tmp);
                                xmlNewProp(
                                        nodeImageInline,
                                        (const xmlChar *) ATTR_HREF,
                                        (const xmlChar *) listeImageInline[indiceImage]->getHrefImage()->getCString());
                                xmlAddChild(nodeline, nodeImageInline);
                            }
                        }
                    }
                }

                // Include a TOKEN tag for the image inline if it exists
                if (!parameters->getImageInline()) {
                    addImageInlineNode(nodeline, nodeImageInline, tmp, word);
                }

                //testLinkedText(nodeline,minLineX,minLineY,minLineX+lineWidth,minLineY+lineHeight);
//			    if (testAnnotatedText(minLineX,minLineY,minLineX+lineWidth,minLineY+lineHeight)){
//				    xmlNewProp(nodeline, (const xmlChar*)ATTR_HIGHLIGHT,(const xmlChar*)"yes");
//			    }

                if (word->lineNumber == false || (!is_number(word)))
                    xmlAddChild(nodeline, node);

                if (wordI < line1->words->getLength() - 1 and (word->spaceAfter == gTrue)) {
                    xmlNodePtr spacingNode = xmlNewNode(NULL, (const xmlChar *) TAG_SPACING);
                    spacingNode->type = XML_ELEMENT_NODE;
                    snprintf(tmp, sizeof(tmp), ATTR_NUMFORMAT, (nextWord->xMin - word->xMax));
                    xmlNewProp(spacingNode, (const xmlChar *) ATTR_WIDTH,
                               (const xmlChar *) tmp);
                    snprintf(tmp, sizeof(tmp), ATTR_NUMFORMAT, (word->yMin));
                    xmlNewProp(spacingNode, (const xmlChar *) ATTR_Y,
                               (const xmlChar *) tmp);
                    snprintf(tmp, sizeof(tmp), ATTR_NUMFORMAT, (word->xMax));
                    xmlNewProp(spacingNode, (const xmlChar *) ATTR_X,
                               (const xmlChar *) tmp);

                    xmlAddChild(nodeline, spacingNode);
                }

                if (!fontStyleInfo->isSuperscript() && !fontStyleInfo->isSubscript()) {
                    currentLineBaseLine = word->base;
                    currentLineYmin = word->yMin;
                    currentLineYmax = word->yMax;
                }
                previousWordBaseLine = word->base;
                previousWordYmin = word->yMin;
                previousWordYmax = word->yMax;

                free(tmp);
            }

            xmlAddChild(nodeblocks, nodeline);
        }

        xmlAddChild(printSpace, nodeblocks);
    }

    int imageCount = listeImages.size();
    for (int i = 0; i < imageCount; ++i) {

        char *tmp;

        tmp = (char *) malloc(10 * sizeof(char));

        node = xmlNewNode(NULL, (const xmlChar *) TAG_IMAGE);
        xmlNewProp(node, (const xmlChar *) ATTR_ID, (const xmlChar *) listeImages[i]->getImageId()->getCString());


        //xmlNewProp(node, (const xmlChar *) ATTR_SID,(const xmlChar*)listeImages[i]->getImageSid()->getCString());


        snprintf(tmp, sizeof(tmp), ATTR_NUMFORMAT, listeImages[i]->getXPositionImage());
        xmlNewProp(node, (const xmlChar *) ATTR_X, (const xmlChar *) tmp);
        snprintf(tmp, sizeof(tmp), ATTR_NUMFORMAT, listeImages[i]->getYPositionImage());
        xmlNewProp(node, (const xmlChar *) ATTR_Y, (const xmlChar *) tmp);
        snprintf(tmp, sizeof(tmp), ATTR_NUMFORMAT, listeImages[i]->getWidthImage());
        xmlNewProp(node, (const xmlChar *) ATTR_WIDTH, (const xmlChar *) tmp);
        snprintf(tmp, sizeof(tmp), ATTR_NUMFORMAT, listeImages[i]->getHeightImage());
        xmlNewProp(node, (const xmlChar *) ATTR_HEIGHT, (const xmlChar *) tmp);

        std::string rotation = std::to_string(listeImages[i]->getRotation());
        xmlNewProp(node,(const xmlChar*)ATTR_ROTATION,(const xmlChar*)rotation.c_str());
        //if (listeImages[i]->getRotation() > 0){
        //    xmlNewProp(node,(const xmlChar*)ATTR_ROTATION,(const xmlChar*)sTRUE);
        //}
        //else{
        //    xmlNewProp(node,(const xmlChar*)ATTR_ROTATION,(const xmlChar*)sFALSE);
        //}

//        if (listeImages[i]->isImageInline()) {
//            xmlNewProp(node, (const xmlChar *) ATTR_INLINE, (const xmlChar *) sTRUE);
//        }
        xmlNewProp(node, (const xmlChar *) ATTR_HREF,
                   (const xmlChar *) listeImages[i]->getHrefImage()->getCString());

        xmlNewProp(node, (const xmlChar *) ATTR_TYPE,
                   (const xmlChar *) listeImages[i]->getType()->getCString());
        if (verbose) {
            xmlNewProp(node, (const xmlChar *) ATTR_CLIPZONE,
                       (const xmlChar *) listeImages[i]->getClipZone()->getCString());
        }
        xmlAddChild(printSpace, node);
        free(tmp);
    }


    if (parameters->getDisplayImage()) {
        GString *sid = new GString("p");
        GBool isInline = false;
        sid = buildSID(getPageNumber(), getIdx(), sid);

        GString *relname = new GString(dataDirectory);
        relname->append("-");
        relname->append(GString::fromInt(num));
        relname->append(EXTENSION_SVG);
        char *tmp;

        tmp = (char *) malloc(10 * sizeof(char));

        node = xmlNewNode(NULL, (const xmlChar *) TAG_IMAGE);
        xmlNewProp(node, (const xmlChar *) ATTR_ID,
                   (const xmlChar *) sid->getCString());

        //xmlNewProp(node, (const xmlChar *) ATTR_SID,(const xmlChar*)listeImages[i]->getImageSid()->getCString());

        double r =0;
        snprintf(tmp, sizeof(tmp), ATTR_NUMFORMAT, svg_xmin);
        xmlNewProp(node, (const xmlChar *) ATTR_X, (const xmlChar *) tmp);
        snprintf(tmp, sizeof(tmp), ATTR_NUMFORMAT, svg_ymin);
        xmlNewProp(node, (const xmlChar *) ATTR_Y, (const xmlChar *) tmp);
        snprintf(tmp, sizeof(tmp), ATTR_NUMFORMAT, svg_xmax - svg_xmin);
        xmlNewProp(node, (const xmlChar *) ATTR_WIDTH, (const xmlChar *) tmp);
        snprintf(tmp, sizeof(tmp), ATTR_NUMFORMAT, svg_ymax - svg_ymin);
        xmlNewProp(node, (const xmlChar *) ATTR_HEIGHT, (const xmlChar *) tmp);

        std::string rotation = std::to_string(r);
        xmlNewProp(node,(const xmlChar*)ATTR_ROTATION,(const xmlChar*)rotation.c_str());
        //if (r > 0) {
        //    xmlNewProp(node, (const xmlChar *) ATTR_ROTATION, (const xmlChar *) sTRUE);
        //} else {
        //    xmlNewProp(node, (const xmlChar *) ATTR_ROTATION, (const xmlChar *) sFALSE);
        //}

//        if (listeImages[i]->isImageInline()) {
//            xmlNewProp(node, (const xmlChar *) ATTR_INLINE, (const xmlChar *) sTRUE);
//        }
        xmlNewProp(node, (const xmlChar *) ATTR_HREF,
                   (const xmlChar *) relname->getCString());

        xmlNewProp(node, (const xmlChar *) ATTR_TYPE,
                   (const xmlChar *) "svg");
        xmlAddChild(printSpace, node);
        free(tmp);

        // Save the file for example with relname 'p_06.xml_data/image-27.vec'
        if (!xmlSaveFile(relname->getCString(), vecdoc)) {
            //error(errIO,-1, "Couldn't open file '%s'", relname->getCString());
        }
        xmlFreeDoc(vecdoc);
    }

    uMap->decRefCnt();
}

// PL: Insert a block in the page's block list according to the reading order
GBool TextPage::addBlockInReadingOrder(TextParagraph * block, double fontSize, GBool lastInserted) {
    // if Y_pos of the block to be inserted is less than Y_pos of the existing block
    // (i.e. block is located above)
    // and, in case of vertical overlap,
    //		X_pos + width of the block to be inserted is less than X_pos of this existing block
    // (i.e. block is on the left and the surfaces of the block are not overlaping -
    // 2 columns case)
    // then the block order is before the existing block
    GBool notInserted = gTrue;
    int indexLowerBlock = 0, insertIndex= 0;
    GBool firstLowerBlock = gFalse;
    GBool noVerticalOverlap = gTrue;
    // we get the first child of the current page node
    unsigned long nbChildren = blocks->getLength();
    if (nbChildren > 0) {
        // coordinates of the block to be inserted
        double blockX = block->getXMin();
        double blockY = block->getYMin();

        double blockHeight = block->getYMax() - block->getYMin();
        double blockWidth = block->getXMax() - block->getXMin();

//cout << "to be inserted: " << nodeblock->name << ", X: " << blockX << ", Y: " << blockY << ", H: " << blockHeight << ", W: " << blockWidth << endl;

        TextParagraph * par;
        // we get all the block nodes in the XML tree corresponding to the page
        for (int i = 0; i <= blocks->getLength()-1 && notInserted; i++) {
            par = (TextParagraph *) blocks->get(i);
            double currentY = par->getYMin();

            double currentX = par->getXMin();

            double currentWidth = par->getXMax() - par->getXMin();
            double currentHeight = par->getYMax() - par->getXMin();

            if((currentY <= blockY && currentY + currentHeight >= blockY) ||
                    (blockY + blockHeight > currentY && blockY + blockHeight < currentY + currentHeight)){
                noVerticalOverlap = gFalse;
            }

            if (currentY < blockY)
                continue;

            if (blockY < currentY) {
                if (blockY + blockHeight < currentY) {

                    if(!notInserted)
                        continue;
                    // we keep the first block under it, if no overlap put it above
                    if(!firstLowerBlock) {
                        indexLowerBlock = i;
                        firstLowerBlock = gTrue;
                    }
                    // we don't have any vertical overlap
                    // check the X-pos, the block cannot be on the right of the current block
                    // check if the
                    if ((blockX <= currentX + currentWidth && blockX >= currentX) ||
                        (blockX <= currentX + currentWidth && blockX + blockWidth > currentX)||
                        blockX < currentX + currentWidth +fontSize * maxColSpacing
                        ) {
                        // we can insert the block before the current block
                        insertIndex = i;
                        notInserted = false;
                    }
                } else
                    noVerticalOverlap = gFalse;
            }
                // we have vertical overlap, check position on X axis

                /*double currentHeight = 0;
                attrValue = xmlGetProp(cur_node, (const xmlChar*)ATTR_HEIGHT);
                if (attrValue != NULL) {
                    currentHeight = atof((const char*)attrValue);
                    xmlFree(attrValue);
                }*/
        }
        if((lastInserted || noVerticalOverlap) && firstLowerBlock){
            insertIndex = indexLowerBlock;
            notInserted = false;
        }
            /*if (notInserted && (blockX + blockWidth < currentX)) {
                // does not work for multi column sections one after the other
                xmlNodePtr result = xmlAddPrevSibling(cur_node, nodeblock);
                notInserted = false;
            }*/
    }

    if (notInserted) {
        blocks->append(block);
        return gFalse;
    } else {
        blocks->insert(insertIndex, block); // beware, the order can be the opposite if next block in next column..
        return gTrue;
    }
}

void TextPage::addImageInlineNode(xmlNodePtr nodeline,
                                  xmlNodePtr nodeImageInline, char *tmp, IWord *word) {
    indiceImage = -1;
    idWORDBefore = -1;
    GBool first = gTrue;
    int nb = listeImageInline.size();
    for (int i = 0; i < nb; i++) {
        if (word->idWord == listeImageInline[i]->idWordBefore) {
            if (listeImageInline[i]->getXPositionImage() > word->xMax
                && listeImageInline[i]->getYPositionImage() <= word->yMax) {
                nodeImageInline = xmlNewNode(NULL, (const xmlChar *) TAG_TOKEN);
                nodeImageInline->type = XML_ELEMENT_NODE;
                GString *id;
                id = new GString("p");
                xmlNewProp(nodeImageInline, (const xmlChar *) ATTR_ID,
                           (const xmlChar *) buildIdToken(num, numToken, id)->getCString());
                delete id;
                numToken = numToken + 1;
                id = new GString("p");
                xmlNewProp(nodeImageInline, (const xmlChar *) ATTR_SID,
                           (const xmlChar *) buildSID(num, listeImageInline[i]->getIdx(), id)->getCString());
                delete id;
                snprintf(tmp, sizeof(tmp), ATTR_NUMFORMAT,
                         listeImageInline[i]->getXPositionImage());
                xmlNewProp(nodeImageInline, (const xmlChar *) ATTR_X,
                           (const xmlChar *) tmp);
                snprintf(tmp, sizeof(tmp), ATTR_NUMFORMAT,
                         listeImageInline[i]->getYPositionImage());
                xmlNewProp(nodeImageInline, (const xmlChar *) ATTR_Y,
                           (const xmlChar *) tmp);
                snprintf(tmp, sizeof(tmp), ATTR_NUMFORMAT,
                         listeImageInline[i]->getWidthImage());
                xmlNewProp(nodeImageInline, (const xmlChar *) ATTR_WIDTH,
                           (const xmlChar *) tmp);
                snprintf(tmp, sizeof(tmp), ATTR_NUMFORMAT,
                         listeImageInline[i]->getHeightImage());
                xmlNewProp(nodeImageInline, (const xmlChar *) ATTR_HEIGHT,
                           (const xmlChar *) tmp);
                xmlNewProp(nodeImageInline, (const xmlChar *) ATTR_HREF,
                           (const xmlChar *) listeImageInline[i]->getHrefImage()->getCString());
                xmlAddChild(nodeline, nodeImageInline);
                idImageInline = listeImageInline[i]->getIdImageCurrent();
            }
            int j = i;
            for (; j < nb; j++) {
                if (word->idWord == listeImageInline[j]->idWordBefore) {
                    if (listeImageInline[j]->getXPositionImage() > word->xMax
                        && listeImageInline[j]->getYPositionImage()
                           <= word->yMax) {
                        if (idImageInline
                            != listeImageInline[j]->getIdImageCurrent()) {
                            nodeImageInline = xmlNewNode(NULL,
                                                         (const xmlChar *) TAG_TOKEN);
                            nodeImageInline->type = XML_ELEMENT_NODE;
                            GString *id;
                            id = new GString("p");
                            xmlNewProp(nodeImageInline,
                                       (const xmlChar *) ATTR_ID,
                                       (const xmlChar *) buildIdToken(num, numToken, id)->getCString());
                            delete id;
                            numToken = numToken + 1;
                            id = new GString("p");
                            xmlNewProp(nodeImageInline,
                                       (const xmlChar *) ATTR_SID,
                                       (const xmlChar *) buildSID(num, listeImageInline[j]->getIdx(),
                                                                  id)->getCString());
                            delete id;
                            snprintf(tmp, sizeof(tmp), ATTR_NUMFORMAT,
                                     listeImageInline[j]->getXPositionImage());
                            xmlNewProp(nodeImageInline, (const xmlChar *) ATTR_X,
                                       (const xmlChar *) tmp);
                            snprintf(tmp, sizeof(tmp), ATTR_NUMFORMAT,
                                     listeImageInline[j]->getYPositionImage());
                            xmlNewProp(nodeImageInline, (const xmlChar *) ATTR_Y,
                                       (const xmlChar *) tmp);
                            snprintf(tmp, sizeof(tmp), ATTR_NUMFORMAT,
                                     listeImageInline[j]->getWidthImage());
                            xmlNewProp(nodeImageInline,
                                       (const xmlChar *) ATTR_WIDTH,
                                       (const xmlChar *) tmp);
                            snprintf(tmp, sizeof(tmp), ATTR_NUMFORMAT,
                                     listeImageInline[j]->getHeightImage());
                            xmlNewProp(nodeImageInline,
                                       (const xmlChar *) ATTR_HEIGHT,
                                       (const xmlChar *) tmp);
                            xmlNewProp(
                                    nodeImageInline,
                                    (const xmlChar *) ATTR_HREF,
                                    (const xmlChar *) listeImageInline[j]->getHrefImage()->getCString());
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

GString *TextPage::buildIdImage(int pageNum, int imageNum, GString *id) {
    char *tmp = (char *) malloc(10 * sizeof(char));
    sprintf(tmp, "%d", pageNum);
    id->append(tmp);
    id->append("_i");
    sprintf(tmp, "%d", imageNum);
    id->append(tmp);
    free(tmp);
    return id;
}

GString *TextPage::buildSID(int pageNum, int sid, GString *id) {
    char *tmp = (char *) malloc(10 * sizeof(char));
    sprintf(tmp, "%d", pageNum);
    id->append(tmp);
    id->append("_s");
    sprintf(tmp, "%d", sid);
    id->append(tmp);
    free(tmp);
    return id;
}

GString *TextPage::buildIdText(int pageNum, int textNum, GString *id) {
    char *tmp = (char *) malloc(10 * sizeof(char));
    sprintf(tmp, "%d", pageNum);
    id->append(tmp);
    id->append("_t");
    sprintf(tmp, "%d", textNum);
    id->append(tmp);
    free(tmp);
    return id;
}

GString *TextPage::buildIdToken(int pageNum, int tokenNum, GString *id) {
    char *tmp = (char *) malloc(10 * sizeof(char));
    sprintf(tmp, "%d", pageNum);
    id->append(tmp);
    id->append("_w");
    sprintf(tmp, "%d", tokenNum);
    id->append(tmp);
    free(tmp);
    return id;
}

GString *TextPage::buildIdBlock(int pageNum, int blockNum, GString *id) {
    char *tmp = (char *) malloc(10 * sizeof(char));
    sprintf(tmp, "%d", pageNum);
    id->append(tmp);
    id->append("_b");
    sprintf(tmp, "%d", blockNum);
    id->append(tmp);
    free(tmp);
    return id;
}

GString *TextPage::buildIdClipZone(int pageNum, int clipZoneNum, GString *id) {
    char *tmp = (char *) malloc(10 * sizeof(char));
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
                for (j = i; j < len && !unicodeTypeR(text[j]); ++j);
                for (k = i; k < j; ++k) {

                    //Check is the code point is defined and not a control one
                    if(u_isdefined(text[k]) && !u_iscntrl(text[k])) {
                        n = uMap->mapUnicode(text[k], buf, sizeof(buf));
                        s->append(buf, n);
                        ++nCols;
                    }
                }
                i = j;
                // output a right-to-left section
                for (j = i; j < len && !unicodeTypeL(text[j]); ++j);
                if (j > i) {
                    s->append(rle, rleLen);
                    for (k = j - 1; k >= i; --k) {
                        //Check is the code point is defined and not a control one
                        if(u_isdefined(text[k]) && !u_iscntrl(text[k])) {
                            n = uMap->mapUnicode(text[k], buf, sizeof(buf));
                            s->append(buf, n);
                            ++nCols;
                        }
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
                for (j = i; j >= 0 && !unicodeTypeL(text[j]); --j);
                for (k = i; k > j; --k) {
                    //Check is the code point is defined and not a control one
                    if(u_isdefined(text[k]) && !u_iscntrl(text[k])) {
                        n = uMap->mapUnicode(text[k], buf, sizeof(buf));
                        s->append(buf, n);
                        ++nCols;
                    }
                }
                i = j;
                // output a left-to-right section
                for (j = i; j >= 0 && !unicodeTypeR(text[j]); --j);
                if (j < i) {
                    s->append(lre, lreLen);
                    for (k = j + 1; k <= i; ++k) {
                        //Check is the code point is defined and not a control one
                        if(u_isdefined(text[k]) && !u_iscntrl(text[k])) {
                            n = uMap->mapUnicode(text[k], buf, sizeof(buf));
                            s->append(buf, n);
                            ++nCols;
                        }
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
            //Check is the code point is defined and not a control one
            if(u_isdefined(text[i]) && !u_iscntrl(text[i])) {
                n = uMap->mapUnicode(text[i], buf, sizeof(buf));
                s->append(buf, n);
                nCols += n;
            }
        }
    }

    return nCols;
}

void TextPage::saveState(GfxState *state) {
    idStack.push(idCur);
}

void TextPage::restoreState(GfxState *state) {
    if (!idStack.empty()) {
        idCur = idStack.top();
        idStack.pop();
    }
}

void TextPage::doPathForClip(GfxPath *path, GfxState *state,
                             xmlNodePtr currentNode) {
    double xMin = 0;
    double yMin = 0;
    double xMax = 0;
    double yMax = 0;

    // Get the clipping box
    state->getClipBBox(&xMin, &yMin, &xMax, &yMax);
    double height = yMax - yMin;
    double width = xMax - xMin;

    if(height < pageHeight && width < pageWidth) {
        char *tmp;
        tmp = (char *) malloc(500 * sizeof(char));

        // Increment the absolute object index
        idx++;

        createPath(path, state, currentNode);
        free(tmp);
    }
}

void TextPage::doPath(GfxPath *path, GfxState *state, GString *gattributes) {
    // Increment the absolute object index
    idx++;
    //printf("path %d\n",idx);
//	if (idx>10000){return;}

    xmlNodePtr groupNode = NULL;

    if (parameters->getDisplayImage()) {
        // GROUP tag
        groupNode = xmlNewNode(NULL, (const xmlChar *) TAG_GROUP);
        xmlAddChild(vecroot, groupNode);

        xmlNewProp(groupNode, (const xmlChar *) ATTR_STYLE,
                   (const xmlChar *) gattributes->getCString());


        GString *id = new GString("p"), *sid = new GString("p");//, *clipZone = new GString("p");
        GBool isInline = false;
        id = buildIdImage(getPageNumber(), numImage, id);
        sid = buildSID(getPageNumber(), getIdx(), sid);
        //clipZone = buildIdClipZone(getPageNumber(), idCur, clipZone);

        xmlNewProp(groupNode, (const xmlChar *) ATTR_SVGID, (const xmlChar *) sid->getCString());
        //xmlNewProp(groupNode, (const xmlChar *) ATTR_IDCLIPZONE, (const xmlChar *) clipZone->getCString());
        createPath(path, state, groupNode);
    }
}

void TextPage::createPath(GfxPath *path, GfxState *state, xmlNodePtr groupNode) {
    GfxSubpath *subpath;
    double x0, y0, x1, y1, x2, y2, x3, y3;
    double xmin =0 , xmax = 0 , ymin = 0, ymax=0;
    int n, m, i, j;
    double a, b;
    //char *tmp;
    //tmp = (char *) malloc(500 * sizeof(*tmp));
    static int SVG_VALUE_BUFFER_SIZE = 500;
    char *tmp = (char *)malloc(SVG_VALUE_BUFFER_SIZE * sizeof(*tmp));

    GString *d;
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
        pathnode = xmlNewNode(NULL, (const xmlChar *) TAG_PATH);
        //snprintf(tmp, SVG_VALUE_BUFFER_SIZE, "M%g,%g", x0, y0);
        snprintf(tmp, SVG_VALUE_BUFFER_SIZE, "M%1.4f,%1.4f", x0, y0);

        d = new GString(tmp);

//        sprintf(tmp, ATTR_NUMFORMAT, y0);
//        xmlNewProp(pathnode, (const xmlChar*)ATTR_Y, (const xmlChar*)tmp);

        j = 1;
        while (j < m) {
            if (subpath->getCurve(j)) {
                x1 = subpath->getX(j);
                y1 = subpath->getY(j);
                x2 = subpath->getX(j + 1);
                y2 = subpath->getY(j + 1);
                x3 = subpath->getX(j + 2);
                y3 = subpath->getY(j + 2);
                state->transform(x1, y1, &a, &b);
                x1 = a;
                y1 = b;
                state->transform(x2, y2, &a, &b);
                x2 = a;
                y2 = b;
                state->transform(x3, y3, &a, &b);
                x3 = a;
                y3 = b;
                // C tag  : curveto
//                pathnode=xmlNewNode(NULL, (const xmlChar*)TAG_C);
                snprintf(tmp, SVG_VALUE_BUFFER_SIZE, " C%1.4f,%1.4f %1.4f,%1.4f %1.4f,%1.4f", x1, y1, x2, y2, x3, y3);
                d->append(tmp);
                if(xmax==0) {
                    double list_double[] = {x0, x1, x2, x3};
                    xmax = *std::max_element(list_double, list_double +4);
                } else {
                    double list_double[] = {x0, x1, x2, x3, xmax};
                    xmax = *std::max_element(list_double, list_double+5);
                }

                if(xmin==0) {
                    double list_double[] = {x0, x1, x2, x3};
                    xmin = *std::min_element(list_double, list_double+4);
                } else {
                    double list_double[] = {x0, x1, x2, x3, xmin};
                    xmin = *std::min_element(list_double, list_double+5);
                }

                if(ymax==0){
                    double list_double[] = { y0, y1, y2, y3};
                    ymax = *std::max_element(list_double, list_double +4);
                }
                else {
                    double list_double[] = { y0, y1, y2, y3};
                    ymax = *std::max_element(list_double, list_double+5);
                }

                if(ymin==0) {
                    double list_double[] = { y0, y1, y2, y3};
                    ymin = *std::min_element(list_double, list_double+4);
                }else {
                    double list_double[] = {y0, y1, y2, y3, ymin};
                    ymin = *std::min_element(list_double, list_double+5);
                }
                j += 3;
            } else {
                x1 = subpath->getX(j);
                y1 = subpath->getY(j);
                state->transform(x1, y1, &a, &b);
                x1 = a;
                y1 = b;

                // L tag : lineto
//                pathnode=xmlNewNode(NULL, (const xmlChar*)TAG_L);
//                sprintf(tmp, ATTR_NUMFORMAT, x1);
//                xmlNewProp(pathnode, (const xmlChar*)ATTR_X,
//                           (const xmlChar*)tmp);
//                sprintf(tmp, ATTR_NUMFORMAT, y1);
//                xmlNewProp(pathnode, (const xmlChar*)ATTR_Y,
//                           (const xmlChar*)tmp);
//                xmlAddChild(groupNode, pathnode);
                snprintf(tmp, SVG_VALUE_BUFFER_SIZE," L%1.4f,%1.4f", x1, y1);
                d->append(tmp);
                if (xmax == 0) {
                    double list_double[] = {x0, x1};
                    xmax = *std::max_element(list_double, list_double+2);
                } else {
                    double list_double[] = {x0, x1, xmax};
                    xmax = *std::max_element(list_double, list_double+3);
                }

                if (xmin == 0) {
                    double list_double[] = {x0, x1};
                    xmin = *std::min_element(list_double, list_double+2);
                } else {
                    double list_double[] = {x0, x1, xmin};
                    xmin = *std::min_element(list_double, list_double+3);
                }

                if (ymax == 0) {
                    double list_double[] = {y0, y1};
                    ymax = *std::max_element(list_double, list_double+2);
                }else {
                    double list_double[] = {y0, y1, ymax};
                    ymax = *std::max_element(list_double, list_double+3);
                }

                if(ymin==0) {
                    double list_double[] = {y0, y1};
                    ymin = *std::min_element(list_double, list_double+2);
                }else {
                    double list_double[] = {y0, y1, ymin};
                    ymin = *std::min_element(list_double, list_double+3);
                }
                ++j;
            }
        }

        if (subpath->isClosed()) {
//            if (!xmlHasProp(groupNode, (const xmlChar*)ATTR_CLOSED)) {
//                xmlNewProp(groupNode, (const xmlChar*)ATTR_CLOSED,
//                           (const xmlChar*)sTRUE);
//            }
            d->append(" Z");
        }

        if(svg_xmax == 0 && svg_ymin == 0 && svg_ymax == 0 && svg_xmin == 0){
            svg_xmin = xmin;
            svg_xmax = xmax;
            svg_ymin = ymin;
            svg_ymax = ymax;
        } else {
            if (svg_xmin > xmin)
                svg_xmin = xmin;
            if (svg_xmax < xmax)
                svg_xmax = xmax;
            if (svg_ymin > ymin)
                svg_ymin = ymin;
            if (svg_ymax < ymax)
                svg_ymax = ymax;
        }

        xmlNewProp(pathnode, (const xmlChar *) ATTR_D, (const xmlChar *) d->getCString());
        xmlAddChild(groupNode, pathnode);
    }
    delete d;
    free(tmp);
}

void TextPage::clip(GfxState *state) {
    idClip++;
    idCur = idClip;

    xmlNodePtr gnode = NULL;
    char tmp[100];

    // Increment the absolute object index
    idx++;
    //printf("clip %d\n",idx);
//	if (idx>10000){
//		return;
//	}
    if (parameters->getDisplayImage()) {

        // CLIP tag
        gnode = xmlNewNode(NULL, (const xmlChar *) TAG_CLIPPATH);
        xmlAddChild(vecroot, gnode);

        GString *id = new GString("p"), *sid = new GString("p");//, *clipZone = new GString("p");
        GBool isInline = false;
        id = buildIdImage(getPageNumber(), numImage, id);
        sid = buildSID(getPageNumber(), getIdx(), sid);
        //clipZone = buildIdClipZone(getPageNumber(), idCur, clipZone);

        xmlNewProp(gnode, (const xmlChar *) ATTR_SVGID, (const xmlChar *) sid->getCString());
        //xmlNewProp(gnode, (const xmlChar *) ATTR_IDCLIPZONE, (const xmlChar *) clipZone->getCString());
        //   	free(tmp);
        doPathForClip(state->getPath(), state, gnode);
    }
}

void TextPage::eoClip(GfxState *state) {

    idClip++;
    idCur = idClip;

    xmlNodePtr gnode = NULL;
    char tmp[100];

    // Increment the absolute object index
    idx++;
    if (parameters->getDisplayImage()) {
        // CLIP tag
        gnode = xmlNewNode(NULL, (const xmlChar *) TAG_CLIPPATH);
        xmlAddChild(vecroot, gnode);

        GString *id = new GString("p"), *sid = new GString("p");//, *clipZone = new GString("p");
        GBool isInline = false;
        id = buildIdImage(getPageNumber(), numImage, id);
        sid = buildSID(getPageNumber(), getIdx(), sid);
        //clipZone = buildIdClipZone(getPageNumber(), idCur, clipZone);

        xmlNewProp(gnode, (const xmlChar *) ATTR_SVGID, (const xmlChar *) sid->getCString());
        //xmlNewProp(gnode, (const xmlChar *) ATTR_IDCLIPZONE, (const xmlChar *) clipZone->getCString());

        xmlNewProp(gnode, (const xmlChar *) ATTR_EVENODD, (const xmlChar *) sTRUE);
        //   	free(tmp);
        doPathForClip(state->getPath(), state, gnode);
    }
}

void TextPage::clipToStrokePath(GfxState *state) {
    idClip++;
    idCur = idClip;

    xmlNodePtr gnode = NULL;
    char tmp[100];

    // Increment the absolute object index
    idx++;

    if (parameters->getDisplayImage()) {
        // CLIP tag
        gnode = xmlNewNode(NULL, (const xmlChar *) TAG_CLIPPATH);
        xmlAddChild(vecroot, gnode);

        GString *id = new GString("p"), *sid = new GString("p");//, *clipZone = new GString("p");
        GBool isInline = false;
        id = buildIdImage(getPageNumber(), numImage, id);
        sid = buildSID(getPageNumber(), getIdx(), sid);
        //clipZone = buildIdClipZone(getPageNumber(), idCur, clipZone);

        xmlNewProp(gnode, (const xmlChar *) ATTR_SVGID, (const xmlChar *) sid->getCString());
        //xmlNewProp(gnode, (const xmlChar *) ATTR_IDCLIPZONE, (const xmlChar *) clipZone->getCString());

        xmlNewProp(gnode, (const xmlChar *) ATTR_EVENODD, (const xmlChar *) sTRUE);
        //   	free(tmp);
        doPathForClip(state->getPath(), state, gnode);

    }
}

//// Draw the image mask
//const char* TextPage::drawImageMask(GfxState *state, Object *ref, Stream *str,
//                             int width, int height, GBool invert, GBool inlineImg, GBool dumpJPEG,
//                             int imageIndex) {
//
//
//    // ugly code / to be simplified
//
//    int i;
//    FILE *f;
//    int c;
//    int size;
//    GString *id;
//
//    double x0, y0; // top left corner of image
//    double w0, h0, w1, h1; // size of image
//    double xt, yt, wt, ht;
//    GBool rotate, xFlip, yFlip;
//    char tmp[10];
//
//    const char* extension = NULL;
//
//    xmlNodePtr node = NULL;
//
//    // Increment the absolute object index
//    idx++;
//
//    // get image position and size
//    state->transform(0, 0, &xt, &yt);
//    state->transformDelta(1, 1, &wt, &ht);
//    if (wt > 0) {
//        x0 = xt;
//        w0 = wt;
//    } else {
//        x0 = xt + wt;
//        w0 = -wt;
//    }
//    if (ht > 0) {
//        y0 = yt;
//        h0 = ht;
//    } else {
//        y0 = yt + ht;
//        h0 = -ht;
//    }
//    state->transformDelta(1, 0, &xt, &yt);
//    rotate = fabs(xt) < fabs(yt);
//    if (rotate) {
//        w1 = h0;
//        h1 = w0;
//        xFlip = ht < 0;
//        yFlip = wt > 0;
//    } else {
//        w1 = w0;
//        h1 = h0;
//        xFlip = wt < 0;
//        yFlip = ht > 0;
//    }
//
//
//
//    // HREF
//    GString *relname = new GString(RelfileName);
//    relname->append("-");
//    relname->append(GString::fromInt(imageIndex));
//    GString *refname = new GString(ImgfileName);
//    refname->append("-");
//    refname->append(GString::fromInt(imageIndex));
//    if(dumpJPEG && str->getKind() == strDCT && !inlineImg) {
//        extension = ".jpg";
//        relname->append(".jpg");
//        refname->append(".jpg");
//        // initialize stream
//        str = ((DCTStream *)str)->getRawStream();
//        str->reset();
//
//        // copy the stream
//        while ((c = str->getChar()) != EOF)
//            fputc(c, f);
//
//        str->close();
//        fclose(f);
//    } else {
//        // open the image file and write the PBM header
//        extension = ".pbm";
//        relname->append(".pbm");
//        refname->append(".pbm");
//
//        if (!(f = fopen(relname->getCString(), "wb"))) {
//            //error(-1, "Couldn't open image file '%s'", relname->getCString());
//            return extension;
//        }
//        fprintf(f, "P4\n");
//        fprintf(f, "%d %d\n", width, height);
//
//        // initialize stream
//        str->reset();
//
//        // copy the stream
//        size = height * ((width + 7) / 8);
//        for (i = 0; i < size; ++i) {
//            fputc(str->getChar(), f);
//        }
//
//        str->close();
//        fclose(f);
//    }
//
//    if (!inlineImg || (inlineImg && parameters->getImageInline())) {
//        node = xmlNewNode(NULL, (const xmlChar*)TAG_IMAGE);
//        GString *id;
//        id = new GString("p");
//        xmlNewProp(node, (const xmlChar*)ATTR_ID, (const xmlChar*)buildIdImage(num, numImage, id)->getCString());
//        delete id;
//        numImage = numImage + 1;
//
//        id = new GString("p");
//        xmlNewProp(node, (const xmlChar*)ATTR_SID, (const xmlChar*)buildSID(num, getIdx(), id)->getCString());
//        delete id;
//
//        sprintf(tmp, ATTR_NUMFORMAT, x0);
//        xmlNewProp(node, (const xmlChar*)ATTR_X, (const xmlChar*)tmp);
//        sprintf(tmp, ATTR_NUMFORMAT, y0);
//
//        xmlNewProp(node, (const xmlChar*)ATTR_Y, (const xmlChar*)tmp);
//        sprintf(tmp, ATTR_NUMFORMAT, w0);
//        xmlNewProp(node, (const xmlChar*)ATTR_WIDTH, (const xmlChar*)tmp);
//        sprintf(tmp, ATTR_NUMFORMAT, h0);
//        xmlNewProp(node, (const xmlChar*)ATTR_HEIGHT, (const xmlChar*)tmp);
//        if (inlineImg) {
//            xmlNewProp(node, (const xmlChar*)ATTR_INLINE, (const xmlChar*)sTRUE);
//        }
//        xmlNewProp(node, (const xmlChar*)ATTR_MASK, (const xmlChar*)sTRUE);
//        xmlNewProp(node, (const xmlChar*)ATTR_HREF,
//                   (const xmlChar*)refname->getCString());
//        xmlAddChild(printSpace, node);
//    }
//
//    if (inlineImg && !parameters->getImageInline()) {
//        listeImageInline.push_back(new ImageInline(x0, y0, w0, h0, getIdWORD(), imageIndex, refname, getIdx()));
//    }
//
//    id = new GString("p");
//    xmlNewProp(node, (const xmlChar*)ATTR_CLIPZONE,
//               (const xmlChar*)buildIdClipZone(num, idCur, id)->getCString());
//    delete id;
//    //  free(tmp);
//    return extension;
//}

const char *TextPage::drawImageOrMask(GfxState *state, Object *ref, Stream *str,
                                      int width, int height,
                                      GfxImageColorMap *colorMap,
                                      int * /* maskColors */, GBool inlineImg, GBool mask, int imageIndex) {
    GString pic_file;

    double x0, y0; // top left corner of image
    double w0, h0, w1, h1; // size of image
    double xt, yt, wt, ht;

    GBool rotate, xFlip, yFlip;
    GString *id;

    xmlNodePtr node = NULL;
    const char *extension = NULL;

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
    if (x1 > x2) {
        flip |= 1;
        flip_x = true;
        temp = x1;
        x1 = x2;
        x2 = temp;
    }
    if (y1 > y2) {
        flip |= 2;
        flip_y = true;
        temp = y1;
        y1 = y2;
        y2 = temp;
    }
    GString *relname = new GString(dataDirectory);
    relname->append("-");
    relname->append(GString::fromInt(imageIndex));

    GString *refname = new GString(dataDirectory);
    refname->append("-");
    refname->append(GString::fromInt(imageIndex));

//	if (pic_file.getLength() == 0)
    if (1) {
        extension = EXTENSION_PNG;
        relname->append(EXTENSION_PNG);
        refname->append(EXTENSION_PNG);
        // ------------------------------------------------------------
        // dump JPEG file
        // ------------------------------------------------------------

        if (str->getKind() == strDCT && (mask || colorMap->getNumPixelComps() == 3) && !inlineImg) {
            // TODO, do we need to flip Jpegs too?

            // open image file
//			compose_image_filename(dev_picture_base, ++dev_picture_number, extension, pic_file);

            FILE *img_file = fopen(relname->getCString(), "wb");
            if (img_file != NULL) {
                // initialize stream
                str = ((DCTStream *) str)->getRawStream();
                str->reset();

                int c;

                // copy the stream
                while ((c = str->getChar()) != EOF) {
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

        else if (mask || (colorMap->getNumPixelComps() == 1 && colorMap->getBits() == 1)) {
//			compose_image_filename(dev_picture_base, ++dev_picture_number, extension, pic_file);

            int stride = (width + 7) >> 3;
            unsigned char *data = new unsigned char[stride * height];

            if (data != NULL) {
                str->reset();

                // Prepare increments and initial value for flipping
                int k, x_increment, y_increment;

                if (flip_x) {
                    if (flip_y) {
                        // both flipped
                        k = height * stride - 1;
                        x_increment = -1;
                        y_increment = 0;
                    } else {
                        // x flipped
                        k = (stride - 1);
                        x_increment = -1;
                        y_increment = 2 * stride;
                    }
                } else {
                    if (flip_y) {
                        // y flipped
                        k = (height - 1) * stride;
                        x_increment = 1;
                        y_increment = -2 * stride;
                    } else {
                        // not flipped
                        k = 0;
                        x_increment = 1;
                        y_increment = 0;
                    }
                }

                // Retrieve the image raw data (columnwise monochrome pixels)
                for (int y = 0; y < height; y++) {
                    for (int x = 0; x < stride; x++) {
                        data[k] = (unsigned char) str->getChar();
                        k += x_increment;
                    }

                    k += y_increment;
                }

                // there is more if the image is flipped in x...
                if (flip_x) {
                    int total = height * stride;
                    unsigned char a;

                    // bitwise flip of all bytes:
                    for (k = 0; k < total; k++) {
                        a = data[k];
                        a = (a >> 4) + (a << 4);
                        a = ((a & 0xCC /* 11001100b */) >> 2) + ((a & 0x33 /* 00110011b */) << 2);
                        data[k] = ((a & 0xAA /* 10101010b */) >> 1) + ((a & 0x55 /* 01010101b */) << 1);
                    }

                    int complementary_shift = (width & 7);

                    if (complementary_shift != 0) {
                        // now shift everything <shift> bits
                        int shift = 8 - complementary_shift;
                        unsigned char mask = 0xFF << complementary_shift;    // mask for remainder
                        unsigned char b;
                        unsigned char remainder = 0; // remainder is part that comes out when shifting
                        // a byte which is reintegrated in the next byte

                        for (k = total - 1; k >= 0; k--) {
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

        else {
            unsigned char *data = new unsigned char[width * height * 3];

            if (data != NULL) {
                if (colorMap->getBits() > 0) {
                    ImageStream *imgStr = new ImageStream(str, width, colorMap->getNumPixelComps(),
                                                          colorMap->getBits());
                    imgStr->reset();

                    GfxRGB rgb;

                    // Prepare increments and initial value for flipping
                    int k, x_increment, y_increment;

                    if (flip_x) {
                        if (flip_y) {
                            // both flipped
                            k = 3 * height * width - 3;
                            x_increment = -6;
                            y_increment = 0;
                        } else {
                            // x flipped
                            k = 3 * (width - 1);
                            x_increment = -6;
                            y_increment = 6 * width;
                        }
                    } else {
                        if (flip_y) {
                            // y flipped
                            k = 3 * (height - 1) * width;
                            x_increment = 0;
                            y_increment = -6 * width;
                        } else {
                            // not flipped
                            k = 0;
                            x_increment = 0;
                            y_increment = 0;
                        }
                    }

                    // Retrieve the image raw data (RGB pixels)
                    for (int y = 0; y < height; y++) {
                        Guchar *p = imgStr->getLine();
                        if (p) {
                            for (int x = 0; x < width; x++) {
                                GfxRenderingIntent ri;
                                colorMap->getRGB(p, &rgb, ri);
                                data[k++] = clamp(rgb.r >> 8);
                                data[k++] = clamp(rgb.g >> 8);
                                data[k++] = clamp(rgb.b >> 8);
                                k += x_increment;
                                p += colorMap->getNumPixelComps();
                            }
                        }

                        k += y_increment;
                    }

                    delete imgStr;

                    // Save PNG file
                    save_png(relname, width, height, width * 3, data, 24, PNG_COLOR_TYPE_RGB, NULL, 0);
                }
            }
        }

    }
    if (!inlineImg) {
        GString *id = new GString("p"), *sid = new GString("p"), *clipZone = new GString("p");
        GBool isInline = false;
        id = buildIdImage(getPageNumber(), numImage, id);
        sid = buildSID(getPageNumber(), getIdx(), sid);
        clipZone = buildIdClipZone(getPageNumber(), idCur, clipZone);

        numImage = numImage + 1;

        if (inlineImg) {
            isInline = true;
        }

        listeImages.push_back(new Image(x0, y0, w0, h0, id, sid, refname, clipZone, isInline, 0 , new GString("image")));
//        delete sid;
//        delete id;
//        delete clipZone;
    }

    if (inlineImg && parameters->getImageInline()) {
        listeImageInline.push_back(new ImageInline(x0, y0, w0, h0, getIdWORD(), imageIndex, refname, getIdx()));
    }


    return extension;
//	append_image_block(round(x1), round(y1), round(x2-x1), round(y2-y1), pic_file);
}


void file_write_data(png_structp png_ptr, png_bytep data, png_size_t length) {
    FILE *file = (FILE *) png_ptr->io_ptr;

    if (fwrite(data, 1, length, file) != length)
        png_error(png_ptr, "Write Error");
}


void file_flush_data(png_structp png_ptr) {
    FILE *file = (FILE *) png_ptr->io_ptr;

    if (fflush(file))
        png_error(png_ptr, "Flush Error");
}


bool TextPage::save_png(GString *file_name,
                        unsigned int width, unsigned int height, unsigned int row_stride,
                        unsigned char *data,
                        unsigned char bpp, unsigned char color_type, png_color *palette, unsigned short color_count) {
    png_struct *png_ptr;
    png_info *info_ptr;

    // Create necessary structs
    png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (png_ptr == NULL) {
        return false;
    }

    info_ptr = png_create_info_struct(png_ptr);
    if (info_ptr == NULL) {
        png_destroy_write_struct(&png_ptr, (png_infopp) NULL);
        return false;
    }

    // Open file
    FILE *file = fopen(file_name->getCString(), "wb");
    if (file == NULL) {
        png_destroy_write_struct(&png_ptr, (png_infopp) NULL);
        return false;
    }

    if (setjmp(png_ptr->jmpbuf)) {
        png_destroy_write_struct(&png_ptr, (png_infopp) &info_ptr);
        fclose(file);
        return false;
    }

    // Writing functions
    png_set_write_fn(png_ptr, file, (png_rw_ptr) file_write_data, (png_flush_ptr) file_flush_data);

    // Image header
    info_ptr->width = width;
    info_ptr->height = height;
    info_ptr->pixel_depth = bpp;
    info_ptr->channels = (bpp > 8) ? (unsigned char) 3 : (unsigned char) 1;
    info_ptr->bit_depth = (unsigned char) (bpp / info_ptr->channels);
    info_ptr->color_type = color_type;
    info_ptr->compression_type = info_ptr->filter_type = 0;
    info_ptr->valid = 0;
    info_ptr->rowbytes = row_stride;
    info_ptr->interlace_type = PNG_INTERLACE_NONE;

    // Background
    png_color_16 image_background = {0, 255, 255, 255, 0};
    png_set_bKGD(png_ptr, info_ptr, &image_background);

    // Metrics
    png_set_pHYs(png_ptr, info_ptr, 3780, 3780, PNG_RESOLUTION_METER); // 3780 dot per meter

    // Palette
    if (palette != NULL) {
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
    for (int pass = 0; pass < num_pass; pass++) {
        for (unsigned int y = 0; y < height; y++) {
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
                                   GString *cmdA) {
    text = NULL;
    splash = NULL;
    fontEngine = NULL;
    physLayout = physLayoutA;
    //rawOrder = 1;
    ok = gTrue;
    doc = NULL;
    docroot = NULL;
    verbose = verboseA;
    GString *imgDirName;
    Catalog *myCatalog;

    if (parameters->getReadingOrder() == gTrue)
        readingOrder = 1;
    else
        readingOrder = 0;

    //font = NULL;
    needFontUpdate = gFalse;
    fontEngine = NULL;

    nT3Fonts = 0;

    unicode_map = new GHash(gTrue);
    //initialise some special unicodes 9 to begin with as placeholders, from https://unicode.org/charts/PDF/U2B00.pdf
    placeholders.push_back((Unicode) 9724);
    placeholders.push_back((Unicode) 9650);
    placeholders.push_back((Unicode) 9658);
    placeholders.push_back((Unicode) 9670);
    placeholders.push_back((Unicode) 9675);
    placeholders.push_back((Unicode) 9671);
    placeholders.push_back((Unicode) 9679);
    placeholders.push_back((Unicode) 9678);
    placeholders.push_back((Unicode) 9725);
    placeholders.push_back((Unicode) 9720);
    placeholders.push_back((Unicode) 9721);
    placeholders.push_back((Unicode) 9722);

    //curstate=(double*)malloc(10000*sizeof(6*double));

    myCatalog = catalog;
    UnicodeMap *uMap;

    //useBlocks = parameters->getDisplayBlocks();
    //useBlocks = gTrue;
    noLineNumbers = parameters->getNoLineNumbers();
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
    dataDir->append(NAME_DATA_DIR);

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

    doc = xmlNewDoc((const xmlChar *) VERSION);

    globalParams->setTextEncoding((char *) ENCODING_UTF8);
    doc->encoding = xmlStrdup((const xmlChar *) ENCODING_UTF8);
    docroot = xmlNewNode(NULL, (const xmlChar *) TAG_ALTO);

    xmlNsPtr nsXi = xmlNewNs(docroot, (const xmlChar *) ALTO_URI,
                             NULL);
    xmlSetNs(docroot, nsXi);

    // The namespace DS to add at the DOCUMENT tag
    if (nsURI) {
        xmlNewNs(docroot, (const xmlChar *) nsURI->getCString(), NULL);
    }

    xmlDocSetRootElement(doc, docroot);

    // here we add basic structure : see https://www.loc.gov/standards/alto/techcenter/structure.html
    xmlNodePtr nodeDescription = xmlNewNode(NULL, (const xmlChar *) TAG_DESCRIPTION);
    nodeDescription->type = XML_ELEMENT_NODE;
    xmlAddChild(docroot, nodeDescription);


    xmlNodePtr nodeStyles = xmlNewNode(NULL, (const xmlChar *) TAG_STYLES);
    nodeStyles->type = XML_ELEMENT_NODE;
    xmlAddChild(docroot, nodeStyles);

    xmlNodePtr nodeLayout = xmlNewNode(NULL, (const xmlChar *) TAG_LAYOUT);
    nodeLayout->type = XML_ELEMENT_NODE;
    xmlAddChild(docroot, nodeLayout);

    // adding the processing description
    xmlNodePtr nodeMeasurementUnit = xmlNewNode(NULL, (const xmlChar *) TAG_MEASUREMENTUNIT);
    nodeMeasurementUnit->type = XML_ELEMENT_NODE;
    xmlNodeSetContent(nodeMeasurementUnit,
                      (const xmlChar *) xmlEncodeEntitiesReentrant(nodeMeasurementUnit->doc,
                                                                   (const xmlChar *) MEASUREMENT_UNIT));
    xmlAddChild(nodeDescription, nodeMeasurementUnit);

    xmlNodePtr nodeSourceImageInformation = xmlNewNode(NULL, (const xmlChar *) TAG_SOURCE_IMAGE_INFO);
    nodeSourceImageInformation->type = XML_ELEMENT_NODE;
    xmlAddChild(nodeDescription, nodeSourceImageInformation);

    xmlNodePtr nodeNameFilePdf = xmlNewNode(NULL,
                                            (const xmlChar *) TAG_PDFFILENAME);
    nodeNameFilePdf->type = XML_ELEMENT_NODE;
    if (!(uMap = globalParams->getTextEncoding())) {
        return;
    }
    GString *title;
    title = new GString();
    title = toUnicode(fileNamePDF, uMap);
//	dumpFragment((Unicode*)fileNamePDF, fileNamePDF->getLength(), uMap, title);
    xmlNodeSetContent(nodeNameFilePdf,
                      (const xmlChar *) xmlEncodeEntitiesReentrant(nodeNameFilePdf->doc,
                                                                   (const xmlChar *) title->getCString()));
    xmlAddChild(nodeSourceImageInformation, nodeNameFilePdf);

    xmlNodePtr nodeOCRProcessing = xmlNewNode(NULL, (const xmlChar *) TAG_OCRPROCESSING);
    nodeOCRProcessing->type = XML_ELEMENT_NODE;
    xmlAddChild(nodeDescription, nodeOCRProcessing);
    xmlNewProp(nodeOCRProcessing, (const xmlChar *) ATTR_ID,
               (const xmlChar *) ATTR_VALUEID_OCRPROCESSING);

    xmlNodePtr nodeOCRProcessingStep = xmlNewNode(NULL, (const xmlChar *) TAG_OCRPROCESSINGSTEP);
    nodeOCRProcessingStep->type = XML_ELEMENT_NODE;
    xmlAddChild(nodeOCRProcessing, nodeOCRProcessingStep);

    xmlNodePtr nodeProcessingDate = xmlNewNode(NULL, (const xmlChar *) TAG_PROCESSINGDATE);
    nodeProcessingDate->type = XML_ELEMENT_NODE;
    xmlAddChild(nodeOCRProcessingStep, nodeProcessingDate);
    time_t t;
    time(&t);
    char tstamp[sizeof "YYYY-MM-DDTHH:MM:SSZ"];
    strftime(tstamp, sizeof tstamp, "%FT%TZ", gmtime(&t));
    xmlNodeSetContent(nodeProcessingDate, (const xmlChar *) xmlEncodeEntitiesReentrant(
            nodeProcessingDate->doc, (const xmlChar *) tstamp));

    xmlNodePtr nodeProcessingSoftware = xmlNewNode(NULL, (const xmlChar *) TAG_PROCESSINGSOFTWARE);
    nodeProcessingSoftware->type = XML_ELEMENT_NODE;
    xmlAddChild(nodeOCRProcessingStep, nodeProcessingSoftware);

    xmlNodePtr nodeSoftwareCreator = xmlNewNode(NULL, (const xmlChar *) TAG_SOFTWARE_CREATOR);
    nodeSoftwareCreator->type = XML_ELEMENT_NODE;
    xmlNodeSetContent(nodeSoftwareCreator,
                      (const xmlChar *) xmlEncodeEntitiesReentrant(nodeSoftwareCreator->doc,
                                                                   (const xmlChar *) PDFALTO_CREATOR));
    xmlAddChild(nodeProcessingSoftware, nodeSoftwareCreator);

    xmlNodePtr nodeSoftwareName = xmlNewNode(NULL, (const xmlChar *) TAG_SOFTWARE_NAME);
    nodeSoftwareName->type = XML_ELEMENT_NODE;
    xmlNodeSetContent(nodeSoftwareName,
                      (const xmlChar *) xmlEncodeEntitiesReentrant(nodeSoftwareName->doc,
                                                                   (const xmlChar *) PDFALTO_NAME));
    xmlAddChild(nodeProcessingSoftware, nodeSoftwareName);
//    xmlNewProp(nodeProcess, (const xmlChar*)ATTR_CMD,
//               (const xmlChar*)cmdA->getCString());

    xmlNodePtr nodeSoftwareVersion = xmlNewNode(NULL, (const xmlChar *) TAG_SOFTWARE_VERSION);
    nodeSoftwareVersion->type = XML_ELEMENT_NODE;
    xmlNodeSetContent(nodeSoftwareVersion,
                      (const xmlChar *) xmlEncodeEntitiesReentrant(nodeSoftwareName->doc,
                                                                   (const xmlChar *) PDFALTO_VERSION));
    xmlAddChild(nodeProcessingSoftware, nodeSoftwareVersion);

    needClose = gFalse;

    delete fileNamePDF;

    //
    text = new TextPage(verbose, catalog, nodeLayout, imgDirName, baseFileName, nsURI);
}

XmlAltoOutputDev::~XmlAltoOutputDev() {
    int j;

    for (j = 0; j < nT3Fonts; ++j) {
        delete t3FontCache[j];
    }
    if (fontEngine) {
        delete fontEngine;
    }
    if (splash) {
        delete splash;
    }

    xmlSaveFile(myfilename->getCString(), doc);
    xmlFreeDoc(doc);

    for (int i = 0; i < lPictureReferences->getLength(); i++) {
        delete ((PictureReference *) lPictureReferences->get(i));
    }
    if (text) {
        delete text;
    }
    if (nsURI) {
        delete nsURI;
    }
}

void XmlAltoOutputDev::beginActualText(GfxState *state, Unicode *u, int uLen) {
    text->beginActualText(state, u, uLen);
}

void XmlAltoOutputDev::endActualText(GfxState *state) {
    SplashFont *splashFont = NULL;
    if (parameters->getOcr() == gTrue) {
        SplashCoord mat[6];
//        mat[0] = (SplashCoord) curstate[0];
//        mat[1] = (SplashCoord) curstate[1];
//        mat[2] = (SplashCoord) curstate[2];
//        mat[3] = (SplashCoord) curstate[3];
//        mat[4] = (SplashCoord) curstate[4];
//        mat[5] = (SplashCoord) curstate[5];
        //splashFont = getSplashFont(state, mat);
    }
    text->endActualText(state, splashFont);
}

void XmlAltoOutputDev::initMetadataInfoDoc() {
    char *tmp = (char *) malloc(10 * sizeof(char));
    docMetadata = xmlNewDoc((const xmlChar *) VERSION);
    globalParams->setTextEncoding((char *) ENCODING_UTF8);
    docMetadataRoot = xmlNewNode(NULL, (const xmlChar *) TAG_METADATA);
    xmlDocSetRootElement(docMetadata, docMetadataRoot);
}

GBool XmlAltoOutputDev::needNonText() {
    if (parameters->getDisplayImage())
        return gTrue;
    else return gFalse;
}

void XmlAltoOutputDev::addMetadataInfo(PDFDocXrce *pdfdocxrce) {
    Object info;

    GString *content;

    //xmlNodePtr nodeSourceImageInfo = findNodeByName(docroot, (const xmlChar*)TAG_SOURCE_IMAGE_INFO);

    xmlNodePtr titleNode = xmlNewNode(NULL, (const xmlChar *) "TITLE");
    xmlNodePtr subjectNode = xmlNewNode(NULL, (const xmlChar *) "SUBJECT");
    xmlNodePtr keywordsNode = xmlNewNode(NULL, (const xmlChar *) "KEYWORDS");
    xmlNodePtr authorNode = xmlNewNode(NULL, (const xmlChar *) "AUTHOR");
    xmlNodePtr creatorNode = xmlNewNode(NULL, (const xmlChar *) "CREATOR");
    xmlNodePtr producerNode = xmlNewNode(NULL, (const xmlChar *) "PRODUCER");
    xmlNodePtr creationDateNode = xmlNewNode(NULL, (const xmlChar *) "CREATIONDATE");
    xmlNodePtr modDateNode = xmlNewNode(NULL, (const xmlChar *) "MODIFICATIONDATE");


    titleNode->type = XML_ELEMENT_NODE;
    subjectNode->type = XML_ELEMENT_NODE;
    keywordsNode->type = XML_ELEMENT_NODE;
    authorNode->type = XML_ELEMENT_NODE;
    creatorNode->type = XML_ELEMENT_NODE;
    producerNode->type = XML_ELEMENT_NODE;
    creationDateNode->type = XML_ELEMENT_NODE;
    modDateNode->type = XML_ELEMENT_NODE;

    xmlAddChild(docMetadataRoot, titleNode);
    xmlAddChild(docMetadataRoot, subjectNode);
    xmlAddChild(docMetadataRoot, keywordsNode);
    xmlAddChild(docMetadataRoot, authorNode);
    xmlAddChild(docMetadataRoot, creatorNode);
    xmlAddChild(docMetadataRoot, producerNode);
    xmlAddChild(docMetadataRoot, creationDateNode);
    xmlAddChild(docMetadataRoot, modDateNode);

//	xmlNewProp(nodeVersion, (const xmlChar*)ATTR_VALUE,(const xmlChar*)PDFTOXML_VERSION);
//
//	infoNode =
    pdfdocxrce->getDocInfo(&info);
    if (info.isDict()) {

        content = getInfoString(info.getDict(), "Title");
        xmlNodeSetContent(titleNode, (const xmlChar *) xmlEncodeEntitiesReentrant(
                titleNode->doc, (const xmlChar *) content->getCString()));

        content = getInfoString(info.getDict(), "Subject");
        xmlNodeSetContent(subjectNode, (const xmlChar *) xmlEncodeEntitiesReentrant(
                subjectNode->doc, (const xmlChar *) content->getCString()));

        content = getInfoString(info.getDict(), "Keywords");
        xmlNodeSetContent(keywordsNode, (const xmlChar *) xmlEncodeEntitiesReentrant(
                keywordsNode->doc, (const xmlChar *) content->getCString()));

        content = getInfoString(info.getDict(), "Author");
        xmlNodeSetContent(authorNode, (const xmlChar *) xmlEncodeEntitiesReentrant(
                authorNode->doc, (const xmlChar *) content->getCString()));

        content = getInfoString(info.getDict(), "Creator");
        xmlNodeSetContent(creatorNode, (const xmlChar *) xmlEncodeEntitiesReentrant(
                creatorNode->doc, (const xmlChar *) content->getCString()));

        content = getInfoString(info.getDict(), "Producer");
        xmlNodeSetContent(producerNode, (const xmlChar *) xmlEncodeEntitiesReentrant(
                producerNode->doc, (const xmlChar *) content->getCString()));

        content = getInfoDate(info.getDict(), "CreationDate");
        xmlNodeSetContent(creationDateNode, (const xmlChar *) xmlEncodeEntitiesReentrant(
                creationDateNode->doc, (const xmlChar *) content->getCString()));

        content = getInfoDate(info.getDict(), "ModDate");
        xmlNodeSetContent(modDateNode, (const xmlChar *) xmlEncodeEntitiesReentrant(
                modDateNode->doc, (const xmlChar *) content->getCString()));
    }
    info.free();
}

void XmlAltoOutputDev::closeMetadataInfoDoc(GString *shortFileName) {
    GString *metadataFilename = new GString(shortFileName);
    metadataFilename->append("_");
    metadataFilename->append(NAME_METADATA);
    metadataFilename->append(EXTENSION_XML);
    xmlSaveFile(metadataFilename->getCString(), docMetadata);
    xmlFreeDoc(docMetadata);

}

void XmlAltoOutputDev::addStyles() {

    xmlNodePtr nodeSourceImageInfo = findNodeByName(docroot, (const xmlChar *) TAG_STYLES);
    for (int j = 0; j < getText()->fontStyles.size(); ++j) {
        xmlNodePtr textStyleNode = xmlNewNode(NULL, (const xmlChar *) TAG_TEXTSTYLE);

        char *tmp;
        tmp = (char *) malloc(50 * sizeof(char));

        TextFontStyleInfo *fontStyleInfo = getText()->fontStyles[j];
        textStyleNode->type = XML_ELEMENT_NODE;

        xmlAddChild(nodeSourceImageInfo, textStyleNode);

        sprintf(tmp, "font%d", fontStyleInfo->getId());
        xmlNewProp(textStyleNode, (const xmlChar *) ATTR_ID, (const xmlChar *) tmp);

        sprintf(tmp, "%s", fontStyleInfo->getFontNameCS()->getCString());
        xmlNewProp(textStyleNode, (const xmlChar *) ATTR_FONTFAMILY, (const xmlChar *) tmp);
        delete fontStyleInfo->getFontNameCS();

        snprintf(tmp, sizeof(tmp), "%.3f", fontStyleInfo->getFontSize());
        xmlNewProp(textStyleNode, (const xmlChar *) ATTR_FONTSIZE, (const xmlChar *) tmp);

        //
        sprintf(tmp, "%s", fontStyleInfo->getFontType() ? "serif" : "sans-serif");
        xmlNewProp(textStyleNode, (const xmlChar *) ATTR_FONTTYPE, (const xmlChar *) tmp);

        sprintf(tmp, "%s", fontStyleInfo->getFontWidth() ? "fixed" : "proportional");
        xmlNewProp(textStyleNode, (const xmlChar *) ATTR_FONTWIDTH, (const xmlChar *) tmp);

        sprintf(tmp, "%s", fontStyleInfo->getFontColor()->getCString());
        xmlNewProp(textStyleNode, (const xmlChar *) ATTR_FONTCOLOR, (const xmlChar *) (tmp+1));

        delete fontStyleInfo->getFontColor();


        GString *fontStyle = new GString("");
        if (fontStyleInfo->isBold())
            fontStyle->append("bold");

        if (fontStyleInfo->isItalic()) {
            if (fontStyle->getLength() > 0)
                fontStyle->append(" italics");
            else fontStyle->append("italics");
        }

        if (fontStyleInfo->isSubscript()) {
            if (fontStyle->getLength() > 0)
                fontStyle->append(" subscript");
            else fontStyle->append("subscript");
        }

        if (fontStyleInfo->isSuperscript()) {
            // PL: font style can't be subscript and superscript at the same time
            if (fontStyle->getLength() > 0)
                fontStyle->append(" superscript");
            else fontStyle->append("superscript");
        }

        sprintf(tmp, "%s", fontStyle->getCString());
        if ( strcmp(tmp, "") )
            xmlNewProp(textStyleNode, (const xmlChar *) ATTR_FONTSTYLE, (const xmlChar *) tmp);

        delete fontStyle;

        free(tmp);
    }
}


xmlNodePtr XmlAltoOutputDev::findNodeByName(xmlNodePtr rootnode, const xmlChar *nodename) {
    xmlNodePtr node = rootnode;
    if (node == NULL) {
        printf("Document is empty!");
        return NULL;
    }

    while (node != NULL) {

        if (!xmlStrcmp(node->name, nodename)) {
            return node;
        } else if (node->children != NULL) {
            node = node->children;
            xmlNodePtr intNode = findNodeByName(node, nodename);
            if (intNode != NULL) {
                return intNode;
            }
        }
        node = node->next;
    }
    return NULL;
}


GString *XmlAltoOutputDev::getInfoString(Dict *infoDict, const char *key) {
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
                    (s1->getChar(i + 1) & 0xff);
                i += 2;
            } else {
                u = pdfDocEncoding[s1->getChar(i) & 0xff];
                ++i;
            }
            n = uMap->mapUnicode(u, buf, sizeof(buf));
            //fwrite(buf, 1, n, stdout);
            s->append(buf, n);
        }
        //fputc('\n', stdout);
    }
    obj.free();
    return s;
}

GString *XmlAltoOutputDev::getInfoDate(Dict *infoDict, const char *key) {
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
                case 1:
                    mon = 1;
                case 2:
                    day = 1;
                case 3:
                    hour = 0;
                case 4:
                    min = 0;
                case 5:
                    sec = 0;
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
            if (mktime(&tmStruct) != (time_t) -1 &&
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

GString *XmlAltoOutputDev::toUnicode(GString *s, UnicodeMap *uMap) {

    GString *news;
    Unicode *uString;
    int uLen;
    int j;

    if ((s->getChar(0) & 0xff) == 0xfe &&
        (s->getChar(1) & 0xff) == 0xff) {
        uLen = (s->getLength() - 2) / 2;
        uString = (Unicode *) gmallocn(uLen, sizeof(Unicode));
        for (j = 0; j < uLen; ++j) {
            uString[j] = ((s->getChar(2 + 2 * j) & 0xff) << 8) |
                         (s->getChar(3 + 2 * j) & 0xff);
        }
    } else {
        uLen = s->getLength();
        uString = (Unicode *) gmallocn(uLen, sizeof(Unicode));
        for (j = 0; j < uLen; ++j) {
            uString[j] = pdfDocEncoding[s->getChar(j) & 0xff];
        }
    }

    news = new GString();
    dumpFragment(uString, uLen, uMap, news);

    return news;
}

void XmlAltoOutputDev::startPage(int pageNum, GfxState *state) {
    //curstate = (double*)malloc(6*sizeof(double));
    for (int i = 0; i < 6; ++i) {
        curstate[i] = state->getCTM()[i];
    }
    if (parameters->getCutAllPages() == 1) {
        text->startPage(pageNum, state, gFalse);

    }
    if (parameters->getCutAllPages() == 0) {
        text->startPage(pageNum, state, gTrue);
    }
}

// Map StrokeAdjustMode (from GlobalParams) to SplashStrokeAdjustMode
// (for Splash).
static SplashStrokeAdjustMode mapStrokeAdjustMode[3] = {
        splashStrokeAdjustOff,
        splashStrokeAdjustNormal,
        splashStrokeAdjustCAD
};

void XmlAltoOutputDev::endPage() {
    text->configuration();
    if (parameters->getDisplayText()) {
//        if (readingOrder) {
//            text->dumpInReadingOrder(noLineNumbers, fullFontName);
//        } else
        text->dump(noLineNumbers, fullFontName, lineNumberStatus);
        appendLineNumberStatus(text->getLineNumber());
    }

    text->endPage(dataDir);
}

vector<bool> XmlAltoOutputDev::getLineNumberStatus() {
    return lineNumberStatus;
}
    
void XmlAltoOutputDev::appendLineNumberStatus(bool hasLineNumber) {
    lineNumberStatus.push_back(hasLineNumber);
}

void XmlAltoOutputDev::updateFont(GfxState *state) {
    needFontUpdate = gTrue;
    text->updateFont(state);
}

void XmlAltoOutputDev::startDoc(XRef *xrefA) {
    int i;

    GBool allowAntialias = gTrue;
    SplashColorMode colorMode = splashModeRGB8;

    xref = xrefA;
    if (fontEngine) {
        delete fontEngine;
    }
    fontEngine = new SplashFontEngine(
#if HAVE_FREETYPE_H
            globalParams->getEnableFreeType(),
            globalParams->getDisableFreeTypeHinting()
            ? splashFTNoHinting : 0,
#endif
            allowAntialias &&
            globalParams->getAntialias() &&
            colorMode != splashModeMono1);
    for (i = 0; i < nT3Fonts; ++i) {
        delete t3FontCache[i];
    }
    nT3Fonts = 0;
}

class SplashOutFontFileID : public SplashFontFileID {

public:
    SplashOutFontFileID(Ref *rA) {
        r = *rA;
        substIdx = -1;
        oblique = 0;
    }

    ~SplashOutFontFileID() {}

    GBool matches(SplashFontFileID *id) {
        return ((SplashOutFontFileID *) id)->r.num == r.num &&
               ((SplashOutFontFileID *) id)->r.gen == r.gen;
    }

    void setOblique(double obliqueA) { oblique = obliqueA; }

    double getOblique() { return oblique; }

    void setSubstIdx(int substIdxA) { substIdx = substIdxA; }

    int getSubstIdx() { return substIdx; }

private:
    Ref r;
    double oblique;
    int substIdx;
};

SplashFont *XmlAltoOutputDev::getSplashFont(GfxState *state, SplashCoord *matrix) {
    SplashFont *font;
    GfxFont *gfxFont;
    GfxFontLoc *fontLoc;
    GfxFontType fontType;
    SplashOutFontFileID *id;
    SplashFontFile *fontFile;
    int fontNum;
    FoFiTrueType *ff;
    Ref embRef;
    Object refObj, strObj;
#if LOAD_FONTS_FROM_MEM
    GString *fontBuf;
  FILE *extFontFile;
#else
    GString *tmpFileName, *fileName;
    FILE *tmpFile;
#endif
    char blk[4096];
    int *codeToGID;
    CharCodeToUnicode *ctu;
    double *textMat;
    double m11, m12, m21, m22, fontSize, oblique;
    double fsx, fsy, w, fontScaleMin, fontScaleAvg, fontScale;
    Gushort ww;
    SplashCoord mat[4];
    char *name;
    Unicode uBuf[8];
    int substIdx, n, code, cmap, cmapPlatform, cmapEncoding, i;

    //needFontUpdate = gFalse;
    font = NULL;
#if LOAD_FONTS_FROM_MEM
    fontBuf = NULL;
#else
    tmpFileName = NULL;
    fileName = NULL;
#endif
    substIdx = -1;

    if (!(gfxFont = state->getFont())) {
        goto err1;
    }
    fontType = gfxFont->getType();
    if (fontType == fontType3) {
        goto err1;
    }

    // sanity-check the font size - skip anything larger than 20 inches
    // (this avoids problems allocating memory for the font cache)
    state->textTransformDelta(state->getFontSize(), state->getFontSize(),
                              &fsx, &fsy);
    state->transformDelta(fsx, fsy, &fsx, &fsy);
    if (fabs(fsx) > 20 * state->getHDPI() ||
        fabs(fsy) > 20 * state->getVDPI()) {
        goto err1;
    }

    // check the font file cache
    id = new SplashOutFontFileID(gfxFont->getID());
    if ((fontFile = fontEngine->getFontFile(id))) {
        delete id;

    } else {

        fontNum = 0;

        if (!(fontLoc = gfxFont->locateFont(xref, gFalse))) {
            error(errSyntaxError, -1, "Couldn't find a font for '{0:s}'",
                  gfxFont->getName() ? gfxFont->getName()->getCString()
                                     : "(unnamed)");
            goto err2;
        }

        // embedded font
        if (fontLoc->locType == gfxFontLocEmbedded) {
            gfxFont->getEmbeddedFontID(&embRef);
#if LOAD_FONTS_FROM_MEM
            fontBuf = new GString();
      refObj.initRef(embRef.num, embRef.gen);
      refObj.fetch(xref, &strObj);
      refObj.free();
      if (!strObj.isStream()) {
    error(errSyntaxError, -1, "Embedded font object is wrong type");
    strObj.free();
    delete fontLoc;
    goto err2;
      }
      strObj.streamReset();
      while ((n = strObj.streamGetBlock(blk, sizeof(blk))) > 0) {
    fontBuf->append(blk, n);
      }
      strObj.streamClose();
      strObj.free();
#else
            if (!openTempFile(&tmpFileName, &tmpFile, "wb", NULL)) {
                error(errIO, -1, "Couldn't create temporary font file");
                delete fontLoc;
                goto err2;
            }
            refObj.initRef(embRef.num, embRef.gen);
            refObj.fetch(xref, &strObj);
            refObj.free();
            if (!strObj.isStream()) {
                error(errSyntaxError, -1, "Embedded font object is wrong type");
                strObj.free();
                fclose(tmpFile);
                delete fontLoc;
                goto err2;
            }
            strObj.streamReset();
            while ((n = strObj.streamGetBlock(blk, sizeof(blk))) > 0) {
                fwrite(blk, 1, n, tmpFile);
            }
            strObj.streamClose();
            strObj.free();
            fclose(tmpFile);
            fileName = tmpFileName;
#endif

            // external font
        } else { // gfxFontLocExternal
#if LOAD_FONTS_FROM_MEM
            if (!(extFontFile = fopen(fontLoc->path->getCString(), "rb"))) {
    error(errSyntaxError, -1, "Couldn't open external font file '{0:t}'",
          fontLoc->path);
    delete fontLoc;
    goto err2;
      }
      fontBuf = new GString();
      while ((n = fread(blk, 1, sizeof(blk), extFontFile)) > 0) {
    fontBuf->append(blk, n);
      }
      fclose(extFontFile);
#else
            fileName = fontLoc->path;
#endif
            fontNum = fontLoc->fontNum;
            if (fontLoc->substIdx >= 0) {
                id->setSubstIdx(fontLoc->substIdx);
            }
            if (fontLoc->oblique != 0) {
                id->setOblique(fontLoc->oblique);
            }
        }

        // load the font file
        switch (fontLoc->fontType) {
            case fontType1:
                if (!(fontFile = fontEngine->loadType1Font(
                        id,
#if LOAD_FONTS_FROM_MEM
                        fontBuf,
#else
                        fileName->getCString(),
                        fileName == tmpFileName,
#endif
                        (const char **) ((Gfx8BitFont *) gfxFont)->getEncoding()))) {
                    error(errSyntaxError, -1, "Couldn't create a font for '{0:s}'",
                          gfxFont->getName() ? gfxFont->getName()->getCString()
                                             : "(unnamed)");
                    delete fontLoc;
                    goto err2;
                }
                break;
            case fontType1C:
                if (!(fontFile = fontEngine->loadType1CFont(
                        id,
#if LOAD_FONTS_FROM_MEM
                        fontBuf,
#else
                        fileName->getCString(),
                        fileName == tmpFileName,
#endif
                        (const char **) ((Gfx8BitFont *) gfxFont)->getEncoding()))) {
                    error(errSyntaxError, -1, "Couldn't create a font for '{0:s}'",
                          gfxFont->getName() ? gfxFont->getName()->getCString()
                                             : "(unnamed)");
                    delete fontLoc;
                    goto err2;
                }
                break;
            case fontType1COT:
                if (!(fontFile = fontEngine->loadOpenTypeT1CFont(
                        id,
#if LOAD_FONTS_FROM_MEM
                        fontBuf,
#else
                        fileName->getCString(),
                        fileName == tmpFileName,
#endif
                        (const char **) ((Gfx8BitFont *) gfxFont)->getEncoding()))) {
                    error(errSyntaxError, -1, "Couldn't create a font for '{0:s}'",
                          gfxFont->getName() ? gfxFont->getName()->getCString()
                                             : "(unnamed)");
                    delete fontLoc;
                    goto err2;
                }
                break;
            case fontTrueType:
            case fontTrueTypeOT:
#if LOAD_FONTS_FROM_MEM
                if ((ff = FoFiTrueType::make(fontBuf->getCString(), fontBuf->getLength(),
                   fontNum))) {
#else
                if ((ff = FoFiTrueType::load(fileName->getCString(), fontNum))) {
#endif
                    codeToGID = ((Gfx8BitFont *) gfxFont)->getCodeToGIDMap(ff);
                    n = 256;
                    delete ff;
                    // if we're substituting for a non-TrueType font, we need to mark
                    // all notdef codes as "do not draw" (rather than drawing TrueType
                    // notdef glyphs)
                    if (gfxFont->getType() != fontTrueType &&
                        gfxFont->getType() != fontTrueTypeOT) {
                        for (i = 0; i < 256; ++i) {
                            if (codeToGID[i] == 0) {
                                codeToGID[i] = -1;
                            }
                        }
                    }
                } else {
                    codeToGID = NULL;
                    n = 0;
                }
                if (!(fontFile = fontEngine->loadTrueTypeFont(
                        id,
#if LOAD_FONTS_FROM_MEM
                        fontBuf,
#else
                        fileName->getCString(),
                        fileName == tmpFileName,
#endif
                        fontNum, codeToGID, n,
                        gfxFont->getEmbeddedFontName()
                        ? gfxFont->getEmbeddedFontName()->getCString()
                        : (char *) NULL))) {
                    error(errSyntaxError, -1, "Couldn't create a font for '{0:s}'",
                          gfxFont->getName() ? gfxFont->getName()->getCString()
                                             : "(unnamed)");
                    delete fontLoc;
                    goto err2;
                }
                break;
            case fontCIDType0:
            case fontCIDType0C:
                if (((GfxCIDFont *) gfxFont)->getCIDToGID()) {
                    n = ((GfxCIDFont *) gfxFont)->getCIDToGIDLen();
                    codeToGID = (int *) gmallocn(n, sizeof(int));
                    memcpy(codeToGID, ((GfxCIDFont *) gfxFont)->getCIDToGID(),
                           n * sizeof(int));
                } else {
                    codeToGID = NULL;
                    n = 0;
                }
                if (!(fontFile = fontEngine->loadCIDFont(
                        id,
#if LOAD_FONTS_FROM_MEM
                        fontBuf,
#else
                        fileName->getCString(),
                        fileName == tmpFileName,
#endif
                        codeToGID, n))) {

                    error(errSyntaxError, -1, "Couldn't create a font for '{0:s}'",
                          gfxFont->getName() ? gfxFont->getName()->getCString()
                                             : "(unnamed)");
                    delete fontLoc;
                    goto err2;
                }
                break;
            case fontCIDType0COT:
                codeToGID = NULL;
                n = 0;
                if (fontLoc->locType == gfxFontLocEmbedded) {
                    if (((GfxCIDFont *) gfxFont)->getCIDToGID()) {
                        n = ((GfxCIDFont *) gfxFont)->getCIDToGIDLen();
                        codeToGID = (int *) gmallocn(n, sizeof(int));
                        memcpy(codeToGID, ((GfxCIDFont *) gfxFont)->getCIDToGID(),
                               n * sizeof(int));
                    }
                } else if (globalParams->getMapExtTrueTypeFontsViaUnicode()) {
                    // create a CID-to-GID mapping, via Unicode
                    if ((ctu = ((GfxCIDFont *) gfxFont)->getToUnicode())) {
#if LOAD_FONTS_FROM_MEM
                        if ((ff = FoFiTrueType::make(fontBuf->getCString(),
                       fontBuf->getLength(), fontNum))) {
#else
                        if ((ff = FoFiTrueType::load(fileName->getCString(), fontNum))) {
#endif
                            // look for a Unicode cmap
                            for (cmap = 0; cmap < ff->getNumCmaps(); ++cmap) {
                                cmapPlatform = ff->getCmapPlatform(cmap);
                                cmapEncoding = ff->getCmapEncoding(cmap);
                                if ((cmapPlatform == 3 && cmapEncoding == 1) ||
                                    (cmapPlatform == 0 && cmapEncoding <= 4)) {
                                    break;
                                }
                            }
                            if (cmap < ff->getNumCmaps()) {
                                // map CID -> Unicode -> GID
                                if (ctu->isIdentity()) {
                                    n = 65536;
                                } else {
                                    n = ctu->getLength();
                                }
                                codeToGID = (int *) gmallocn(n, sizeof(int));
                                for (code = 0; code < n; ++code) {
                                    if (ctu->mapToUnicode(code, uBuf, 8) > 0) {
                                        codeToGID[code] = ff->mapCodeToGID(cmap, uBuf[0]);
                                    } else {
                                        codeToGID[code] = -1;
                                    }
                                }
                            }
                            delete ff;
                        }
                        ctu->decRefCnt();
                    } else {
                        error(errSyntaxError, -1,
                              "Couldn't find a mapping to Unicode for font '{0:s}'",
                              gfxFont->getName() ? gfxFont->getName()->getCString()
                                                 : "(unnamed)");
                    }
                }
                if (!(fontFile = fontEngine->loadOpenTypeCFFFont(
                        id,
#if LOAD_FONTS_FROM_MEM
                        fontBuf,
#else
                        fileName->getCString(),
                        fileName == tmpFileName,
#endif
                        codeToGID, n))) {
                    error(errSyntaxError, -1, "Couldn't create a font for '{0:s}'",
                          gfxFont->getName() ? gfxFont->getName()->getCString()
                                             : "(unnamed)");
                    delete fontLoc;
                    goto err2;
                }
                break;
            case fontCIDType2:
            case fontCIDType2OT:
                codeToGID = NULL;
                n = 0;
                if (fontLoc->locType == gfxFontLocEmbedded) {
                    if (((GfxCIDFont *) gfxFont)->getCIDToGID()) {
                        n = ((GfxCIDFont *) gfxFont)->getCIDToGIDLen();
                        codeToGID = (int *) gmallocn(n, sizeof(int));
                        memcpy(codeToGID, ((GfxCIDFont *) gfxFont)->getCIDToGID(),
                               n * sizeof(int));
                    }
                } else if (globalParams->getMapExtTrueTypeFontsViaUnicode()) {
                    // create a CID-to-GID mapping, via Unicode
                    if ((ctu = ((GfxCIDFont *) gfxFont)->getToUnicode())) {
#if LOAD_FONTS_FROM_MEM
                        if ((ff = FoFiTrueType::make(fontBuf->getCString(),
                       fontBuf->getLength(), fontNum))) {
#else
                        if ((ff = FoFiTrueType::load(fileName->getCString(), fontNum))) {
#endif
                            // look for a Unicode cmap
                            for (cmap = 0; cmap < ff->getNumCmaps(); ++cmap) {
                                cmapPlatform = ff->getCmapPlatform(cmap);
                                cmapEncoding = ff->getCmapEncoding(cmap);
                                if ((cmapPlatform == 3 && cmapEncoding == 1) ||
                                    (cmapPlatform == 0 && cmapEncoding <= 4)) {
                                    break;
                                }
                            }
                            if (cmap < ff->getNumCmaps()) {
                                // map CID -> Unicode -> GID
                                if (ctu->isIdentity()) {
                                    n = 65536;
                                } else {
                                    n = ctu->getLength();
                                }
                                codeToGID = (int *) gmallocn(n, sizeof(int));
                                for (code = 0; code < n; ++code) {
                                    if (ctu->mapToUnicode(code, uBuf, 8) > 0) {
                                        codeToGID[code] = ff->mapCodeToGID(cmap, uBuf[0]);
                                    } else {
                                        codeToGID[code] = -1;
                                    }
                                }
                            }
                            delete ff;
                        }
                        ctu->decRefCnt();
                    } else {
                        error(errSyntaxError, -1,
                              "Couldn't find a mapping to Unicode for font '{0:s}'",
                              gfxFont->getName() ? gfxFont->getName()->getCString()
                                                 : "(unnamed)");
                    }
                }
                if (!(fontFile = fontEngine->loadTrueTypeFont(
                        id,
#if LOAD_FONTS_FROM_MEM
                        fontBuf,
#else
                        fileName->getCString(),
                        fileName == tmpFileName,
#endif
                        fontNum, codeToGID, n,
                        gfxFont->getEmbeddedFontName()
                        ? gfxFont->getEmbeddedFontName()->getCString()
                        : (char *) NULL))) {
                    error(errSyntaxError, -1, "Couldn't create a font for '{0:s}'",
                          gfxFont->getName() ? gfxFont->getName()->getCString()
                                             : "(unnamed)");
                    delete fontLoc;
                    goto err2;
                }
                break;
            default:
                // this shouldn't happen
                goto err2;
        }

        delete fontLoc;
    }

    // get the font matrix
    textMat = state->getTextMat();
    fontSize = state->getFontSize();
    oblique = ((SplashOutFontFileID *) fontFile->getID())->getOblique();
    m11 = state->getHorizScaling() * textMat[0];
    m12 = state->getHorizScaling() * textMat[1];
    m21 = oblique * m11 + textMat[2];
    m22 = oblique * m12 + textMat[3];
    m11 *= fontSize;
    m12 *= fontSize;
    m21 *= fontSize;
    m22 *= fontSize;

    // for substituted fonts: adjust the font matrix -- compare the
    // widths of letters and digits (A-Z, a-z, 0-9) in the original font
    // and the substituted font
    substIdx = ((SplashOutFontFileID *) fontFile->getID())->getSubstIdx();
    if (substIdx >= 0 && substIdx < 12) {
        fontScaleMin = 1;
        fontScaleAvg = 0;
        n = 0;
        for (code = 0; code < 256; ++code) {
            if ((name = ((Gfx8BitFont *) gfxFont)->getCharName(code)) &&
                name[0] && !name[1] &&
                ((name[0] >= 'A' && name[0] <= 'Z') ||
                 (name[0] >= 'a' && name[0] <= 'z') ||
                 (name[0] >= '0' && name[0] <= '9'))) {
                w = ((Gfx8BitFont *) gfxFont)->getWidth(code);
                builtinFontSubst[substIdx]->widths->getWidth(name, &ww);
                if (w > 0.01 && ww > 10) {
                    w /= ww * 0.001;
                    if (w < fontScaleMin) {
                        fontScaleMin = w;
                    }
                    fontScaleAvg += w;
                    ++n;
                }
            }
        }
        // if real font is narrower than substituted font, reduce the font
        // size accordingly -- this currently uses a scale factor halfway
        // between the minimum and average computed scale factors, which
        // is a bit of a kludge, but seems to produce mostly decent
        // results
        if (n) {
            fontScaleAvg /= n;
            if (fontScaleAvg < 1) {
                fontScale = 0.5 * (fontScaleMin + fontScaleAvg);
                m11 *= fontScale;
                m12 *= fontScale;
            }
        }
    }

    // create the scaled font
    mat[0] = m11;
    mat[1] = m12;
    mat[2] = m21;
    mat[3] = m22;
    font = fontEngine->getFont(fontFile, mat, matrix);

#if !LOAD_FONTS_FROM_MEM
    if (tmpFileName) {
        delete tmpFileName;
    }
#endif
    return font;

    err2:
    delete id;
    err1:
#if LOAD_FONTS_FROM_MEM
    if (fontBuf) {
    delete fontBuf;
  }
#else
    if (tmpFileName) {
        unlink(tmpFileName->getCString());
        delete tmpFileName;
    }
#endif
    return font;
}

void XmlAltoOutputDev::drawChar(GfxState *state, double x, double y, double dx,
                                double dy, double originX, double originY, CharCode c, int nBytes,
                                Unicode *u, int uLen) {

    GBool isNonUnicodeGlyph = gFalse;

    SplashFont *splashFont = NULL;

    if (parameters->getOcr() == gTrue) {
        SplashCoord mat[6];
//        mat[0] = (SplashCoord) (curstate[0]);
//        mat[1] = (SplashCoord) (curstate[1]);
//        mat[2] = (SplashCoord) (curstate[2]);
//        mat[3] = (SplashCoord) (curstate[3]);
//        mat[4] = (SplashCoord) (curstate[4]);
//        mat[5] = (SplashCoord) (curstate[5]);
        //splashFont = getSplashFont(state, mat);
        if ((uLen == 0 ||
             ((u[0] == (Unicode) 0 || u[0] < (Unicode) 32) && uLen == 1) ||
             (uLen > 1 && (globalParams->getTextEncodingName()->cmp(ENCODING_UTF8)==0)&& !isUTF8(u, uLen)))) {
        //when len is gt 1 check if sequence is valid, if not replace by placeholder
            //&& globalParams->getApplyOCR())
            // as a first iteration for dictionnaries, placing a placeholder, which means creating a map based on the font-code mapping to unicode from : https://unicode.org/charts/PDF/U2B00.pdf
            GString *fontName = new GString();
            if (state->getFont()->getName()) { //AA : Here if fontName is NULL is problematic
                fontName = state->getFont()->getName()->copy();
                fontName = fontName->lowerCase();
            }
            GString *fontName_charcode = fontName->append(
                    to_string(c).c_str());// for performance and simplicity only appending is done
            if (globalParams->getPrintCommands()) {
                printf("ToBeOCRISEChar: x=%.2f y=%.2f c=%3d=0x%02x='%c' u=%3d fontName=%s \n",
                       (double) x, (double) y, c, c, c, u[0], fontName->getCString());
            }
            // do map every char to a unicode, depending on charcode and font name
            Unicode mapped_unicode = unicode_map->lookupInt(fontName_charcode);
            if (!mapped_unicode) {
                mapped_unicode = placeholders[0];//no special need for random
                if (placeholders.size() > 1) {
                    placeholders.erase(placeholders.begin());
                }
                unicode_map->add(fontName_charcode, mapped_unicode);
            }
            u[0] = mapped_unicode;
            uLen = 1;
            isNonUnicodeGlyph = gTrue;
        }
    } else if(uLen > 1 && (globalParams->getTextEncodingName()->cmp(ENCODING_UTF8)==0)&& !isUTF8(u, uLen))
        return;

    text->addChar(state, x, y, dx, dy, c, nBytes, u, uLen, splashFont, isNonUnicodeGlyph);
}

void XmlAltoOutputDev::updateCTM(GfxState *state, double m11, double m12,
                                 double m21, double m22,
                                 double m31, double m32) {
    for (int i = 0; i < 6; ++i) {
        curstate[i] = state->getCTM()[i];
    }
}

//------------------------------------------------------------------------
// T3FontCache
//------------------------------------------------------------------------
T3FontCache::T3FontCache(Ref *fontIDA, double m11A, double m12A,
                         double m21A, double m22A,
                         int glyphXA, int glyphYA, int glyphWA, int glyphHA,
                         GBool validBBoxA, GBool aa) {
    int i;

    fontID = *fontIDA;
    m11 = m11A;
    m12 = m12A;
    m21 = m21A;
    m22 = m22A;
    glyphX = glyphXA;
    glyphY = glyphYA;
    glyphW = glyphWA;
    glyphH = glyphHA;
    validBBox = validBBoxA;
    // sanity check for excessively large glyphs (which most likely
    // indicate an incorrect BBox)
    i = glyphW * glyphH;
    if (i > 100000 || glyphW > INT_MAX / glyphH || glyphW <= 0 || glyphH <= 0) {
        glyphW = glyphH = 100;
        validBBox = gFalse;
    }
    if (aa) {
        glyphSize = glyphW * glyphH;
    } else {
        glyphSize = ((glyphW + 7) >> 3) * glyphH;
    }
    cacheAssoc = type3FontCacheAssoc;
    for (cacheSets = type3FontCacheMaxSets;
         cacheSets > 1 &&
         cacheSets * cacheAssoc * glyphSize > type3FontCacheSize;
         cacheSets >>= 1);
    cacheData = (Guchar *) gmallocn(cacheSets * cacheAssoc, glyphSize);
    cacheTags = (T3FontCacheTag *) gmallocn(cacheSets * cacheAssoc,
                                            sizeof(T3FontCacheTag));
    for (i = 0; i < cacheSets * cacheAssoc; ++i) {
        cacheTags[i].mru = i & (cacheAssoc - 1);
    }
}

T3FontCache::~T3FontCache() {
    gfree(cacheData);
    gfree(cacheTags);
}

struct T3GlyphStack {
    Gushort code;            // character code

    GBool haveDx;            // set after seeing a d0/d1 operator
    GBool doNotCache;        // set if we see a gsave/grestore before
    //   the d0/d1

    //----- cache info
    T3FontCache *cache;        // font cache for the current font
    T3FontCacheTag *cacheTag;    // pointer to cache tag for the glyph
    Guchar *cacheData;        // pointer to cache data for the glyph

    //----- saved state
    SplashBitmap *origBitmap;
    Splash *origSplash;
    double origCTM4, origCTM5;

    T3GlyphStack *next;        // next object on stack
};

void XmlAltoOutputDev::setupScreenParams(double hDPI, double vDPI) {
    screenParams.size = globalParams->getScreenSize();
    screenParams.dotRadius = globalParams->getScreenDotRadius();
    screenParams.gamma = (SplashCoord) globalParams->getScreenGamma();
    screenParams.blackThreshold =
            (SplashCoord) globalParams->getScreenBlackThreshold();
    screenParams.whiteThreshold =
            (SplashCoord) globalParams->getScreenWhiteThreshold();
    switch (globalParams->getScreenType()) {
        case screenDispersed:
            screenParams.type = splashScreenDispersed;
            if (screenParams.size < 0) {
                screenParams.size = 4;
            }
            break;
        case screenClustered:
            screenParams.type = splashScreenClustered;
            if (screenParams.size < 0) {
                screenParams.size = 10;
            }
            break;
        case screenStochasticClustered:
            screenParams.type = splashScreenStochasticClustered;
            if (screenParams.size < 0) {
                screenParams.size = 64;
            }
            if (screenParams.dotRadius < 0) {
                screenParams.dotRadius = 2;
            }
            break;
        case screenUnset:
        default:
            // use clustered dithering for resolution >= 300 dpi
            // (compare to 299.9 to avoid floating point issues)
            if (hDPI > 299.9 && vDPI > 299.9) {
                screenParams.type = splashScreenStochasticClustered;
                if (screenParams.size < 0) {
                    screenParams.size = 64;
                }
                if (screenParams.dotRadius < 0) {
                    screenParams.dotRadius = 2;
                }
            } else {
                screenParams.type = splashScreenDispersed;
                if (screenParams.size < 0) {
                    screenParams.size = 4;
                }
            }
    }
}

void XmlAltoOutputDev::stroke(GfxState *state) {
    GString *attr = new GString();
    char tmp[100];
    GfxRGB rgb;

    // The stroke attribute : the stroke color value
    state->getStrokeRGB(&rgb);
    GString *hexColor = colortoString(rgb);
    sprintf(tmp, "stroke: %s;", hexColor->getCString());
    attr->append(tmp);
    delete hexColor;

    // The stroke-opacity attribute
    double fo = state->getStrokeOpacity();
    if (fo != 1) {
        snprintf(tmp, sizeof(tmp), "stroke-opacity: %g;", fo);
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
        for (i = 0; i < length; i++) {
            snprintf(tmp, sizeof(tmp), ATTR_NUMFORMAT, state->transformWidth(dash[i]) == 0 ? 1
                                                                   : state->transformWidth(dash[i]));
            attr->append(tmp);
            sprintf(tmp, "%s", (i == length - 1) ? "" : ", ");
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
    snprintf(tmp, sizeof(tmp), "stroke-width: %g;", lineWidth2);
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
        snprintf(tmp, sizeof(tmp), "stroke-miterlimit: %g;", miter);
    }
    attr->append(tmp);

    doPath(state->getPath(), state, attr);
}

void XmlAltoOutputDev::fill(GfxState *state) {
    GString *attr = new GString();
    char tmp[100];
    GfxRGB rgb;

    // The fill attribute which give color value
    state->getFillRGB(&rgb);
    GString *hexColor = colortoString(rgb);
    sprintf(tmp, "fill: %s;", hexColor->getCString());
    attr->append(tmp);
    delete hexColor;

    // The fill-opacity attribute
    double fo = state->getFillOpacity();
    snprintf(tmp, sizeof(tmp), "fill-opacity: %g;", fo);
    attr->append(tmp);

    doPath(state->getPath(), state, attr);
}

void XmlAltoOutputDev::eoFill(GfxState *state) {
    GString *attr = new GString();
    char tmp[100];
    GfxRGB rgb;

    // The fill attribute which give color value
    state->getFillRGB(&rgb);
    GString *hexColor = colortoString(rgb);
    sprintf(tmp, "fill: %s;", hexColor->getCString());
    attr->append(tmp);
    delete hexColor;

    // The fill-rule attribute with evenodd value
    attr->append("fill-rule: evenodd;");

    // The fill-opacity attribute
    double fo = state->getFillOpacity();
    snprintf(tmp, sizeof(tmp), "fill-opacity: %g;", fo);
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
    needFontUpdate = gTrue;
    for (int i = 0; i < 6; ++i) {
        curstate[i] = state->getCTM()[i];
    }
    text->restoreState(state);
}

// Return the hexadecimal value of the color of string
GString *XmlAltoOutputDev::colortoString(GfxRGB rgb) const {
    char *temp;
    temp = (char *) malloc(10 * sizeof(char));
    sprintf(temp, "#%02X%02X%02X",
            static_cast<int>(255 * colToDbl(rgb.r)),
            static_cast<int>(255 * colToDbl(rgb.g)),
            static_cast<int>(255 * colToDbl(rgb.b)));

    GString *tmp = new GString(temp);

    free(temp);

    return tmp;
}

GString *XmlAltoOutputDev::convtoX(unsigned int xcol) const {
    GString *xret = new GString();
    char tmp;
    unsigned int k;
    k = (xcol / 16);
    if ((k >= 0) && (k < 10))
        tmp = (char) ('0' + k);
    else
        tmp = (char) ('a' + k - 10);
    xret->append(tmp);
    k = (xcol % 16);
    if ((k >= 0) && (k < 10))
        tmp = (char) ('0' + k);
    else
        tmp = (char) ('a' + k - 10);
    xret->append(tmp);
    return xret;
}

void XmlAltoOutputDev::drawSoftMaskedImage(GfxState *state, Object *ref,
                                           Stream *str,
                                           int width, int height,
                                           GfxImageColorMap *colorMap,
                                           Stream *maskStr,
                                           int maskWidth, int maskHeight,
                                           GfxImageColorMap *maskColorMap,
                                           double *matte, GBool interpolate) {
    drawImage(state, ref, str, width, height, colorMap,
              NULL, gFalse, interpolate);
    drawImage(state, ref, maskStr, maskWidth, maskHeight, maskColorMap,
              NULL, gFalse, interpolate);
}


void XmlAltoOutputDev::drawImage(GfxState *state, Object *ref, Stream *str,
                                 int width, int height, GfxImageColorMap *colorMap, int *maskColors,
                                 GBool inlineImg,
                                 GBool interpolate) {

    const char *ext;

    int index = -1;
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
        if (x1 > x2) {
            flip |= 1;
            flip_x = true;
            temp = x1;
            x1 = x2;
            x2 = temp;
        }

        if (y1 > y2) {
            flip |= 2;
            flip_y = true;
            temp = y1;
            y1 = y2;
            y2 = temp;
        }
        int reference = -1;
        if ((ref != NULL) && (ref->isRef())) {
            reference = ref->getRefNum();

            for (int i = 0; i < lPictureReferences->getLength(); i++) {
                PictureReference *pic_reference = (PictureReference *) lPictureReferences->get(i);
                if ((pic_reference->reference_number == reference)
                    && (pic_reference->picture_flip == flip)) {
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
        if (index == -1) {
            //HD : in order to avoid millions of small (pixel) images
            if (height > 8 && width > 8 && imageIndex < parameters->getFilesCountLimit()) {
                imageIndex += 1;
                // Save this in the references
                //			text->drawImage(state, ref, str, width, height, colorMap, maskColors,inlineImg, dumpJPEG, imageIndex);
                ext = text->drawImageOrMask(state, ref, str, width, height, colorMap, maskColors, inlineImg, false,
                                            imageIndex); // not a mask
                lPictureReferences->append(new PictureReference(reference, flip, imageIndex, ext));


            }

        } else {
            //HD : in order to avoid millions of small (pixel) images
            if (height > 8 && width > 8 && imageIndex < parameters->getFilesCountLimit()) {
                imageIndex += 1;
                //			text->drawImage(state, ref, str, width, height, colorMap, maskColors,inlineImg, dumpJPEG, imageIndex);
                ext = text->drawImageOrMask(state, ref, str, width, height, colorMap, maskColors, inlineImg, false,
                                            imageIndex); // not a mask
                lPictureReferences->append(new PictureReference(reference, flip, imageIndex, ext));

            }
        }
    }
}

void XmlAltoOutputDev::drawImageMask(GfxState *state, Object *ref, Stream *str,
                                     int width, int height, GBool invert, GBool inlineImg,
                                     GBool interpolate) {


    const char *ext;
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
    if (x1 > x2) {
        flip |= 1;
        flip_x = true;
        temp = x1;
        x1 = x2;
        x2 = temp;
    }

    if (y1 > y2) {
        flip |= 2;
        flip_y = true;
        temp = y1;
        y1 = y2;
        y2 = temp;
    }
    int reference = -1;
    if ((ref != NULL) && (ref->isRef())) {
        reference = ref->getRefNum();

        for (int i = 0; i < lPictureReferences->getLength(); i++) {
            PictureReference *pic_reference = (PictureReference *) lPictureReferences->get(i);

            if ((pic_reference->reference_number == reference)
                && (pic_reference->picture_flip == flip)) {
                // We already created a file for this picture
                index = pic_reference->picture_number;
//				printf("MASK IMAGE ALREADY SEEN flip=%d\t%d %d\n",flip,reference,index);
                break;
            }
        }
    }
    //new image
    if (index == -1) {
        if (parameters->getDisplayImage()) {
            //HD : in order to avoid millions of small (pixel) images
            if (height > 8 && width > 8 && imageIndex < parameters->getFilesCountLimit()) {
                imageIndex += 1;
                // Save this in the references
                ext = text->drawImageOrMask(state, ref, str, width, height, NULL, NULL, inlineImg, true,
                                            imageIndex); // mask
//                ext= text->drawImageMask(state, ref, str, width, height, invert, inlineImg,
//                                         dumpJPEG, imageIndex);
                lPictureReferences->append(new PictureReference(reference, flip, imageIndex, ext));

            }
        }
    } else {
        if (parameters->getDisplayImage()) {
            //HD : in order to avoid millions of small (pixel) images
            if (height > 8 && width > 8 && imageIndex < parameters->getFilesCountLimit()) {
                imageIndex += 1;
                //use reference instead
                ext = text->drawImageOrMask(state, ref, str, width, height, NULL, NULL, inlineImg, true,
                                            imageIndex); // mask
                //			text->drawImageMask(state, ref, str, width, height, invert, inlineImg,
                //				dumpJPEG, imageIndex);
                lPictureReferences->append(new PictureReference(reference, flip, imageIndex, ext));
            }
        }
    }
}

void XmlAltoOutputDev::initOutline(int nbPage) {
    char *tmp = (char *) malloc(10 * sizeof(char));
    docOutline = xmlNewDoc((const xmlChar *) VERSION);
    globalParams->setTextEncoding((char *) ENCODING_UTF8);
    docOutlineRoot = xmlNewNode(NULL, (const xmlChar *) TAG_TOCITEMS);
    sprintf(tmp, "%d", nbPage);
    xmlNewProp(docOutlineRoot, (const xmlChar *) ATTR_NB_PAGES, (const xmlChar *) tmp);
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
        dumpOutline(docOutlineRoot, itemsA, docA, uMap, levelA, idItemToc);
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
                for (j = i; j < len && !unicodeTypeR(text[j]); ++j);
                for (k = i; k < j; ++k) {
                    n = uMap->mapUnicode(text[k], buf, sizeof(buf));
                    s->append(buf, n);
                    ++nCols;
                }
                i = j;
                // output a right-to-left section
                for (j = i; j < len && !unicodeTypeL(text[j]); ++j);
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
                for (j = i; j >= 0 && !unicodeTypeL(text[j]); --j);
                for (k = i; k > j; --k) {
                    n = uMap->mapUnicode(text[k], buf, sizeof(buf));
                    s->append(buf, n);
                    ++nCols;
                }
                i = j;
                // output a left-to-right section
                for (j = i; j >= 0 && !unicodeTypeR(text[j]); --j);
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

GBool XmlAltoOutputDev::dumpOutline(xmlNodePtr parentNode, GList *itemsA, PDFDoc *docA, UnicodeMap *uMapA,
                                    int levelA, int idItemTocParentA) {
    // store them in a list
    xmlNodePtr nodeTocItem = NULL;
    xmlNodePtr nodeItem = NULL;
    xmlNodePtr nodeString = NULL;
    xmlNodePtr nodeLink = NULL;
    double x2, y2;

    GBool atLeastOne = gFalse;

    char *tmp = (char *) malloc(10 * sizeof(char));

    //    UnicodeMap *uMap;

    // Get the output encoding
    if (!(uMapA = globalParams->getTextEncoding())) {
        return 0;
    }
    int i;
    //GString *title;

    nodeTocItem = xmlNewNode(NULL, (const xmlChar *) TAG_TOCITEMLIST);
    sprintf(tmp, "%d", levelA);
    xmlNewProp(nodeTocItem, (const xmlChar *) ATTR_LEVEL, (const xmlChar *) tmp);

    if (levelA != 0) {
        sprintf(tmp, "%d", idItemTocParentA);
        xmlNewProp(nodeTocItem, (const xmlChar *) ATTR_ID_ITEM_PARENT,
                   (const xmlChar *) tmp);
    }
    //xmlAddChild(docOutlineRoot, nodeTocItem);
    xmlAddChild(parentNode, nodeTocItem);

    for (i = 0; i < itemsA->getLength(); ++i) {
        GString* title = new GString();

        ((OutlineItem *) itemsA->get(i))->open(); // open the kids
        dumpFragment(((OutlineItem *) itemsA->get(i))->getTitle(), ((OutlineItem *) itemsA->get(i))->getTitleLength(),
                     uMapA, title);

        //printf("%s\n",title->getCString());
        LinkActionKind kind;

        LinkDest *dest;
        GString *namedDest;

        GString *fileName;
        int page = 0;

        GBool destLink = gFalse;

        double left = 0;
        double top = 0;
        double right = 0;
        double bottom = 0;
        x2 = 0;
        y2 = 0;

        if (((OutlineItem *) itemsA->get(i))->getAction()) {
            switch (kind = ((OutlineItem *) itemsA->get(i))->getAction()->getKind()) {

                // GOTO action
                case actionGoTo:
                    dest = NULL;
                    namedDest = NULL;

                    if ((dest = ((LinkGoTo *) ((OutlineItem *) itemsA->get(i))->getAction())->getDest())) {
                        dest = dest->copy();
                    } else if ((namedDest = ((LinkGoTo *) ((OutlineItem *) itemsA->get(
                            i))->getAction())->getNamedDest())) {
                        namedDest = namedDest->copy();
                    }
                    if (namedDest) {
                        dest = docA->findDest(namedDest);
                    }

                    if (dest) {
                        if (dest->isPageRef()) {
                            Ref pageref = dest->getPageRef();
                            page = docA->getCatalog()->findPage(pageref.num, pageref.gen);
                        } else {
                            page = dest->getPageNum();
                        }

                        left = dest->getLeft();
                        top = dest->getTop();
                        x2 = left;
                        y2 = top;
                        //printf("%g %g %g %g %g %g\n",curstate[0],curstate[1],curstate[2],curstate[3],curstate[4],curstate[5]);
                        x2 = curstate[0] * left + curstate[2] * top + curstate[4];
                        y2 = curstate[1] * left + curstate[3] * top + curstate[5];

                        //printf("%g %g \n",x2,y2);
                        bottom = dest->getBottom();
                        right = dest->getRight();
                        destLink = gTrue;
                    }

                    if (dest) {
                        delete dest;
                    }
                    if (namedDest) {
                        delete namedDest;
                    }
                    break;


                    // LAUNCH action
                case actionLaunch:
                    //fileName = ((LinkLaunch *) ((OutlineItem *) itemsA->get(i))->getAction())->getFileName();
                    //delete fileName;
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

                    // GoToR action
                case actionGoToR:
                    break;

                    // Hide action
                case actionHide:
                    break;

                    // SubmitForm action
                case actionSubmitForm:
                    break;

                    // JavaScript action
                case actionJavaScript:
                    break;

            } // end SWITCH
        } // end IF

        // ITEM node
        nodeItem = xmlNewNode(NULL, (const xmlChar *) TAG_ITEM);
        nodeItem->type = XML_ELEMENT_NODE;
        sprintf(tmp, "%d", idItemToc);
        xmlNewProp(nodeItem, (const xmlChar *) ATTR_ID, (const xmlChar *) tmp);
        xmlAddChild(nodeTocItem, nodeItem);

        // STRING node
        nodeString = xmlNewNode(NULL, (const xmlChar *) TAG_STRING);
        nodeString->type = XML_ELEMENT_NODE;
        //   	  	xmlNodeSetContent(nodeString,(const xmlChar*)xmlEncodeEntitiesReentrant(nodeString->doc,(const xmlChar*)title->getCString()));
        xmlNodeSetContent(nodeString,
                          (const xmlChar *) xmlEncodeEntitiesReentrant(nodeString->doc,
                                                                       (const xmlChar *) title->getCString()));

        xmlAddChild(nodeItem, nodeString);

        // LINK node
        if (destLink) {
            nodeLink = xmlNewNode(NULL, (const xmlChar *) TAG_LINK);
            nodeLink->type = XML_ELEMENT_NODE;

            sprintf(tmp, "%d", page);
            xmlNewProp(nodeLink, (const xmlChar *) ATTR_PAGE, (const xmlChar *) tmp);
            snprintf(tmp, sizeof(tmp), ATTR_NUMFORMAT, y2);
            xmlNewProp(nodeLink, (const xmlChar *) ATTR_TOP, (const xmlChar *) tmp);
            snprintf(tmp, sizeof(tmp), ATTR_NUMFORMAT, bottom);
            xmlNewProp(nodeLink, (const xmlChar *) ATTR_BOTTOM, (const xmlChar *) tmp);
            snprintf(tmp, sizeof(tmp), ATTR_NUMFORMAT, x2);
            xmlNewProp(nodeLink, (const xmlChar *) ATTR_LEFT, (const xmlChar *) tmp);
            snprintf(tmp, sizeof(tmp), ATTR_NUMFORMAT, right);
            xmlNewProp(nodeLink, (const xmlChar *) ATTR_RIGHT, (const xmlChar *) tmp);

            xmlAddChild(nodeItem, nodeLink);
        }

        int idItemCurrent = idItemToc;
        idItemToc++;
        if (((OutlineItem *) itemsA->get(i))->hasKids()) {
            dumpOutline(nodeItem, ((OutlineItem *) itemsA->get(i))->getKids(), docA, uMapA, levelA + 1, idItemCurrent);
        }

        delete title;

        ((OutlineItem *) itemsA->get(i))->close(); // close the kids

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

void XmlAltoOutputDev::tilingPatternFill(GfxState *state, Gfx *gfx,
                                         Object *strRef,
                                         int paintType, int tilingType,
                                         Dict *resDict,
                                         double *mat, double *bbox,
                                         int x0, int y0, int x1, int y1,
                                         double xStep, double yStep) {
    // do nothing -- this avoids the potentially slow loop in Gfx.cc
}

/**
 * Checks if the unicode sequence is valid utf8 
 * @param u unicode sequence
 * @param uLen
 * @return true it is complied
 */
bool XmlAltoOutputDev::isUTF8(Unicode *u, int uLen) {
    char buf[8];
    int n;
    int j = 0;
    GString *s = new GString();
    UnicodeMap *uMap;
    // get the output encoding
    if (!(uMap = globalParams->getTextEncoding())) {
        return 0;
    }
    while (j < uLen) {
        n = uMap->mapUnicode(u[j], buf, sizeof(buf));
        s->append(buf, n);
        j++;
    }

    const unsigned char *str = (unsigned char*)s->getCString();
    const unsigned char *end = str + s->getLength();
    unsigned char byte;
    unsigned int code_length, i;
    uint32_t ch;
    while (str != end) {
        byte = *str;
        if (byte <= 0x7F) {
            /* 1 byte sequence: U+0000..U+007F */
            str += 1;
            continue;
        }

        if (0xC2 <= byte && byte <= 0xDF)
            /* 0b110xxxxx: 2 bytes sequence */
            code_length = 2;
        else if (0xE0 <= byte && byte <= 0xEF)
            /* 0b1110xxxx: 3 bytes sequence */
            code_length = 3;
        else if (0xF0 <= byte && byte <= 0xF4)
            /* 0b11110xxx: 4 bytes sequence */
            code_length = 4;
        else {
            /* invalid first byte of a multibyte character */
            return 0;
        }

        if (str + (code_length - 1) >= end) {
            /* truncated string or invalid byte sequence */
            return 0;
        }

        /* Check continuation bytes: bit 7 should be set, bit 6 should be
         * unset (b10xxxxxx). */
        for (i=1; i < code_length; i++) {
            if ((str[i] & 0xC0) != 0x80)
                return 0;
        }

        if (code_length == 2) {
            /* 2 bytes sequence: U+0080..U+07FF */
            ch = ((str[0] & 0x1f) << 6) + (str[1] & 0x3f);
            /* str[0] >= 0xC2, so ch >= 0x0080.
               str[0] <= 0xDF, (str[1] & 0x3f) <= 0x3f, so ch <= 0x07ff */
        } else if (code_length == 3) {
            /* 3 bytes sequence: U+0800..U+FFFF */
            ch = ((str[0] & 0x0f) << 12) + ((str[1] & 0x3f) << 6) +
                 (str[2] & 0x3f);
            /* (0xff & 0x0f) << 12 | (0xff & 0x3f) << 6 | (0xff & 0x3f) = 0xffff,
               so ch <= 0xffff */
            if (ch < 0x0800)
                return 0;

            /* surrogates (U+D800-U+DFFF) are invalid in UTF-8:
               test if (0xD800 <= ch && ch <= 0xDFFF) */
            if ((ch >> 11) == 0x1b)
                return 0;
        } else if (code_length == 4) {
            /* 4 bytes sequence: U+10000..U+10FFFF */
            ch = ((str[0] & 0x07) << 18) + ((str[1] & 0x3f) << 12) +
                 ((str[2] & 0x3f) << 6) + (str[3] & 0x3f);
            if ((ch < 0x10000) || (0x10FFFF < ch))
                return 0;
        }
        str += code_length;
    }
    return 1;
}
