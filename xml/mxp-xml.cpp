/*
 * mxp-xml.cpp
 *   Mxp xml
 *   ____            _
 *  |  _ \ ___  __ _| |_ __ ___  ___
 *  | |_) / _ \/ _` | | '_ ` _ \/ __|
 *  |  _ <  __/ (_| | | | | | | \__ \
 *  |_| \_\___|\__,_|_|_| |_| |_|___/
 *
 * Permission to use, modify and distribute is granted via the
 *  GNU Affero General Public License v3 or later
 *
 *  Copyright (C) 2007-2020 Jason Mitchell, Randi Mitchell
 *     Contributions by Tim Callahan, Jonathan Hseu
 *  Based on Mordor (C) Brooke Paul, Brett J. Vickers, John P. Freeman
 *
 */


#include <libxml/parser.h>  // for xmlFreeDoc, xmlNode, xmlDocGetRootElement
#include <cstdio>           // for sprintf
#include <map>              // for operator==
#include <utility>          // for pair

#include "config.hpp"       // for Config, stringMap, MxpElementMap
#include "mxp.hpp"          // for MxpElement
#include "paths.hpp"        // for Code
#include "xml.hpp"          // for copyToString, NODE_NAME, copyToBool, loa...

//*********************************************************************
//                      loadMxpElements
//*********************************************************************

bool Config::loadMxpElements() {
    xmlDocPtr   xmlDoc;

    xmlNodePtr  curNode;
    char        filename[80];

    // build an XML tree from a the file
    sprintf(filename, "%s/mxp.xml", Path::Code);

    xmlDoc = xml::loadFile(filename, "Mxp");
    if(xmlDoc == nullptr)
        return(false);

    curNode = xmlDocGetRootElement(xmlDoc);

    curNode = curNode->children;
    while(curNode && xmlIsBlankNode(curNode)) {
        curNode = curNode->next;
    }
    if(curNode == nullptr) {
        xmlFreeDoc(xmlDoc);
        xmlCleanupParser();
        return(false);
    }

    clearMxpElements();
    while(curNode != nullptr) {
        if(NODE_NAME(curNode, "MxpElement")) {
            auto* mxpElement = new MxpElement(curNode);
            mxpElements.insert(std::pair<std::string, MxpElement*>(mxpElement->getName(), mxpElement));
            if(mxpElement->isColor()) {
                mxpColors.insert(std::pair<std::string, std::string>(mxpElement->getColor(), mxpElement->getName()));
            }
        }
        curNode = curNode->next;
    }
    xmlFreeDoc(xmlDoc);
    xmlCleanupParser();
    return(true);
}
//std::string name;
//MxpType mxpType;
//std::string command;
//std::string hint;
//bool prompt;
//std::string attributes;
//std::string expire;
MxpElement::MxpElement(xmlNodePtr rootNode) {
    rootNode = rootNode->children;
    prompt = false;
    while(rootNode != nullptr)
    {
        if(NODE_NAME(rootNode, "Name")) xml::copyToString(name, rootNode);
        else if(NODE_NAME(rootNode, "Command")) xml::copyToString(command, rootNode);
        else if(NODE_NAME(rootNode, "Hint")) xml::copyToString(hint, rootNode);
        else if(NODE_NAME(rootNode, "Type")) xml::copyToString(mxpType, rootNode);
        else if(NODE_NAME(rootNode, "Prompt")) xml::copyToBool(prompt, rootNode);
        else if(NODE_NAME(rootNode, "Attributes")) xml::copyToString(attributes, rootNode);
        else if(NODE_NAME(rootNode, "Expire")) xml::copyToString(expire, rootNode);
        else if(NODE_NAME(rootNode, "Color")) xml::copyToString(color, rootNode);

        rootNode = rootNode->next;
    }
}
