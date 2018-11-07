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
#include "DataGeneratorOutputDev.h"
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
#include <xpdf/PDFDocEncoding.h>


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

// In simple layout mode, lines are broken at gaps larger than this
// value multiplied by font size.
#define simpleLayoutGapThreshold 0.4

// Max spacing (as a multiple of font size) allowed between the end of
// a line and a clipped character to be included in that line.
#define clippedTextMaxWordSpace 0.5

// Text is considered diagonal if abs(tan(angle)) > diagonalThreshold.
// (Or 1/tan(angle) for 90/270 degrees.)
#define diagonalThreshold 0.1


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
                                               : (GString *)NULL;
    flags = gfxFont ? gfxFont->getFlags() : 0;
    mWidth = 0;
    if (gfxFont && !gfxFont->isCIDFont()) {
        char *name;
        int code;
        for (code = 0; code < 256; ++code) {
            if ((name = ((Gfx8BitFont *)gfxFont)->getCharName(code)) &&
                name[0] == 'm' && name[1] == '\0') {
                mWidth = ((Gfx8BitFont *)gfxFont)->getWidth(code);
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
    unicode_block = ublock_getCode(cA);
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
        t = xMin; xMin = xMax; xMax = t;
    }
    if (yMin > yMax) {
        t = yMin; yMin = yMax; yMax = t;
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
    rot = (Guchar)rotA;
    clipped = (char)clippedA;
    invisible = (char)invisibleA;
    spaceAfter = (char)gFalse;
    font = fontA;
    fontSize = fontSizeA;
    splashfont = splashFontA;
    isNonUnicodeGlyph = isNonUnicodeGlyphA;
    colorR = colorRA;
    colorG = colorGA;
    colorB = colorBA;
}

int TextChar::cmpX(const void *p1, const void *p2) {
    const TextChar *ch1 = *(const TextChar **)p1;
    const TextChar *ch2 = *(const TextChar **)p2;

    if (ch1->xMin < ch2->xMin) {
        return -1;
    } else if (ch1->xMin > ch2->xMin) {
        return 1;
    } else {
        return 0;
    }
}

int TextChar::cmpY(const void *p1, const void *p2) {
    const TextChar *ch1 = *(const TextChar **)p1;
    const TextChar *ch2 = *(const TextChar **)p2;

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

    spaceAfter = spaceAfterA;

    isNonUnicodeGlyph = gFalse;

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
        angleSkewing_Y = static_cast<int>(angSkew);
        if (rot == 0) {
            angle = 0;
        }
    } else
        angleSkewing_Y = 0;

    if (m[1]!=0 && m[2]==0) {
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
            if (strstr(fontA->getFontName()->lowerCase()->getCString(), "bold"))
                bold=gTrue;
            if (strstr(fontA->getFontName()->lowerCase()->getCString(), "italic")|| strstr(fontA->getFontName()->lowerCase()->getCString(), "oblique"))
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
    edge = (double *)gmallocn(len + 1, sizeof(double));
    charPos = (int *)gmallocn(len + 1, sizeof(int));
    rot = rotA;
    if (rot & 1) {
        ch = (TextChar *)charsA->get(start);
        xMin = ch->xMin;
        xMax = ch->xMax;
        yMin = ch->yMin;
        ch = (TextChar *)charsA->get(start + len - 1);
        yMax = ch->yMax;
        base = xMin - descent;
        baseYmin = xMin;
    } else {
        ch = (TextChar *)charsA->get(start);
        xMin = ch->xMin;
        yMin = ch->yMin;
        yMax = ch->yMax;
        ch = (TextChar *)charsA->get(start + len - 1);
        xMax = ch->xMax;
        base = yMin + ascent;
        baseYmin = yMin;
    }

    int i = 0, j = 0;
    GBool overlap;
    double delta;
    while (j < len) {
        ch = (TextChar *)charsA->get(rot >= 2 ? start + len - 1 - j : start + j);
        chars->append(ch);

        if(ch->isNonUnicodeGlyph || (ch->unicode_block != UBLOCK_LATIN_1_SUPPLEMENT && ch->unicode_block != UBLOCK_BASIC_LATIN))
            isNonUnicodeGlyph = gTrue;

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

    if(j==len)
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

Unicode TextWord::getChar(int idx) { return ((TextChar *)chars->get(idx))->c; }

TextWord::TextWord(TextWord *word) {
    *this = *word;
//    text = (Unicode *)gmallocn(len, sizeof(Unicode));
//    memcpy(text, word->text, len * sizeof(Unicode));
        chars = word->chars->copy();
    edge = (double *)gmallocn(len + 1, sizeof(double));
    memcpy(edge, word->edge, (len + 1) * sizeof(double));
    charPos = (int *)gmallocn(len + 1, sizeof(int));
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
        angleSkewing_Y = static_cast<int>(angSkew);
        if (rot == 0) {
            angle = 0;
        }
    }

    if (m[1]!=0 && m[2]==0) {
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

    //text = NULL;
    chars = new GList();
    charPos = NULL;
    edge = NULL;
    len = size = 0;
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

    isNonUnicodeGlyph = gFalse;
}

TextRawWord::~TextRawWord() {
    //gfree(text);
    gfree(edge);
}

Unicode TextRawWord::getChar(int idx) { return ((TextChar *)chars->get(idx))->c; }

void TextRawWord::addChar(GfxState *state, double x, double y, double dx,
                       double dy, Unicode u, CharCode charCodeA, int charPosA, GBool overlap, TextFontInfo *fontA, double fontSizeA, SplashFont* splashFont, int rotA, int nBytes, GBool isNonUnicodeGlyph) {

    if (len == size) {
        size += 16;
        //text = (Unicode *)grealloc(text, size * sizeof(Unicode));
        edge = (double *)grealloc(edge, (size + 1) * sizeof(double));
        //charPos = (int *)grealloc(charPos, size + 1 * sizeof(int));
        //chars = new GList();
    }

    GfxRGB rgb;
    double alpha;

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
        charState->setCTM(state->getCTM()[0], state->getCTM()[1], state->getCTM()[2], state->getCTM()[3], state->getCTM()[4], state->getCTM()[5]);
        charState->setTextMat(state->getTextMat()[0], state->getTextMat()[1], state->getTextMat()[2], state->getTextMat()[3], state->getTextMat()[4], state->getTextMat()[5]);
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
        edge = (double *)grealloc(edge, (size + 1) * sizeof(double));
    }
    for (i = 0; i < word->len; ++i) {
        chars->append(((TextChar *)word->chars->get(i)));
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
    return cmp< 0 ? -1 : cmp> 0 ?1 : 0;
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

GBool TextRawWord::overlap(TextRawWord *w2){

    return gFalse;
}

void IWord::setContainNonUnicodeGlyph(GBool containNonUnicodeGlyph) {
    isNonUnicodeGlyph = containNonUnicodeGlyph;
}

//------------------------------------------------------------------------
// TextRawWord
//------------------------------------------------------------------------

int IWord::cmpX(const void *p1, const void *p2) {
    const TextRawWord *word1 = *(const TextRawWord **)p1;
    const TextRawWord *word2 = *(const TextRawWord **)p2;

    if (word1->xMin < word2->xMin) {
        return -1;
    } else if (word1->xMin > word2->xMin) {
        return 1;
    } else {
        return 0;
    }
}

int IWord::cmpY(const void *p1, const void *p2) {
    const TextRawWord *word1 = *(const TextRawWord **)p1;
    const TextRawWord *word2 = *(const TextRawWord **)p2;

    if (word1->yMin < word2->yMin) {
        return -1;
    } else if (word1->yMin > word2->yMin) {
        return 1;
    } else {
        return 0;
    }
}

int IWord::cmpYX(const void *p1, const void *p2) {
    TextRawWord *word1 = *(TextRawWord **)p1;
    TextRawWord *word2 = *(TextRawWord **)p2;
    double cmp;

    cmp = word1->yMin - word2->yMin;
    if (cmp == 0) {
        cmp = word1->xMin - word2->xMin;
    }
    return cmp< 0 ? -1 : cmp> 0 ?1 : 0;
}

GString *IWord::convtoX(double xcol) const {
    GString *xret=new GString();
    char tmp;
    unsigned int k;
    k = static_cast<int>(xcol);
    k= k/16;
    if ((k>0)&&(k<10))
        tmp=(char) ('0'+k);
    else
        tmp=(char)('a'+k-10);
    xret->append(tmp);
    k = static_cast<int>(xcol);
    k= k%16;
    if ((k>0)&&(k<10))
        tmp=(char) ('0'+k);
    else
        tmp=(char)('a'+k-10);
    xret->append(tmp);

    return xret;
}

GString *IWord::colortoString() const {
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

const char* IWord::normalizeFontName(char* fontName) {
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
// TextLine
//------------------------------------------------------------------------

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
        word = (TextWord *)wordsA->get(i);
        len += word->getLength();
        if (word->spaceAfter) {
            ++len;
        }
    }
    text = (Unicode *)gmallocn(len, sizeof(Unicode));
    edge = (double *)gmallocn(len + 1, sizeof(double));
    j = 0;
    for (i = 0; i < words->getLength(); ++i) {
        word = (TextWord *)words->get(i);
        if (i == 0) {
            rot = word->rot;
        }
        for (k = 0; k < word->getLength(); ++k) {
            text[j] = ((TextChar *)word->chars->get(k))->c;
            edge[j] = word->edge[k];
            ++j;
        }
        edge[j] = word->edge[word->getLength()];
        if (word->spaceAfter) {
            text[j] = (Unicode)0x0020;
            ++j;
            edge[j] = edge[j - 1];
        }
    }
    //~ need to check for other Unicode chars used as hyphens
    hyphenated = text[len - 1] == (Unicode)'-';
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
    const TextLine *line1 = *(const TextLine **)p1;
    const TextLine *line2 = *(const TextLine **)p2;

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

TextParagraph::TextParagraph(GList *linesA, GBool dropCapA) {
    TextLine *line;
    int i;

    lines = linesA;
    dropCap = dropCapA;
    xMin = yMin = xMax = yMax = 0;
    for (i = 0; i < lines->getLength(); ++i) {
        line = (TextLine *)lines->get(i);
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

    par = (TextParagraph *)paragraphs->get(0);
    line = (TextLine *)par->getLines()->get(0);
    return line->getRotation();
}

int TextColumn::cmpX(const void *p1, const void *p2) {
    const TextColumn *col1 = *(const TextColumn **)p1;
    const TextColumn *col2 = *(const TextColumn **)p2;

    if (col1->xMin < col2->xMin) {
        return -1;
    } else if (col1->xMin > col2->xMin) {
        return 1;
    } else {
        return 0;
    }
}

int TextColumn::cmpY(const void *p1, const void *p2) {
    const TextColumn *col1 = *(const TextColumn **)p1;
    const TextColumn *col2 = *(const TextColumn **)p2;

    if (col1->yMin < col2->yMin) {
        return -1;
    } else if (col1->yMin > col2->yMin) {
        return 1;
    } else {
        return 0;
    }
}

int TextColumn::cmpPX(const void *p1, const void *p2) {
    const TextColumn *col1 = *(const TextColumn **)p1;
    const TextColumn *col2 = *(const TextColumn **)p2;

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
    GBool smallSplit;		// true for blkVertSplit/blkHorizSplit
    //   where the gap size is small
    GList *children;		// for blkLeaf, children are TextWord;
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

    child = (TextBlock *)children->get(childIdx);
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
                   GString* dir, GString *base, GString *nsURIA) {

    splash = NULL;
    verbose = verboseA;
    //rawOrder = 1;

    // PL: to modify block order according to reading order
    if (parameters->getReadingOrder() == gTrue) {
        chars = new GList();
        readingOrder = 1;
    } else {
        words = new GList();
        readingOrder = 0;
    }

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

    rawWords = NULL;
    rawLastWord = NULL;
    fonts = new GList();
    lastFindXMin = lastFindYMin = 0;
    haveLastFind = gFalse;
}

TextPage::~TextPage() {
    clear();
    delete fonts;
    if (namespaceURI) {
        delete namespaceURI;
    }

    if (splash) {
        delete splash;
    }
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

    if (state) {
        pageWidth = state->getPageWidth();
        pageHeight = state->getPageHeight();
    } else {
        pageWidth = pageHeight = 0;
    }
    curstate=state;

    tmp = (char*)malloc(20*sizeof(char));

    // Cut all pages OK
    if (cutter) {
        globalParams->setTextEncoding((char*)ENCODING_UTF8);
    }

    globalParams->setTextEncoding((char*)ENCODING_UTF8);

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
    if (!readingOrder) {
        primaryRot = 0;
        primaryLR = gTrue;
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
    TextRawWord *word;

    if (curWord) {
        delete curWord;
        curWord = NULL;
    }
    if (readingOrder) {
        deleteGList(chars, TextChar);
        chars = new GList();
    } else {
        while (rawWords) {
            word = rawWords;
            rawWords = rawWords->next;
            delete word;
        }
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

    rawWords = NULL;
    rawLastWord = NULL;
    fonts = new GList();
}

void TextPage::beginActualText(GfxState *state, Unicode *u, int uLen) {
    if (actualText) {
        gfree(actualText);
    }
    actualText = (Unicode *)gmallocn(uLen, sizeof(Unicode));
    memcpy(actualText, u, uLen * sizeof(Unicode));
    actualTextLen = uLen;
    actualTextNBytes = 0;
}

void TextPage::endActualText(GfxState *state, SplashFont* splashFont) {
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
    if (fabs(m[0]) >= fabs(m[1]))  {
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

    curWord
            = new TextRawWord(state, x0, y0, curFont, curFontSize, getIdWORD(), getIdx());
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

void TextPage::addCharToPageChars(GfxState *state, double x, double y, double dx,
                       double dy, CharCode c, int nBytes, Unicode *u, int uLen, SplashFont * splashFont, GBool isNonUnicodeGlyph){
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
    if (c == (CharCode)0x20) {
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
    if (uLen == 1 && (u[0] == (Unicode)0x20 ||
                      u[0] == (Unicode)0x09 ||
                      u[0] == (Unicode)0xa0)) {
        charPos += nBytes;
        if (chars->getLength() > 0) {
            ((TextChar *)chars->get(chars->getLength() - 1))->spaceAfter =
                    (char)gTrue;
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
                       double dy, CharCode c, int nBytes, Unicode *u, int uLen, SplashFont * splashFont, GBool isNonUnicodeGlyph){

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

    // subtract char and word spacing from the dx,dy values // HD why ??
    sp = state->getCharSpace();
    if (c == (CharCode) 0x20) {
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
    if (uLen == 1 && (u[0] == (Unicode)0x20 ||
                       u[0] == (Unicode)0x09 ||
                       u[0] == (Unicode)0xa0)) {
        if (curWord) {
            ++curWord->charLen;
            curWord->setSpaceAfter(gTrue);
            if(curWord->chars->getLength() > 0)
            ((TextChar *)curWord->chars->get(curWord->chars->getLength() - 1))->spaceAfter =
                    (char)gTrue;
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

        // take into account rotation angle ??
        if (((overlap  || fabs(base - curWord->base) > 1 ||
             sp > minWordBreakSpace * curWord->fontSize ||
                (sp < -minDupBreakOverlap * curWord->fontSize)))) {
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
            if(isNonUnicodeGlyph)
                curWord->setContainNonUnicodeGlyph(isNonUnicodeGlyph);
            curWord->addChar(state, x1 + i * w1, y1 + i * h1, w1, h1, u[i], c, charPos, (overlap || sp < -minDupBreakOverlap * curWord->fontSize), curFont, curFontSize, splashFont, nBytes, curRot, isNonUnicodeGlyph);
        }
    }

    if (curWord) {
        curWord->charLen += nBytes;
    }
    charPos += nBytes;
}

void TextPage::addChar(GfxState *state, double x, double y, double dx,
                       double dy, CharCode c, int nBytes, Unicode *u, int uLen, SplashFont * splashFont, GBool isNonUnicodeGlyph) {

//    if(parameters->getReadingOrder() == gTrue)
        addCharToPageChars(state, x, y, dx, dy, c, nBytes, u, uLen, splashFont, isNonUnicodeGlyph);
//    else
//        addCharToRawWord(state, x, y, dx, dy, c, nBytes, u, uLen, splashFont, isNonUnicodeGlyph);
}

void TextPage::endWord() {
    // This check is needed because Type 3 characters can contain
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
//        words->append(word);
//    } else {
        if (rawLastWord) {
            rawLastWord->next = word;
        } else {
            rawWords = word;
        }
        rawLastWord = word;
//    }
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
                        } case actionGoToR:{
                            break;
                        } case actionLaunch:{
                            break;
                        } case actionNamed:{
                            break;
                        } case actionMovie:{
                            break;
                        } case actionJavaScript:{
                            break;
                        } case actionSubmitForm:{
                            break;
                        } case actionHide:{
                            break;
                        } case actionUnknown:{
                            break;
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
    if( ((fontName && fontName->cmp(tsi->getFontName())  == 0 )
            &&  (fontSize == tsi->getFontSize())
            && (fontColor && fontColor->cmp(tsi->getFontColor()) == 0 )
            && (fontType == tsi->getFontType())
            && (fontWidth == tsi->getFontWidth())
            && (isbold == tsi->isBold())
            && (isitalic == tsi->isItalic())
//            && (issubscript == tsi->isSubscript())
//            && (issuperscript == tsi->isSuperscript())
    )
            )
        return gTrue;
    else return gFalse;
}

//------------------------------------------------------------------------
// TextGap
//------------------------------------------------------------------------

class TextGap {
public:

    TextGap(double aXY, double aW): xy(aXY), w(aW) {}

    double xy;			// center of gap: x for vertical gaps,
    //   y for horizontal gaps
    double w;			// width of gap
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
        ch = (TextChar *)charsA->get(i);
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
                ch = (TextChar *)charsA->get(i);
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
                ch = (TextChar *)charsA->get(i);
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
                ch = (TextChar *)charsA->get(i);
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
                word = (TextWord *)wordsA->get(i);
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
                word = (TextWord *)wordsA->get(i);
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
                word = (TextWord *)wordsA->get(i);
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
        ch = (TextChar *)charsA->get(i);
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
            ch = (TextChar *)charsA->get(i);
            if (ch->rot == rot ) {
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
        ch = (TextChar *)clippedChars->del(0);
        if (ch->rot != 0) {
            continue;
        }
        if (!(leaf = findClippedCharLeaf(ch, tree))) {
            continue;
        }
        leaf->addChild(ch, gFalse);
        i = 0;
        while (i < clippedChars->getLength()) {
            ch2 = (TextChar *)clippedChars->get(i);
            if (ch2->xMin > ch->xMax + clippedTextMaxWordSpace * ch->fontSize) {
                break;
            }
            y = 0.5 * (ch2->yMin + ch2->yMax);
            if (y > leaf->yMin && y < leaf->yMax) {
                ch2 = (TextChar *)clippedChars->del(i);
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
            child = (TextBlock *)tree->children->get(i);
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
        gap = (TextGap *)horizGaps->get(i);
        if (gap->w > horizGapSize) {
            horizGapSize = gap->w;
        }
    }
    vertGapSize = 0;
    for (i = 0; i < vertGaps->getLength(); ++i) {
        gap = (TextGap *)vertGaps->get(i);
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
            gap = (TextGap *)horizGaps->get(i);
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
            gap = (TextGap *)vertGaps->get(i);
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
//    if (control.mode == textOutTableLayout) {
//        vertGapThreshold = vertGapThresholdTableMax
//                           + vertGapThresholdTableSlope * nLines;
//        if (vertGapThreshold < vertGapThresholdTableMin) {
//            vertGapThreshold = vertGapThresholdTableMin;
//        }
//    } else if (control.mode == textOutSimpleLayout) {
//        vertGapThreshold = simpleLayoutGapThreshold;
//    } else {
        vertGapThreshold = vertGapThresholdMax + vertGapThresholdSlope * nLines;
        if (vertGapThreshold < vertGapThresholdMin) {
            vertGapThreshold = vertGapThresholdMin;
        }
    //}
    vertGapThreshold = vertGapThreshold * avgFontSize;

    // compute the minimum allowed chunk width
//    if (control.mode == textOutTableLayout) {
//        minChunk = 0;
//    } else {
        minChunk = vertSplitChunkThreshold * avgFontSize;
    //}

    // look for large chars
    // -- this kludge (multiply by 256, convert to int, divide by 256.0)
    //    prevents floating point stability issues on x86 with gcc, where
    //    largeCharSize could otherwise have slightly different values
    //    here and where it's used below to do the large char partition
    //    (because it gets truncated from 80 to 64 bits when spilled)
    largeCharSize = (int)(largeCharThreshold * avgFontSize * 256) / 256.0;
    nLargeChars = 0;
    for (i = 0; i < charsA->getLength(); ++i) {
        ch = (TextChar *)charsA->get(i);
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
            gap = (TextGap *)vertGaps->get(i);
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
            gap = (TextGap *)horizGaps->get(i);
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
            ch = (TextChar *)charsA->get(i);
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
            blk->addChild((TextChar *)charsA->get(i), gTrue);
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
        ch = (TextChar *)charsA->get(i);
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
        ch = (TextChar *)largeChars->get(i-1);
        ch2 = (TextChar *)largeChars->get(i);
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
            ch = (TextChar *)largeChars->get(i);
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
            ch = (TextChar *)largeChars->get(i);
            blk->prependChild(ch);
        }
    } else {
        insertLargeCharsInFirstLeaf(largeChars, (TextBlock *)blk->children->get(0));
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
            child = (TextBlock *)blk->children->get(i);
            if (y < child->yMax || i == blk->children->getLength() - 1) {
                insertLargeCharInLeaf(ch, child);
                blk->updateBounds(i);
                break;
            }
        }
    } else {
        insertLargeCharInLeaf(ch, (TextBlock *)blk->children->get(0));
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
        ch = (TextChar *)charsA->get(i);
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
    xMinI = (int)floor(xMin / splitPrecision) - 1;
    yMinI = (int)floor(yMin / splitPrecision) - 1;
    xMaxI = (int)floor(xMax / splitPrecision) + 1;
    yMaxI = (int)floor(yMax / splitPrecision) + 1;
    horizProfile = (int *)gmallocn(yMaxI - yMinI + 1, sizeof(int));
    vertProfile = (int *)gmallocn(xMaxI - xMinI + 1, sizeof(int));
    memset(horizProfile, 0, (yMaxI - yMinI + 1) * sizeof(int));
    memset(vertProfile, 0, (xMaxI - xMinI + 1) * sizeof(int));
    for (i = 0; i < charsA->getLength(); ++i) {
        ch = (TextChar *)charsA->get(i);
        // yMinI2 and yMaxI2 are adjusted to allow for slightly overlapping lines
        switch (rot) {
            case 0:
            default:
                xMinI2 = (int)floor(ch->xMin / splitPrecision);
                xMaxI2 = (int)floor(ch->xMax / splitPrecision);
                ascentAdjust = ascentAdjustFactor * (ch->yMax - ch->yMin);
                yMinI2 = (int)floor((ch->yMin + ascentAdjust) / splitPrecision);
                descentAdjust = descentAdjustFactor * (ch->yMax - ch->yMin);
                yMaxI2 = (int)floor((ch->yMax - descentAdjust) / splitPrecision);
                break;
            case 1:
                descentAdjust = descentAdjustFactor * (ch->xMax - ch->xMin);
                xMinI2 = (int)floor((ch->xMin + descentAdjust) / splitPrecision);
                ascentAdjust = ascentAdjustFactor * (ch->xMax - ch->xMin);
                xMaxI2 = (int)floor((ch->xMax - ascentAdjust) / splitPrecision);
                yMinI2 = (int)floor(ch->yMin / splitPrecision);
                yMaxI2 = (int)floor(ch->yMax / splitPrecision);
                break;
            case 2:
                xMinI2 = (int)floor(ch->xMin / splitPrecision);
                xMaxI2 = (int)floor(ch->xMax / splitPrecision);
                descentAdjust = descentAdjustFactor * (ch->yMax - ch->yMin);
                yMinI2 = (int)floor((ch->yMin + descentAdjust) / splitPrecision);
                ascentAdjust = ascentAdjustFactor * (ch->yMax - ch->yMin);
                yMaxI2 = (int)floor((ch->yMax - ascentAdjust) / splitPrecision);
                break;
            case 3:
                ascentAdjust = ascentAdjustFactor * (ch->xMax - ch->xMin);
                xMinI2 = (int)floor((ch->xMin + ascentAdjust) / splitPrecision);
                descentAdjust = descentAdjustFactor * (ch->xMax - ch->xMin);
                xMaxI2 = (int)floor((ch->xMax - descentAdjust) / splitPrecision);
                yMinI2 = (int)floor(ch->yMin / splitPrecision);
                yMaxI2 = (int)floor(ch->yMax / splitPrecision);
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

    for (start = yMinI; start < yMaxI && !horizProfile[start - yMinI]; ++start) ;
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

    for (start = xMinI; start < xMaxI && !vertProfile[start - xMinI]; ++start) ;
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
                child = (TextBlock *)blk->children->get(i);
                if (child->tag != blkTagColumn && child->tag != blkTagLine) {
                    blk->tag = blkTagMulticolumn;
                    break;
                }
            }
        } else {
            if (blk->smallSplit) {
                blk->tag = blkTagLine;
                for (i = 0; i < blk->children->getLength(); ++i) {
                    child = (TextBlock *)blk->children->get(i);
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
                buildColumns2((TextBlock *)blk->children->get(i), columns, primaryLR);
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
        line0 = (TextLine *)lines->get(i);
        parLines->append(line0);
        ++i;

        if (i < lines->getLength()) {
            line1 = (TextLine *)lines->get(i);
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
                    line1 = (TextLine *)lines->get(i);
                    indent1 = getLineIndent(line1, blk);
                    fontSize1 = line1->fontSize;
                    if (indent0 - indent1 > minParagraphIndent * fontSize0) {
                        break;
                    }
                    if (fabs(fontSize0 - fontSize1) > paragraphFontSizeDelta) {
                        break;
                    }
                    if (getLineSpacing((TextLine *)lines->get(i - 1), line1)
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
                    line1 = (TextLine *)lines->get(i);
                    indent1 = getLineIndent(line1, blk);
                    if (indent1 - indent0 <= minParagraphIndent * fontSize0) {
                        break;
                    }
                    if (getLineSpacing((TextLine *)lines->get(i - 1), line1)
                        > spaceThresh) {
                        break;
                    }
                    parLines->append(line1);
                }
                for (; i < lines->getLength(); ++i) {
                    line1 = (TextLine *)lines->get(i);
                    indent1 = getLineIndent(line1, blk);
                    fontSize1 = line1->fontSize;
                    if (indent1 - indent0 > minParagraphIndent * fontSize0) {
                        break;
                    }
                    if (fabs(fontSize0 - fontSize1) > paragraphFontSizeDelta) {
                        break;
                    }
                    if (getLineSpacing((TextLine *)lines->get(i - 1), line1)
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
                    line1 = (TextLine *)lines->get(i);
                    indent1 = getLineIndent(line1, blk);
                    fontSize1 = line1->fontSize;
                    if (indent1 - indent0 > minParagraphIndent * fontSize0) {
                        break;
                    }
                    if (fabs(fontSize0 - fontSize1) > paragraphFontSizeDelta) {
                        break;
                    }
                    if (getLineSpacing((TextLine *)lines->get(i - 1), line1)
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
        default: indent = line->xMin - blk->xMin;  break;
        case 1:  indent = line->yMin - blk->yMin;  break;
        case 2:  indent = blk->xMax  - line->xMax; break;
        case 3:  indent = blk->yMax  - line->yMax; break;
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
        sp = getLineSpacing((TextLine *)lines->get(i - 1),
                            (TextLine *)lines->get(i));
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
        default: sp = line1->yMin - line0->yMax; break;
        case 1:  sp = line0->xMin - line1->xMax; break;
        case 2:  sp = line0->yMin - line1->yMin; break;
        case 3:  sp = line1->xMin - line1->xMax; break;
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
                buildLines((TextBlock *)blk->children->get(i), lines);
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
        dir = getCharDirection((TextChar *)charsA->get(i));
        for (j = i+1; j < charsA->getLength(); ++j) {
            ch = (TextChar *)charsA->get(j-1);
            ch2 = (TextChar *)charsA->get(j);
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
                            (blk->rot >= 2) ? spaceBefore : spaceAfter, ((TextChar *)charsA->get(i))->state, ((TextChar *)charsA->get(i))->font, ((TextChar *)charsA->get(i))->fontSize, getIdWORD(), getIdx());

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
            getLineChars((TextBlock *)blk->children->get(i), charsA);
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
        ch = (TextChar *)charsA->get(i);
        avgFontSize += ch->fontSize;
        if (i < charsA->getLength() - 1) {
            ch2 = (TextChar *)charsA->get(i+1);
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

// Merge blk (rot != 0) into primaryTree (rot == 0).
void TextPage::insertIntoTree(TextBlock *blk, TextBlock *primaryTree) {
    TextBlock *child;

    // we insert a whole column at a time - so call insertIntoTree
    // recursively until we get to a column (or line)

    if (blk->tag == blkTagMulticolumn) {
        while (blk->children->getLength()) {
            child = (TextBlock *)blk->children->del(0);
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
        child = (TextBlock *)tree->children->get(i);
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
                child = (TextBlock *)tree->children->get(i);
                if (column->xMax > 0.5 * (child->xMin + child->xMax)) {
                    break;
                }
            }
        } else {
            for (i = 0; i < tree->children->getLength(); ++i) {
                child = (TextBlock *)tree->children->get(i);
                if (column->xMin < 0.5 * (child->xMin + child->xMax)) {
                    break;
                }
            }
        }
    } else if (tree->type == blkHorizSplit) {
        if (tree->rot >= 2) {
            for (i = 0; i < tree->children->getLength(); ++i) {
                child = (TextBlock *)tree->children->get(i);
                if (column->yMax > 0.5 * (child->yMin + child->yMax)) {
                    break;
                }
            }
        } else {
            for (i = 0; i < tree->children->getLength(); ++i) {
                child = (TextBlock *)tree->children->get(i);
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

// Map StrokeAdjustMode (from GlobalParams) to SplashStrokeAdjustMode
// (for Splash).
static SplashStrokeAdjustMode mapStrokeAdjustMode[3] = {
        splashStrokeAdjustOff,
        splashStrokeAdjustNormal,
        splashStrokeAdjustCAD
};

void TextPage::dumpInReadingOrder(GBool blocks, GBool fullFontName) {
    TextBlock *tree;
    TextColumn *col;
    TextParagraph *par;
    TextLine *line;
    TextWord *word, *sWord, *nextWord;
    GList *columns;
    //GBool primaryLR;

    int colIdx, parIdx, lineIdx, wordI, wordJ, rot, n;

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
//    if (control.html) {
//        rotateUnderlinesAndLinks(rot);
//        generateUnderlinesAndLinks(columns);
//    }
#if 0 //~debug
    dumpColumns(columns);
#endif

    xmlNodePtr node = NULL;
    xmlNodePtr nodeline = NULL;
    xmlNodePtr nodeblocks = NULL;
    xmlNodePtr nodeImageInline = NULL;

    double previousWordBaseLine = 0;
    double previousWordYmin = 0;
    double previousWordYmax = 0;

    // For TEXT tag attributes
    double xMin = 0;
    double yMin = 0;
    double xMax = 0;
    double yMax = 0;
    double yMaxRot = 0;
    double yMinRot = 0;
    double xMaxRot = 0;
    double xMinRot = 0;

    GString *id;

    int data_count = 0;

    for (colIdx = 0; colIdx < columns->getLength(); ++colIdx) {
        col = (TextColumn *)columns->get(colIdx);
        for (parIdx = 0; parIdx < col->paragraphs->getLength(); ++parIdx) {

            if (blocks) {

                delete id;
                numBlock = numBlock + 1;
            }

            par = (TextParagraph *)col->paragraphs->get(parIdx);
            for (lineIdx = 0; lineIdx < par->lines->getLength(); ++lineIdx) {
                line = (TextLine *)par->lines->get(lineIdx);


                n = line->len;
                if (line->hyphenated && lineIdx + 1 < par->lines->getLength()) {
                    --n;
                }

                for (wordI = 0; wordI < line->words->getLength(); ++wordI) {

                    word = (TextWord *)line->words->get(wordI);

                    if(wordI < line->words->getLength() - 1)
                        nextWord = (TextWord *)line->words->get(wordI+1);


                    if(word->containNonUnicodeGlyph()){
                        SplashColorMode colorMode = splashModeRGB8;
                        GBool bitmapTopDown = gTrue;
                        GBool allowAntialias = gTrue;
                        SplashColor paperColor;
                        setupScreenParams(128, 64);
                        paperColor[0] = paperColor[1] = paperColor[2] = 0xff;
                        GBool vectorAntialias = allowAntialias &&
                                                globalParams->getVectorAntialias() &&
                                                colorMode != splashModeMono1;
                        int bitmapRowPad = 1;
                        //This
                        SplashBitmap *bitmap = new SplashBitmap(512, 64, bitmapRowPad, colorMode,
                                                                colorMode != splashModeMono1, bitmapTopDown);

                        splash = new Splash(bitmap, vectorAntialias, &screenParams);
                        splash->setMinLineWidth(globalParams->getMinLineWidth());
                        splash->setStrokeAdjust(
                                mapStrokeAdjustMode[globalParams->getStrokeAdjust()]);
                        splash->setEnablePathSimplification(
                                globalParams->getEnablePathSimplification());
                        splash->clear(paperColor, 0);
//        int render = state->getRender();
                        //if (needFontUpdate) {
                        //}
                        int firstCharXmin = word->xMin;
                        CharCode c = 0;
                        int k = 0;
                        int y = (int)(parameters->getCharCount()*0.5);

                        for (wordJ = wordI-1; wordJ >= 0; --wordJ) {
                            sWord = (TextWord *)line->words->get(wordJ);
                            int j = sWord->getLength() - 1 ;
                            while(j > 0) {
                                TextChar *wordchar = (TextChar *) sWord->chars->get(j);
                                splash->fillChar((SplashCoord) (256 + wordchar->xMin - firstCharXmin), (SplashCoord) 50,
                                                 wordchar->charCode,
                                                 wordchar->splashfont);

                                j--;
                                k++;
                                if(k >= y)
                                    goto rightPart;
                            }
                        }
                        rightPart:
                        for (wordJ = wordI; wordJ < line->words->getLength(); ++wordJ) {
                            sWord = (TextWord *)line->words->get(wordJ);
                            int j = 0;
                            while(sWord->getLength() > j) {
                                TextChar *wordchar = (TextChar *) sWord->chars->get(j);
                                if (wordchar->isNonUnicodeGlyph)
                                    c = wordchar->charCode;
                                splash->fillChar((SplashCoord) (256 + wordchar->xMin - firstCharXmin), (SplashCoord) 50,
                                                 wordchar->charCode,
                                                 wordchar->splashfont);

                                j++;
                                k++;
                                if(k >= parameters->getCharCount())
                                    goto theEnd;
                            }
                        }
                        theEnd:

                        FILE *f;
                        png_structp png;
                        png_infop pngInfo;
                        GString *pngFile;
                        pngFile = GString::format("e{0:s}-{1:02d}.png", to_string(data_count).c_str(), c);
                        if (!(f = fopen(pngFile->getCString(), "wb"))) {
                            exit(2);
                        }
                        delete pngFile;
                        double resolution = 500;
                        setupPNG(&png, &pngInfo, f,
                                 8, gFalse ? PNG_COLOR_TYPE_RGB_ALPHA : PNG_COLOR_TYPE_RGB,
                                 resolution, bitmap);
                        writePNGData(png, bitmap);
                        finishPNG(&png, &pngInfo);

                        fclose(f);

                        data_count++;

                    }

                    char* tmp;

                    tmp=(char*)malloc(10*sizeof(char));

                    fontStyleInfo = new TextFontStyleInfo;


                    //AA : this is a naive heuristic ( regarding general typography cases ) super/sub script, wikipedia description is good
                    // first is clear, second check is in case of firstword in line and superscript which is recurrent for declaring affiliations or even refs.
                    if((word->base < previousWordBaseLine && word->yMax > previousWordYmin)|| (wordI == 0 && wordI < line->words->getLength() - 1 && word->base < nextWord->base && word->yMax > nextWord->yMin))
                        fontStyleInfo->setIsSuperscript(gTrue);
                    else if(((wordI > 0) && word->base > previousWordBaseLine && word->yMin > previousWordYmax ))
                        fontStyleInfo->setIsSubscript(gTrue);

                    previousWordBaseLine = word->base;
                    previousWordYmin = word->yMin;
                    previousWordYmax = word->yMax;

                    free(tmp);
                }

            }
        }
        //(*outputFunc)(outputStream, eol, eolLen);
    }
    deleteGList(columns, TextColumn);
}

void TextPage::saveState(GfxState *state) {
    idStack.push(idCur);
}

void TextPage::restoreState(GfxState *state) {
if(!idStack.empty()) {
    idCur = idStack.top();
    idStack.pop();
}
}

void TextPage::doPathForClip(GfxPath *path, GfxState *state,
                             xmlNodePtr currentNode) {

    // Increment the absolute object index
    idx++;
}

void TextPage::doPath(GfxPath *path, GfxState *state, GString* gattributes) {

    // Increment the absolute object index
    idx++;
}

void TextPage::clip(GfxState *state) {
    idClip++;
    idCur = idClip;

    xmlNodePtr gnode = NULL;
    char tmp[100];

    // Increment the absolute object index
    idx++;
}

void TextPage::eoClip(GfxState *state) {

    idClip++;
    idCur = idClip;

    xmlNodePtr gnode = NULL;
    char tmp[100];

    // Increment the absolute object index
    idx++;
}

void TextPage::clipToStrokePath(GfxState *state) {

    idClip++;
    idCur = idClip;

    // Increment the absolute object index
    idx++;

}

const char* TextPage::drawImageOrMask(GfxState *state, Object* ref, Stream *str,
                                      int width, int height,
                                      GfxImageColorMap *colorMap,
                                      int* /* maskColors */, GBool inlineImg, GBool mask, int imageIndex)
{
    const char* extension = NULL;

    char tmp[10];

    // Increment the absolute object index
    idx++;
    // get image position and size
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
// DataGeneratorOutputDev
//------------------------------------------------------------------------

DataGeneratorOutputDev::DataGeneratorOutputDev(GString *fileName, GString *fileNamePdf,
                           Catalog *catalog, GBool physLayoutA, GBool verboseA, GString *nsURIA,
                           GString *cmdA)	{
    text = NULL;
    fontEngine = NULL;
    physLayout = physLayoutA;
    //rawOrder = 1;
    ok = gTrue;

    verbose = verboseA;

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

    myCatalog = catalog;
    UnicodeMap *uMap;

    blocks = parameters->getDisplayBlocks();
    fullFontName = parameters->getFullFontName();
    noImageInline = parameters->getImageInline();

    fileNamePDF = new GString(fileNamePdf);

    globalParams->setTextEncoding((char*)ENCODING_UTF8);
    needClose = gFalse;

    delete fileNamePDF;

    text = new TextPage(verbose, catalog, NULL, NULL, baseFileName, NULL);
}

DataGeneratorOutputDev::~DataGeneratorOutputDev() {
    int j;

    for (j = 0; j < nT3Fonts; ++j) {
        delete t3FontCache[j];
    }
    if (fontEngine) {
        delete fontEngine;
    }

    if (text) {
        delete text;
    }
}

void DataGeneratorOutputDev::beginActualText(GfxState *state, Unicode *u, int uLen) {
    text->beginActualText(state, u, uLen);
}

void DataGeneratorOutputDev::endActualText(GfxState *state) {

    SplashCoord mat[6];
    mat[0] = (SplashCoord)curstate[0];
    mat[1] = (SplashCoord)curstate[1];
    mat[2] = (SplashCoord)curstate[2];
    mat[3] = (SplashCoord)curstate[3];
    mat[4] = (SplashCoord)curstate[4];
    mat[5] = (SplashCoord)curstate[5];
    SplashFont* splashFont = getSplashFont(state, mat);
    text->endActualText(state, splashFont);
}


GBool DataGeneratorOutputDev::needNonText() {
    if(parameters->getDisplayImage())
        return gTrue;
    else return gFalse;
}

xmlNodePtr DataGeneratorOutputDev::findNodeByName(xmlNodePtr rootnode, const xmlChar * nodename)
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

GString* DataGeneratorOutputDev::toUnicode(GString *s,UnicodeMap *uMap){

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

void DataGeneratorOutputDev::startPage(int pageNum, GfxState *state) {
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

void DataGeneratorOutputDev::endPage() {
    text->configuration();
    if (parameters->getDisplayText()) {

//            if (readingOrder) {
            text->dumpInReadingOrder(blocks, fullFontName);
//        }
//        else
//            text->dump(blocks, fullFontName);
    }

    text->endPage(dataDir);
}

void DataGeneratorOutputDev::updateFont(GfxState *state) {
    needFontUpdate = gTrue;
    text->updateFont(state);
}

void DataGeneratorOutputDev::startDoc(XRef *xrefA) {
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

class SplashOutFontFileID: public SplashFontFileID {
public:

    SplashOutFontFileID(Ref *rA) {
        r = *rA;
        substIdx = -1;
        oblique = 0;
    }
    ~SplashOutFontFileID() {}

    GBool matches(SplashFontFileID *id) {
        return ((SplashOutFontFileID *)id)->r.num == r.num &&
               ((SplashOutFontFileID *)id)->r.gen == r.gen;
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

SplashFont* DataGeneratorOutputDev::getSplashFont(GfxState *state, SplashCoord* matrix) {
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
    mat[0] = m11;  mat[1] = m12;
    mat[2] = m21;  mat[3] = m22;
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

void DataGeneratorOutputDev::drawChar(GfxState *state, double x, double y, double dx,
                            double dy, double originX, double originY, CharCode c, int nBytes,
                            Unicode *u, int uLen) {

    GBool isNonUnicodeGlyph = gFalse;

    SplashFont* splashFont = NULL;


    SplashCoord mat[6];
    mat[0] = (SplashCoord)(curstate[0] );
    mat[1] = (SplashCoord)(curstate[1] );
    mat[2] = (SplashCoord)(curstate[2] );
    mat[3] = (SplashCoord)(curstate[3] );
    mat[4] = (SplashCoord)(curstate[4] );
    mat[5] = (SplashCoord)(curstate[5] );
    splashFont = getSplashFont(state, mat);
    if((uLen == 0  || ((u[0] == (Unicode)0 || u[0] < (Unicode)32) && uLen == 1 ))) {//&& globalParams->getApplyOCR())
        // as a first iteration for dictionnaries, placing a placeholder, which means creating a map based on the font-code mapping to unicode from : https://unicode.org/charts/PDF/U2B00.pdf
        GString *fontName = new GString();
        if(state->getFont()->getName()) { //AA : Here if fontName is NULL is problematic
            fontName = state->getFont()->getName()->copy();
            fontName = fontName->lowerCase();
        }
        GString *fontName_charcode = fontName->append(to_string(c).c_str());// for performance and simplicity only appending is done
        //if (globalParams->getPrintCommands()) {
            printf("ToBeOCRISEChar: x=%.2f y=%.2f c=%3d=0x%02x='%c' u=%3d fontName=%s \n",
                   (double) x, (double) y, c, c, c, u[0], fontName->getCString());
        //}
        uLen=1;
        isNonUnicodeGlyph = gTrue;
    }
    text->addChar(state, x, y, dx, dy, c, nBytes, u, uLen, splashFont, isNonUnicodeGlyph);
}

void DataGeneratorOutputDev::updateCTM(GfxState *state, double m11, double m12,
                                double m21, double m22,
                                double m31, double m32) {
    for (int i=0;i<6;++i){
        curstate[i]=state->getCTM()[i];
    }
}

//------------------------------------------------------------------------
// T3FontCache
//------------------------------------------------------------------------

struct T3FontCacheTag {
    Gushort code;
    Gushort mru;			// valid bit (0x8000) and MRU index
};

class T3FontCache {
public:

    T3FontCache(Ref *fontID, double m11A, double m12A,
                double m21A, double m22A,
                int glyphXA, int glyphYA, int glyphWA, int glyphHA,
                GBool validBBoxA, GBool aa);
    ~T3FontCache();
    GBool matches(Ref *idA, double m11A, double m12A,
                  double m21A, double m22A)
    { return fontID.num == idA->num && fontID.gen == idA->gen &&
             m11 == m11A && m12 == m12A && m21 == m21A && m22 == m22A; }

    Ref fontID;			// PDF font ID
    double m11, m12, m21, m22;	// transform matrix
    int glyphX, glyphY;		// pixel offset of glyph bitmaps
    int glyphW, glyphH;		// size of glyph bitmaps, in pixels
    GBool validBBox;		// false if the bbox was [0 0 0 0]
    int glyphSize;		// size of glyph bitmaps, in bytes
    int cacheSets;		// number of sets in cache
    int cacheAssoc;		// cache associativity (glyphs per set)
    Guchar *cacheData;		// glyph pixmap cache
    T3FontCacheTag *cacheTags;	// cache tags, i.e., char codes
};

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
         cacheSets >>= 1) ;
    cacheData = (Guchar *)gmallocn(cacheSets * cacheAssoc, glyphSize);
    cacheTags = (T3FontCacheTag *)gmallocn(cacheSets * cacheAssoc,
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
    Gushort code;			// character code

    GBool haveDx;			// set after seeing a d0/d1 operator
    GBool doNotCache;		// set if we see a gsave/grestore before
    //   the d0/d1

    //----- cache info
    T3FontCache *cache;		// font cache for the current font
    T3FontCacheTag *cacheTag;	// pointer to cache tag for the glyph
    Guchar *cacheData;		// pointer to cache data for the glyph

    //----- saved state
    SplashBitmap *origBitmap;
    Splash *origSplash;
    double origCTM4, origCTM5;

    T3GlyphStack *next;		// next object on stack
};

void TextPage::setupPNG(png_structp *png, png_infop *pngInfo, FILE *f,
                     int bitDepth, int colorType, double res,
                     SplashBitmap *bitmap) {
    png_color_16 background;
    int pixelsPerMeter;

    if (!(*png = png_create_write_struct(PNG_LIBPNG_VER_STRING,
                                         NULL, NULL, NULL)) ||
        !(*pngInfo = png_create_info_struct(*png))) {
        exit(2);
    }
    if (setjmp(png_jmpbuf(*png))) {
        exit(2);
    }
    png_init_io(*png, f);
    png_set_IHDR(*png, *pngInfo, bitmap->getWidth(), bitmap->getHeight(),
                 bitDepth, colorType, PNG_INTERLACE_NONE,
                 PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
    if (colorType == PNG_COLOR_TYPE_GRAY_ALPHA ||
        colorType == PNG_COLOR_TYPE_RGB_ALPHA) {
        background.index = 0;
        background.red = 0xff;
        background.green = 0xff;
        background.blue = 0xff;
        background.gray = 0xff;
        png_set_bKGD(*png, *pngInfo, &background);
    }
    pixelsPerMeter = (int)(res * (1000 / 25.4) + 0.5);
    png_set_pHYs(*png, *pngInfo, pixelsPerMeter, pixelsPerMeter,
                 PNG_RESOLUTION_METER);
    png_write_info(*png, *pngInfo);
}

void TextPage::writePNGData(png_structp png, SplashBitmap *bitmap) {
    Guchar *p, *alpha, *rowBuf, *rowBufPtr;
    int y, x;

    if (setjmp(png_jmpbuf(png))) {
        exit(2);
    }
    p = bitmap->getDataPtr();
    if (gFalse) {
        alpha = bitmap->getAlphaPtr();
        if (bitmap->getMode() == splashModeMono8) {
            rowBuf = (Guchar *)gmallocn(bitmap->getWidth(), 2);
            for (y = 0; y < bitmap->getHeight(); ++y) {
                rowBufPtr = rowBuf;
                for (x = 0; x < bitmap->getWidth(); ++x) {
                    *rowBufPtr++ = *p++;
                    *rowBufPtr++ = *alpha++;
                }
                png_write_row(png, (png_bytep)rowBuf);
            }
            gfree(rowBuf);
        } else { // splashModeRGB8
            rowBuf = (Guchar *)gmallocn(bitmap->getWidth(), 4);
            for (y = 0; y < bitmap->getHeight(); ++y) {
                rowBufPtr = rowBuf;
                for (x = 0; x < bitmap->getWidth(); ++x) {
                    *rowBufPtr++ = *p++;
                    *rowBufPtr++ = *p++;
                    *rowBufPtr++ = *p++;
                    *rowBufPtr++ = *alpha++;
                }
                png_write_row(png, (png_bytep)rowBuf);
            }
            gfree(rowBuf);
        }
    } else {
        for (y = 0; y < bitmap->getHeight(); ++y) {
            png_write_row(png, (png_bytep)p);
            p += bitmap->getRowSize();
        }
    }
}



void TextPage::finishPNG(png_structp *png, png_infop *pngInfo) {
    if (setjmp(png_jmpbuf(*png))) {
        exit(2);
    }
    png_write_end(*png, *pngInfo);
    png_destroy_write_struct(png, pngInfo);
}


void TextPage::setupScreenParams(double hDPI, double vDPI) {
    screenParams.size = globalParams->getScreenSize();
    screenParams.dotRadius = globalParams->getScreenDotRadius();
    screenParams.gamma = (SplashCoord)globalParams->getScreenGamma();
    screenParams.blackThreshold =
            (SplashCoord)globalParams->getScreenBlackThreshold();
    screenParams.whiteThreshold =
            (SplashCoord)globalParams->getScreenWhiteThreshold();
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

void DataGeneratorOutputDev::stroke(GfxState *state) {
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

void DataGeneratorOutputDev::fill(GfxState *state) {
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

void DataGeneratorOutputDev::eoFill(GfxState *state) {
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

void DataGeneratorOutputDev::clip(GfxState *state) {
    if (parameters->getDisplayImage()) {
        text->clip(state);
    }
}

void DataGeneratorOutputDev::eoClip(GfxState *state) {
    text->eoClip(state);
}
void DataGeneratorOutputDev::clipToStrokePath(GfxState *state) {
    text->clipToStrokePath(state);
}

void DataGeneratorOutputDev::doPath(GfxPath *path, GfxState *state, GString *gattributes) {
    if (parameters->getDisplayImage()) {
        text->doPath(path, state, gattributes);
    }
}

void DataGeneratorOutputDev::saveState(GfxState *state) {
    text->saveState(state);
}

void DataGeneratorOutputDev::restoreState(GfxState *state) {
    needFontUpdate = gTrue;
    for (int i=0;i<6;++i){
        curstate[i]=state->getCTM()[i];
    }
    text->restoreState(state);
}

// Return the hexadecimal value of the color of string
GString *DataGeneratorOutputDev::colortoString(GfxRGB rgb) const {
    char* temp;
    temp = (char*)malloc(10*sizeof(char));
    sprintf(temp, "#%02X%02X%02X", static_cast<int>(255*colToDbl(rgb.r)),
            static_cast<int>(255*colToDbl(rgb.g)), static_cast<int>(255
                                                                    *colToDbl(rgb.b)));
    GString *tmp = new GString(temp);

    free(temp);

    return tmp;
}

GString *DataGeneratorOutputDev::convtoX(unsigned int xcol) const {
    GString *xret=new GString();
    char tmp;
    unsigned int k;
    k = (xcol/16);
    if ((k>0)&&(k<10))
        tmp=(char) ('0'+k);
    else
        tmp=(char)('a'+k-10);
    xret->append(tmp);
    k = (xcol%16);
    if ((k>0)&&(k<10))
        tmp=(char) ('0'+k);
    else
        tmp=(char)('a'+k-10);
    xret->append(tmp);
    return xret;
}

void DataGeneratorOutputDev::drawSoftMaskedImage(GfxState *state, Object *ref,
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


void DataGeneratorOutputDev::drawImage(GfxState *state, Object *ref, Stream *str,
                             int width, int height, GfxImageColorMap *colorMap, int *maskColors,
                             GBool inlineImg,
                                 GBool interpolate) {
}

void DataGeneratorOutputDev::drawImageMask(GfxState *state, Object *ref, Stream *str,
                                 int width, int height, GBool invert, GBool inlineImg,
                                     GBool interpolate) {

}

int DataGeneratorOutputDev::dumpFragment(Unicode *text, int len, UnicodeMap *uMap,
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

void DataGeneratorOutputDev::tilingPatternFill(GfxState *state, Gfx *gfx,
                                       Object *strRef,
                                       int paintType, int tilingType,
                                       Dict *resDict,
                                       double *mat, double *bbox,
                                       int x0, int y0, int x1, int y1,
                                       double xStep, double yStep) {
    // do nothing -- this avoids the potentially slow loop in Gfx.cc
}
