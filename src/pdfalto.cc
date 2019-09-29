#include <aconf.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include "parseargs.h"
#include "GString.h"
#include "gmem.h"
#include "GlobalParams.h"
#include "Object.h"
#include "Stream.h"
#include "Array.h"
#include "Dict.h"
#include "XRef.h"
#include "Catalog.h"
#include "Page.h"
#include "PDFDoc.h"
#include "XmlAltoOutputDev.h"
#include "CharTypes.h"
#include "UnicodeMap.h"
#include "Error.h"
#include "config.h"
#include "Parameters.h"
#include "Outline.h"

#include "PDFDocXrce.h"

#include <sys/types.h>
#include <dirent.h>

#include <iostream>

using namespace std;

#include "ConstantsUtils.h"

using namespace ConstantsUtils;

#include "ConstantsXML.h"

using namespace ConstantsXML;

#include "TextString.h"

void removeAlreadyExistingData(GString *dir);

static const char cvsid[] = "$Revision: 1.4 $";

static int firstPage = 1;
static int lastPage = 0;
static int filesCountLimit = 0;
static GBool physLayout = gTrue;
static GBool verbose = gFalse;

static char cfgFileName[256] = "";

static char XMLcfgFileName[256] = "";

static GBool noText = gFalse;
static GBool noImage = gFalse;
static GBool outline = gFalse;
static GBool cutPages = gFalse;
static GBool blocks = gFalse;
static GBool fullFontName = gFalse;
static GBool noImageInline = gFalse;
static GBool annots = gFalse;
static GBool readingOrder = gFalse;
static GBool charReadingOrderAttr = gFalse;
static GBool ocr = gFalse;

static char ownerPassword[33] = "\001";
static char userPassword[33] = "\001";
static GBool quiet = gFalse;
static GBool printVersion = gFalse;
static GBool printHelp = gFalse;
static char namespaceUri[256] = "\001";

static ArgDesc argDesc[] = {
        {"-f",             argInt,    &firstPage,       0,
                "first page to convert"},
        {"-l",             argInt,    &lastPage,        0,
                "last page to convert"},
        {"-verbose",       argFlag,   &verbose,         0,
                "display pdf attributes"},
        {"-noText",        argFlag,   &noText,          0,
                "do not extract textual objects"},
        {"-noImage",       argFlag,   &noImage,         0,
                "do not extract Images (Bitmap and Vectorial)"},
        {"-noImageInline", argFlag,   &noImageInline,   0,
                "do not include images inline in the stream"},
        {"-outline",       argFlag,   &outline,         0,
                "create an outline file xml"},
        {"-annotation",    argFlag,   &annots,          0,
                "create an annotations file xml"},
// PL: code for supporting cut pages need to be put back
//        {"-cutPages",      argFlag,   &cutPages,        0,
//                "cut all pages in separately files"},
        {"-blocks",        argFlag,   &blocks,          0,
                "add blocks informations within the structure"},
        {"-readingOrder",  argFlag,   &readingOrder,    0,
                "blocks follow the reading order"},
        {"-charReadingOrderAttr",  argFlag,   &charReadingOrderAttr,    0,
                "include TYPE attribute to String elements to indicate right-to-left reading order (not valid ALTO)"},
//        {"-ocr",           argFlag,   &ocr,             0,
//                "recognises all characters that are missing from unicode."},
        {"-fullFontName",  argFlag,   &fullFontName,    0,
                "fonts names are not normalized"},
        {"-nsURI",         argString, namespaceUri,     sizeof(namespaceUri),
                "add the specified namespace URI"},
        {"-opw",           argString, ownerPassword,    sizeof(ownerPassword),
                "owner password (for encrypted files)"},
        {"-upw",           argString, userPassword,     sizeof(userPassword),
                "user password (for encrypted files)"},
        {"-q",             argFlag,   &quiet,           0,
                "don't print any messages or errors"},
        {"-v",             argFlag,   &printVersion,    0,
                "print version info"},
        {"-h",             argFlag,   &printHelp,       0,
                "print usage information"},
        {"-help",          argFlag,   &printHelp,       0,
                "print usage information"},
        {"--help",         argFlag,   &printHelp,       0,
                "print usage information"},
        {"-?",             argFlag,   &printHelp,       0,
                "print usage information"},
        {"--saveconf",     argString, XMLcfgFileName,   sizeof(XMLcfgFileName),
                "save all command line parameters in the specified XML <file>"},
        {"-conf",          argString, cfgFileName,      sizeof(cfgFileName),
                "configuration file to use in place of xpdfrc"},

        {"-filesLimit",    argInt,    &filesCountLimit, 0,
                "limit of asset files be extracted"},
        {NULL}
};

/**
* Main method which execute pdfalto tool <br/>
*/
int main(int argc, char *argv[]) {
    PDFDocXrce *doc;

    GString *fileName;
    GString *textFileName;
    GString *dataDirName;
    GString *shortFileName;
    GString *annotationfile;
    GString *ownerPW, *userPW;
    GString *nsURI;
    GString *cmd;
    XmlAltoOutputDev *xmlAltoOut;
    GBool ok;
    char *p;
    int exitCode;
    char *temp;

    exitCode = 99;

    // parse args
    ok = parseArgs(argDesc, &argc, argv);
    if (XMLcfgFileName[0]) {}
    else {
        if (!ok || argc < 2 || argc > 3 || printVersion || printHelp) {
            fprintf(stderr, "%s", PDFALTO_NAME);
            fprintf(stderr, " version ");
            fprintf(stderr, "%s", PDFALTO_VERSION);
            fprintf(stderr, "\n");
            //fprintf(stderr, "(Based on Xpdf version %s, %s)\n", xpdfVersion, xpdfCopyright);
            if (!printVersion) {
                printUsage("pdfalto", "<PDF-file> [<xml-file>]", argDesc);
            }
            goto err0;
        }
    }
    //fileName = new GString(argv[1]);
    cmd = new GString();
    globalParams = new GlobalParams(cfgFileName);

    // Parameters specifics to pdfalto
    parameters = new Parameters();

    if (noImage) {
        parameters->setDisplayImage(gFalse);
        cmd->append("-noImage ");
    } else {
        parameters->setDisplayImage(gTrue);
    }

    if (noText) {
        parameters->setDisplayText(gFalse);
        cmd->append("-noText ");
    } else {
        parameters->setDisplayText(gTrue);
    }

    if (outline) {
        parameters->setDisplayOutline(gFalse);
        cmd->append("-outline ");
    } else {
        parameters->setDisplayOutline(gTrue);
    }

    if (cutPages) {
        parameters->setCutAllPages(gFalse);
        cmd->append("-cutPages ");
    } else {
        parameters->setCutAllPages(gTrue);
    }

    if (blocks) {
        parameters->setDisplayBlocks(gTrue);
        cmd->append("-blocks ");
    } else {
        parameters->setDisplayBlocks(gFalse);
    }

    if (readingOrder) {
        parameters->setReadingOrder(gTrue);
        cmd->append("-readingOrder ");
    }

    if (charReadingOrderAttr) {
        parameters->setCharReadingOrderAttr(gTrue);
        cmd->append("-charReadingOrderAttr ");
    }

    if (ocr) {
        parameters->setOcr(gTrue);
        cmd->append("-ocr ");
        //we avoid using heuristic mapping (not reliable)
        globalParams->setMapNumericCharNames(gFalse);
    } else
        globalParams->setMapNumericCharNames(gTrue);

    if (fullFontName) {
        parameters->setFullFontName(gTrue);
        cmd->append("-fullFontName ");
    } else {
        parameters->setFullFontName(gFalse);
    }

    if (noImageInline) {
        parameters->setImageInline(gTrue);
        cmd->append("-noImageInline ");
    } else {
        parameters->setImageInline(gFalse);
    }

    if (quiet) {
        globalParams->setErrQuiet(quiet);
    }

    if (verbose) {
        globalParams->setPrintCommands(gTrue);
        cmd->append("-verbose ");
    }

    // open PDF file
    if (ownerPassword[0] != '\001') {
        ownerPW = new GString(ownerPassword);
    } else {
        ownerPW = NULL;
    }
    if (userPassword[0] != '\001') {
        userPW = new GString(userPassword);
    } else {
        userPW = NULL;
    }

    if (filesCountLimit > 0) {
        parameters->setFilesCountLimit(filesCountLimit);
        cmd->append("--filesLimit ")->append(filesCountLimit)->append(" ");
    } else
        parameters->setFilesCountLimit(1000);//this is the previous default limit

    if (namespaceUri[0] != '\001') {
        nsURI = new GString(namespaceUri);
        cmd->append("-nsURI ")->append(nsURI)->append(" ");
    } else {
        nsURI = NULL;
    }

    //store paramneters in a given XML file
    if (XMLcfgFileName[0]) {
        parameters->saveToXML(XMLcfgFileName, firstPage, lastPage);
//   	goto err0;
    }

    if (argc < 2) { goto err0; }
    fileName = new GString(argv[1]);
    // Create the object PDF doc
    doc = new PDFDocXrce(fileName, ownerPW, userPW);

    if (userPW) {
        delete userPW;
    }
    if (ownerPW) {
        delete ownerPW;
    }
    if (!doc->isOk()) {
        exitCode = 1;
        goto err2;
    }

    if (!doc->okToCopy())
        fprintf(stderr, "\n\nYou are not supposed to copy this document...\n\n");

    // IF output XML file name was given in command line
    if (argc == 3) {
        textFileName = new GString(argv[2]);
        temp = textFileName->getCString() + textFileName->getLength() - 4;
        if (!strcmp(temp, EXTENSION_XML) || !strcmp(temp, EXTENSION_XML_MAJ)) {
            shortFileName = new GString(textFileName->getCString(), textFileName->getLength() - 4);
        } else {
            shortFileName = new GString(textFileName);
        }
    }
        // ELSE we build the output XML file name with the PDF file name
    else {
        p = fileName->getCString() + fileName->getLength() - 4;
        if (!strcmp(p, EXTENSION_PDF) || !strcmp(p, EXTENSION_PDF_MAJ)) {
            textFileName = new GString(fileName->getCString(), fileName->getLength() - 4);
            shortFileName = new GString(textFileName);

        } else {
            textFileName = fileName->copy();
            shortFileName = new GString(textFileName);
        }
        textFileName->append(EXTENSION_XML);
    }

    // For the annotations XML file
    if (annots) {
        annotationfile = new GString(shortFileName);
        annotationfile->append("_");
        annotationfile->append(NAME_ANNOT);
        annotationfile->append(EXTENSION_XML);
        cmd->append("-annotation ");
    }

    // Get page range
    if (firstPage < 1) {
        firstPage = 1;
    }
    if (firstPage != 1) {
        char *temp = (char *) malloc(10 * sizeof(char));
        sprintf(temp, "%d", firstPage);
        cmd->append("-f ")->append(temp)->append(" ");
        free(temp);
    }

    if (lastPage != 0) {
        int last = lastPage;
        if (lastPage > doc->getNumPages()) {
            last = doc->getNumPages();
        }
        char *temp = (char *) malloc(10 * sizeof(char));
        sprintf(temp, "%d", last);
        cmd->append("-l ")->append(temp)->append(" ");
        free(temp);
    }
    if (lastPage < 1 || lastPage > doc->getNumPages()) {
        lastPage = doc->getNumPages();
    }

    // Write xml file
//  xmlOut = new XmlOutputDev(textFileName, fileName, physLayout, verbose, nsURI, cmd);

//    printf("crop width: %g\n",doc->getPageCropWidth(1));
    xmlAltoOut = new XmlAltoOutputDev(textFileName, fileName, doc->getCatalog(), physLayout, verbose, nsURI, cmd);

    xmlAltoOut->initMetadataInfoDoc();
    xmlAltoOut->addMetadataInfo(doc);
    xmlAltoOut->closeMetadataInfoDoc(shortFileName);


    if (xmlAltoOut->isOk()) {

        // We clean the data directory if it is already exist
        dataDirName = new GString(textFileName);
        dataDirName->append(NAME_DATA_DIR);
        removeAlreadyExistingData(dataDirName);

        // Xml file to store annotations informations
        if (annots) {
            xmlDocPtr docAnnotXml;
            xmlNodePtr docroot;
            docAnnotXml = xmlNewDoc((const xmlChar *) VERSION);
            docAnnotXml->encoding = xmlStrdup((const xmlChar *) ENCODING_UTF8);
            docroot = xmlNewNode(NULL, (const xmlChar *) TAG_ANNOTATIONS);
            xmlDocSetRootElement(docAnnotXml, docroot);

            doc->displayPages(xmlAltoOut, docroot, firstPage, lastPage, 72, 72, 0, gFalse, gTrue, gFalse);


            xmlSaveFile(annotationfile->getCString(), docAnnotXml);
            xmlFreeDoc(docAnnotXml);
        } else {
            doc->displayPages(xmlAltoOut, NULL, firstPage, lastPage, 72, 72, 0, gFalse, gTrue, gFalse);
        }
        if (outline) {
            if (doc->getOutline()) {
                xmlAltoOut->initOutline(doc->getNumPages());
                xmlAltoOut->generateOutline(doc->getOutline()->getItems(), doc, 0);
                xmlAltoOut->closeOutline(shortFileName);
            }
        }
        xmlAltoOut->addStyles();
    } else {
        delete xmlAltoOut;
        exitCode = 2;
        goto err3;
    }
    delete xmlAltoOut;
    exitCode = 0;
    // clean up

    if (nsURI) {
        delete nsURI;
    }

    err3:
    delete textFileName;

    err2:
    delete doc;
    delete globalParams;
    delete parameters;
    delete cmd;

    err0:
    // check for memory leaks
    Object::memCheck(stderr);
    gMemReport(stderr);
    return exitCode;
}

/** Remove all files which are in data directory of file pdf if it is already exist
 * @param dir The directory name where we remove all data */
void removeAlreadyExistingData(GString *dir) {
    GString *file;
    struct dirent *lecture;
    DIR *rep;
    rep = opendir(dir->getCString());
    if (rep != NULL) {
        while ((lecture = readdir(rep))) {
            file = new GString(dir);
            file->append("/");
            file->append(lecture->d_name);
            remove(file->getCString());
        }
        if (file) delete file;
        closedir(rep);
    }
}