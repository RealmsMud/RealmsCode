/*
 * xml.cpp
 *   Various XML related functions used elsewhere in the mud
 *   ____            _
 *  |  _ \ ___  __ _| |_ __ ___  ___
 *  | |_) / _ \/ _` | | '_ ` _ \/ __|
 *  |  _ <  __/ (_| | | | | | | \__ \
 *  |_| \_\___|\__,_|_|_| |_| |_|___/
 *
 * Permission to use, modify and distribute is granted via the
 *  GNU Affero General Public License v3 or later
 *
 *  Copyright (C) 2007-2016 Jason Mitchell, Randi Mitchell
 *     Contributions by Tim Callahan, Jonathan Hseu
 *  Based on Mordor (C) Brooke Paul, Brett J. Vickers, John P. Freeman
 *
 */

// Mud includes
#include "mud.h"
#include "xml.h"

namespace xml {

    // doStrCpy
    //  Copys src to dest and then free's src
    //   Returns a pointer to dest
    // unXSC: yes
    char *doStrCpy(char *dest, char *src) {
        if(!src || !dest)
            return(0);
        strcpy(dest, unxsc(src).c_str());
        free(src);
        return(dest);
    }

    // unXSC: no
    char *doStrDup(char *src) {
        if(!src)
            return(0);
        char *tmp = strdup(src);
        free(src);
        return(tmp);
    }

    // XSC: yes
    xmlAttrPtr newProp(xmlNodePtr node, bstring name, const bstring value) {
        xmlAttrPtr toReturn;
        xmlChar* xmlTmp;
        xmlTmp = xmlEncodeSpecialChars((node)->doc, BAD_CAST (xsc(value).c_str()) );
        toReturn = xmlNewProp( (node), BAD_CAST (name.c_str()), xmlTmp );
        free(xmlTmp);

        return(toReturn);
    }


    xmlNodePtr newBoolChild(xmlNodePtr node, const bstring name, const bool value) {
        return(xmlNewChild( node, nullptr, BAD_CAST (name.c_str()), BAD_CAST iToYesNo(value)));
    }

    // XSC: yes
    xmlNodePtr newStringChild(xmlNodePtr node, const bstring name, const bstring value) {
        return(xmlNewTextChild( node, nullptr, BAD_CAST (name.c_str()), BAD_CAST (xsc(value).c_str()) ));
    }

    // XSC: yes
    xmlNodePtr saveNonNullString(xmlNodePtr node, bstring name, const bstring value) {
        if(value == "")
            return(nullptr);
        return(xmlNewTextChild( (node), nullptr, BAD_CAST (name.c_str()), BAD_CAST (xsc(value).c_str()) ));
    }

    // unXSC: yes
    void copyToBString(bstring& to, xmlNodePtr node) {
        char* xTemp = getCString(node);
        if(xTemp) {
            to = unxsc(xTemp);
            free (xTemp);
        }
    }

    // unXSC: yes
    void copyPropToBString(bstring& to, xmlNodePtr node, bstring name) {
        char* xTemp = (char *)xmlGetProp(node, BAD_CAST(name.c_str()));
        if(xTemp) {
            to = unxsc(xTemp);
            free (xTemp);
        }
    }

    // unXSC: yes
    bstring getBString(xmlNodePtr node) {
        bstring toReturn;
        char* xTemp = getCString(node);
        if(xTemp) {
            toReturn = unxsc(xTemp);
            free (xTemp);
        }
        return(toReturn);
    }

    // unXSC: yes
    void copyToCString(char* to, xmlNodePtr node) {
        to = doStrCpy( to, getCString( node ));
    }

    //#define copyToCString(to, node)   doStrCpy( (to), getCString( (node) ))

    void copyToBool(bool& to, xmlNodePtr node) {
        to = toBoolean(getCString( (node) ));
    }
    void copyToColor(Color& to, xmlNodePtr node) {
        to = toColor(getCString( (node) ));
    }

    // unXSC: no
    char* getCString(xmlNodePtr node) {
        return((char *)xmlNodeListGetString((node)->doc, (node)->children , 1));
    }

    // unXSC: yes
    bstring getProp(xmlNodePtr node, const char *name) {
        bstring toReturn;
        char* xTemp = (char *)xmlGetProp( (node) , BAD_CAST (name) );
        if(xTemp) {
            toReturn = unxsc(xTemp);
            free (xTemp);
        }
        return(toReturn);
    }

    // unXSC: yes
    void copyPropToCString(char* to, xmlNodePtr node, bstring name) {
        strcpy(to, getProp(node, name.c_str()).c_str());
    }

    //#define copyPropToCString(to, node, name) )

    int getIntProp(xmlNodePtr node, const char *name) {
        return(toNum<int>((char *)xmlGetProp( (node) , BAD_CAST (name) )));
    }

    //#define getIntProp(node, name)    )
    xmlDocPtr loadFile(const char *filename, const char *expectedRoot) {
        xmlDocPtr doc;
        xmlNodePtr cur;

        doc = xmlReadFile(filename, nullptr, XML_PARSE_NOERROR|XML_PARSE_NOWARNING|XML_PARSE_NOBLANKS );

        if(doc == nullptr)
            return(nullptr);

        // Check the document is of the right kind
        cur = xmlDocGetRootElement(doc);
        if(cur == nullptr) {
            loge("%s_Load: empty document\n", expectedRoot);
            xmlFreeDoc(doc);
            return(nullptr);
        }
        if(strcmp((char*)cur->name, expectedRoot)) {
            loge("%s_Load: document of the wrong type: Got: [%s] Expected: [%s]\n", expectedRoot, cur->name, expectedRoot);
            xmlFreeDoc(doc);
            return(nullptr);
        }

        return(doc);
    }

    int saveFile(const char * filename, xmlDocPtr cur) {
        //xmlSaveFile(filename, cur);
        return(xmlSaveFormatFile(filename, cur, 1));
    }

} // End xml namespace

/* toBoolean & toInt & toLong
 *  Used to avoid memory leaks:
 *   Accepts a string xmlNodeListGetString, converts it to an numer, frees
 *   the memory and returns the number
 */
// Returns 1 if true, otherwise false and then frees fromStr
int toBoolean(char *fromStr) {
    int toReturn = 0; // Defaults to false
    if(!fromStr)
        return(toReturn);
    if(!strcasecmp(fromStr, "yes") || !strcasecmp(fromStr, "on") || !strcmp(fromStr, "1") ||
            !strcasecmp(fromStr, "true") || !strcasecmp(fromStr, "enabled"))
        toReturn = 1;
    free(fromStr);
    return(toReturn);
}

// Returns an int representing the color in fromStr and then frees fromStr
Color toColor(char *fromStr) {
    int toReturn = 0;
    if(!fromStr)
        return(NOCOLOR);

    if(strstr(fromStr, "Bold"))
        toReturn |= BOLD;
    if(strstr(fromStr, "Blink"))
        toReturn |= BLINK;
    if(strstr(fromStr, "Normal"))
        toReturn |= NORMAL;

    if(strstr(fromStr, "Red"))
        toReturn |= RED;
    else if(strstr(fromStr, "Green"))
        toReturn |= GREEN;
    else if(strstr(fromStr, "Yellow"))
        toReturn |= YELLOW;
    else if(strstr(fromStr, "Blue"))
        toReturn |= BLUE;
    else if(strstr(fromStr, "Magenta"))
        toReturn |= MAGENTA;
    else if(strstr(fromStr, "Cyan"))
        toReturn |= CYAN;
    else //if(strstr(fromStr, "White"))
        toReturn |= WHITE;

    //  if(toReturn == 0)
    //      toReturn |= WHITE);

    free(fromStr);
    return((Color)toReturn);
}
// Converts an int to a yes or no
char *iToYesNo(int fromInt) {
    static char toReturn[8];

    if(fromInt == 0)
        sprintf(toReturn, "No");
    else
        sprintf(toReturn, "Yes");
    return(toReturn);
}
char *colorToStr(int fromColor) {
    static char toReturn[128];
    strcpy(toReturn, "");

    if(fromColor & BOLD)
        strcat(toReturn, "Bold");
    if(fromColor & BLINK)
        strcat(toReturn, "Blink");
    if(fromColor & NORMAL)
        strcat(toReturn, "Normal");

    if(fromColor & RED)
        strcat(toReturn, "Red");
    else if(fromColor & GREEN)
        strcat(toReturn, "Green");
    else if(fromColor & YELLOW)
        strcat(toReturn, "Yellow");
    else if(fromColor & BLUE)
        strcat(toReturn, "Blue");
    else if(fromColor & MAGENTA)
        strcat(toReturn, "Magenta");
    else if(fromColor & CYAN)
        strcat(toReturn, "Cyan");
    else // Default to White
        strcat(toReturn, "White");

    return(toReturn);
}

