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
#include "DataGeneratorOutputDev.h"
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

#include "ConstantsXMLALTO.h"

using namespace ConstantsXMLALTO;

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

static char ownerPassword[33] = "\001";
static char userPassword[33] = "\001";
static GBool quiet = gFalse;
static GBool printVersion = gFalse;
static GBool printHelp = gFalse;
static char namespaceUri[256] = "\001";

static ArgDesc argDesc[] = {
        {"-f",       argInt,      &firstPage,     0,
                "first page to convert"},
        {"-l",       argInt,      &lastPage,      0,
                "last page to convert"},
        {"-verbose",     argFlag,     &verbose,      0,
                "display pdf attributes"},
        {"-readingOrder", argFlag,     &readingOrder,  0,
                "blocks follow the reading order"},
        {"-fullFontName", argFlag,     &fullFontName,  0,
                "fonts names are not normalized"},
        {"-opw",     argString,   ownerPassword,  sizeof(ownerPassword),
                "owner password (for encrypted files)"},
        {"-upw",     argString,   userPassword,   sizeof(userPassword),
                "user password (for encrypted files)"},
        {"-q",       argFlag,     &quiet,         0,
                "don't print any messages or errors"},
        {"-v",       argFlag,     &printVersion,  0,
                "print version info"},
        {"-h",       argFlag,     &printHelp,     0,
                "print usage information"},
        {"-help",    argFlag,     &printHelp,     0,
                "print usage information"},
        {"--help",   argFlag,     &printHelp,     0,
                "print usage information"},
        {"-?",       argFlag,     &printHelp,     0,
                "print usage information"},
        {"-conf",        argString,      cfgFileName,    sizeof(cfgFileName),
                "configuration file to use in place of xpdfrc"},

        {NULL}
};

/**
* -verbose : display pdf attributes<br/>
* @date 11-2018
* @param argc The arguments number
* @param argv[] The arguments list that the user configured
* @author Achraf Azhar
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
    DataGeneratorOutputDev *datagen;
    GBool ok;
    char *p;
    int exitCode;
    char *temp;

    exitCode = 99;

    // parse args
    ok = parseArgs(argDesc, &argc, argv);
    if (XMLcfgFileName[0]){}
    else{
        if (!ok || argc < 2 || argc > 3 || printVersion || printHelp) {
            fprintf(stderr, "%s", PDFALTO_NAME);
            fprintf(stderr, " version ");
            fprintf(stderr, "%s", PDFALTO_VERSION);
            fprintf(stderr, "\n");
            fprintf(stderr, "(Based on Xpdf version %s, %s)\n", xpdfVersion, xpdfCopyright);
//            if (!printVersion) {
//                printUsage("pdfalto", "<PDF-file> [<xml-file>]", argDesc);
//            }
            goto err0;
        }
    }
//  fileName = new GString(argv[1]);
    cmd = new GString();
    globalParams = new GlobalParams(cfgFileName);

    // Parameters specifics to pdfalto
    parameters = new Parameters();


//    if(readingOrder){
        parameters->setReadingOrder(gTrue);
        cmd->append("-readingOrder ");
//    }


    parameters->setDisplayText(gTrue);
    parameters->setCutAllPages(gTrue);
    if(fullFontName){
        parameters->setFullFontName(gTrue);
        cmd->append("-fullFontName ");
    }
    else{
        parameters->setFullFontName(gFalse);
    }

    if (quiet) {
        globalParams->setErrQuiet(quiet);
    }

    if (verbose){
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


    if (argc < 2) {goto err0;}
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

    // Get page range
    if (firstPage < 1) {
        firstPage = 1;
    }

    if (lastPage!=0){
        int last = lastPage;
        if (lastPage > doc->getNumPages()){
            last = doc->getNumPages();
        }
    }
    if (lastPage < 1 || lastPage > doc->getNumPages()) {
        lastPage = doc->getNumPages();
    }

    datagen = new DataGeneratorOutputDev(textFileName, fileName, doc->getCatalog(), physLayout, verbose, nsURI, cmd);


    if (datagen->isOk()) {
        clock_t startTime = clock();

        datagen->startDoc(doc->getXRef());

        doc->displayPages(datagen, NULL, firstPage, lastPage, 400, 400, 0, gFalse, gTrue, gFalse);

        cout << double( clock() - startTime ) / (double)CLOCKS_PER_SEC<< " seconds." << endl;
    }
    else {
        delete datagen;
        exitCode = 2;
        goto err3;
    }
    delete datagen;
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