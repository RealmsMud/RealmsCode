/*
 * Mdsp.h
 *   Stuff to deal with MDSP
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

// C Includes
#include <arpa/telnet.h>

// Mud Includes
#include "config.h"
#include "mud.h"
#include "msdp.h"
#include "login.h"
#include "server.h"
#include "socket.h"
#include "xml.h"

#define MSDP_DEBUG

void Server::processMsdp(void) {
    for(Socket *sock : sockets) {
        if(sock->getState() == CON_DISCONNECTING)
            continue;
        if(sock->getMccp() || sock->getAtcp()) {
            for(std::pair<bstring, ReportedMsdpVariable*> p : sock->msdpReporting) {
                ReportedMsdpVariable* var = p.second;
                if(var->getRequiresPlayer() && (!sock->getPlayer() || sock->getState() != CON_PLAYING)) continue;
                if(!var->checkTimer()) continue;
                var->update();
                if(!var->isDirty()) continue;
                sock->msdpSend(var);
                var->setDirty(false);
            }
        }
    }
}

bool Socket::processMsdpVarVal(bstring& variable, bstring& value) {
#ifdef MSDP_DEBUG
    std::cout << "Found Var: '" << variable << "' Val: '" << value << "'"
            << std::endl;
#endif
    if (variable.equals("LIST")) {
        return (msdpList(value));
    } else if (variable.equals("REPORT")) {
        return (msdpReport(value));
    } else if (variable.equals("UNREPORT")) {
        return (msdpUnReport(value));
    } else if (variable.equals("SEND")) {
        return (msdpSend(value));
    } else {
        // See if they've sent us a configurable variable, if so set it
        for(std::pair<bstring, MsdpVariable*> p : gConfig->msdpVariables ) {
            MsdpVariable* msdpVar = p.second;
            if(msdpVar->isConfigurable()) {
                if(!isReporting(variable)) {
                    // If we're not reporting, start reporting it
                    msdpReport(variable);
                }
                ReportedMsdpVariable* reportedVar = getReportedMsdpVariable(variable);

                // Should never happen since we just added it
                if(!reportedVar)
                    return(false);
                // If it's a write once variable, we can only set it if the value is currently unknown
                if(msdpVar->isWriteOnce() && reportedVar->getValue() != "unknown")
                    return(false);
                reportedVar->setValue(value);
#ifdef MSDP_DEBUG
                std::cout << "processMsdpVarVal: Set configurable variable '" << variable << "' to '" << value << "'" << std::endl;
#endif
                return(true);
            }
        }
#ifdef MSDP_DEBUG
        std::cout << "processMsdpVarVal: Unknown variable '" << variable << "'"
                << std::endl;
#endif
    }
    return (true);
}
bool Socket::msdpList(bstring& value) {
    if (value.equals("COMMANDS")) {
        const char MsdpCommandList[] = "LIST REPORT RESET SEND UNREPORT";
        msdpSendList(value, MsdpCommandList);
        return (true);
    } else if (value.equals("LISTS")) {
        const char MsdpLists[] =
                "COMMANDS LISTS CONFIGURABLE_VARIABLES REPORTABLE_VARIABLES REPORTED_VARIABLES SENDABLE_VARIABLES";
        msdpSendList(value, MsdpLists);
        return (true);
    } else if (value.equals("SENDABLE_VARIABLES")) {
        std::ostringstream oStr;
        for (std::pair<bstring, MsdpVariable*> p : gConfig->msdpVariables) {
            oStr << " " << p.second->getName();
        }
        msdpSendList(value, bstring(oStr.str()).trim());
        return (true);
    } else if (value.equals("REPORTABLE_VARIABLES")) {
        std::ostringstream oStr;
        for (std::pair<bstring, MsdpVariable*> p : gConfig->msdpVariables) {
            if(p.second->isReportable())
                oStr << " " << p.second->getName();
        }
        msdpSendList(value, bstring(oStr.str()).trim());
        return (true);
    } else if (value.equals("CONFIGURABLE_VARIABLES")) {
        std::ostringstream oStr;
        for (std::pair<bstring, MsdpVariable*> p : gConfig->msdpVariables) {
            if (p.second->isConfigurable())
                oStr << " " << p.second->getName();
        }
        msdpSendList(value, bstring(oStr.str()).trim());
        return (true);
    } else if (value.equals("REPORTED_VARIABLES")) {
        std::ostringstream oStr;
        for (std::pair<bstring, ReportedMsdpVariable*> p : msdpReporting) {
            oStr << " " << p.second->getName();
        }
        msdpSendList("REPORTED_VARIABLES", bstring(oStr.str()).trim());
        return (true);
    } else if (value.equals("")) {
        // If we just get a LIST command, send off a list of all variables
        std::ostringstream oStr;
        for (std::pair<bstring, MsdpVariable*> p : gConfig->msdpVariables) {
            oStr << " " << p.second->getName();
        }
        msdpSendList("SENDABLE_VARIABLES", bstring(oStr.str()).trim());
        return (true);
    }

    return (false);
}
ReportedMsdpVariable* Socket::getReportedMsdpVariable(bstring& value) {
    std::map<bstring, ReportedMsdpVariable*>::iterator it = msdpReporting.find(value);

    if (it == msdpReporting.end())
        return (nullptr);
    else
        return (it->second);
}
bool Socket::isReporting(bstring& value) {
    std::map<bstring, ReportedMsdpVariable*>::iterator it = msdpReporting.find(value);

    if (it == msdpReporting.end())
        return (false);
    else
        return (true);
}

bool Socket::msdpReport(bstring& value) {
    MsdpVariable* msdpVar = gConfig->getMsdpVariable(value);
    if (!msdpVar || msdpVar->getName() != value || !msdpVar->isReportable())
        return (false);

    if (isReporting(value)) {
#ifdef MSDP_DEBUG
        std::cout << "MsdpHandleReport: Already Reporting '" << value << "'"
                << std::endl;
#endif
        return (true);
    }

    msdpReporting[msdpVar->getName()] = new ReportedMsdpVariable(msdpVar, this);
#ifdef MSDP_DEBUG
    std::cout << "MsdpHandleReport: Now Reporting '" << msdpVar->getName()
            << "'" << std::endl;
#endif
    return (true);
}
bool Socket::msdpReset(bstring& value) {
    if (value.equals("REPORTABLE_VARIABLES")
            || value.equals("SENDABLE_VARIABLES")) {
        // For now just clear all reported variables
        msdpClearReporting();
    }
    return (false);
}
void Socket::msdpClearReporting() {
    for (std::pair<bstring, ReportedMsdpVariable*> p : msdpReporting) {
        delete (p.second);
    }
    msdpReporting.clear();
}
bool Socket::msdpSend(bstring value) {
    MsdpVariable *msdpVar = gConfig->getMsdpVariable(value);
    if(msdpVar == nullptr) {
#ifdef MSDP_DEBUG
        std::cout << "Unknown variable to send: '" << value << "'" << std::endl;
#endif
        return (false);
    }
    if(msdpVar->hasSendScript()) {
        gServer->runPython(msdpVar->getSendScript(), "", this, getPlayer(), msdpVar);
        return(true);
    } else {
        return(msdpSend(getReportedMsdpVariable(value)));
    }
}
bool Socket::msdpSend(ReportedMsdpVariable* reportedVar) {
    if(!reportedVar || (reportedVar->getRequiresPlayer() && !getPlayer())) {
        msdpSendPair(reportedVar->getName(), "N/A");
    } else {
        // Send scripts will trump locally set value
        if(reportedVar->hasSendScript())
            gServer->runPython(reportedVar->getSendScript(), "", this, getPlayer(), reportedVar);
        else {
            msdpSendPair(reportedVar->getName(), reportedVar->getValue());
        }
    }
    return(true);
}
bool Socket::msdpUnReport(bstring& value) {
    std::map<bstring, ReportedMsdpVariable*>::iterator it = msdpReporting.find(value);

    if (it == msdpReporting.end())
        return (false);
    else {
#ifdef MSDP_DEBUG
        std::cout << "MsdpHandleUnReport: No longer reporting '" << value << "'" << std::endl;
#endif
        delete it->second;
        msdpReporting.erase(it);
        return (true);
    }
}
void Socket::msdpSendList(bstring variable, bstring value) {
    std::ostringstream oStr;

    if (getMsdp()) {
        value.Replace(' ', (unsigned char) MSDP_VAL);
        oStr
                << (unsigned char) IAC << (unsigned char) SB << (unsigned char) TELOPT_MSDP
                << (unsigned char) MSDP_VAR << variable << (unsigned char) MSDP_VAL
                << (unsigned char) MSDP_ARRAY_OPEN << (unsigned char) MSDP_VAL << value
                << (unsigned char) MSDP_ARRAY_CLOSE << (unsigned char) IAC << (unsigned char) SE;
    } else if (getAtcp()) {
        oStr
                << (unsigned char) IAC << (unsigned char) SB << (unsigned char) TELOPT_ATCP
                << "MSDP." << variable << " " << value
                << (unsigned char) IAC << (unsigned char) SE;
    }

    write(oStr.str());

}
void Socket::msdpSendPair(bstring variable, bstring value) {
    if (variable.empty() || value.empty())
        return;

    std::ostringstream oStr;

    if (this->getMsdp()) {
        oStr
                << (unsigned char) IAC << (unsigned char) SB << (unsigned char) TELOPT_MSDP
                << (unsigned char) MSDP_VAR << variable
                << (unsigned char) MSDP_VAL << value
                << (unsigned char) IAC << (unsigned char) SE;
    } else if (getAtcp()) {
        oStr
                << (unsigned char) IAC << (unsigned char) SB << (unsigned char) TELOPT_ATCP
                << "MSDP." << variable << " " << value
                << (unsigned char) IAC << (unsigned char) SE;
    }

    write(oStr.str());
}



MsdpVariable* Config::getMsdpVariable(bstring& name) {
    std::map<bstring, MsdpVariable*>::iterator it = msdpVariables.find(name);
    if(it == msdpVariables.end())
        return(nullptr);
    else
        return(it->second);
}
void MsdpVariable::init() {
    configurable = writeOnce = false;
    requiresPlayer = true; // Defaults to true, manually disable via xml
    reportable = false; // Not reportable by default, only sendable
    updateInterval = 1;
    name ="unknown";
}
MsdpVariable::MsdpVariable(xmlNodePtr rootNode) {
    init();
    xmlNodePtr curNode = rootNode->children;

        while(curNode) {
                 if(NODE_NAME(curNode, "Name")) xml::copyToBString(name, curNode);
            else if(NODE_NAME(curNode, "SendScript")) xml::copyToBString(sendScript, curNode);
            else if(NODE_NAME(curNode, "UpdateScript")) xml::copyToBString(updateScript, curNode);
            else if(NODE_NAME(curNode, "RequiresPlayer")) xml::copyToBool(requiresPlayer, curNode);
            else if(NODE_NAME(curNode, "Configurable")) xml::copyToBool(configurable, curNode);
            else if(NODE_NAME(curNode, "Reportable")) xml::copyToBool(reportable, curNode);
            else if(NODE_NAME(curNode, "WriteOnce")) xml::copyToBool(writeOnce, curNode);
            else if(NODE_NAME(curNode, "UpdateInterval")) xml::copyToNum(updateInterval, curNode);
            curNode = curNode->next;
        }

    if(name.empty())
        throw(std::runtime_error("No Name for MSDP Variable!\n"));
//  else
//      std::cout << "New MSDP Variable '" << name << "'" << std::endl;
}
MsdpVariable::MsdpVariable() {
    init();
}

ReportedMsdpVariable::ReportedMsdpVariable(const MsdpVariable* mv, Socket* sock) {
    name = mv->getName();
    parentSock = sock;
    configurable = mv->isConfigurable();
    writeOnce = mv->isWriteOnce();
    sendScript = mv->getSendScript();
    updateScript = mv->getUpdateScript();
    updateInterval = mv->getUpdateInterval();
    reportable = mv->isReportable();
    timer.setDelay(updateInterval);

    dirty = true;
    value = "unknown";
}

bstring MsdpVariable::getName() const {
    return(name);
}

bstring MsdpVariable::getSendScript() const {
    return(sendScript);
}

bstring MsdpVariable::getUpdateScript() const {
    return(updateScript);
}

bool MsdpVariable::isConfigurable() const {
    return(configurable);
}

bool MsdpVariable::isReportable() const {
    return(reportable);
}

bool MsdpVariable::isWriteOnce() const {
    return(writeOnce);
}

bool MsdpVariable::getRequiresPlayer() const {
    return(requiresPlayer);
}
int MsdpVariable::getUpdateInterval() const {
    return(updateInterval);
}
bool MsdpVariable::hasSendScript() const {
    return(!sendScript.empty());
}
bool MsdpVariable::hasUpdateScript() const {
    return(!updateScript.empty());
}
bstring ReportedMsdpVariable::getValue() const {
    return(value);
}
bool ReportedMsdpVariable::checkTimer() {
    if(timer.hasExpired()) {
        timer.update(getUpdateInterval());
        return(true);
    } else {
        return(false);
    }
}

void ReportedMsdpVariable::setValue(bstring newValue) {
    if(value != newValue) {
        value = newValue;
        dirty = true;
    }
}
void ReportedMsdpVariable::setValue(int newValue) {
    return(setValue(bstring(newValue)));
}
void ReportedMsdpVariable::setValue(long newValue) {
    return(setValue(bstring(newValue)));
}
bool ReportedMsdpVariable::isDirty() const {
    return(dirty);
}

void ReportedMsdpVariable::update() {
    if(!hasUpdateScript()) return;
    if(!parentSock) return;

    bstring oldValue = value;
    gServer->runPython(getUpdateScript(), "", parentSock, parentSock->getPlayer(), this);
    if(value != oldValue) {
        setDirty(true);
    }
}

void ReportedMsdpVariable::setDirty(bool pDirty) {
    dirty = pDirty;
}


//*********************************************************************
//                      loadMsdpVariables
//*********************************************************************

bool Config::loadMsdpVariables() {
    xmlDocPtr   xmlDoc;

    xmlNodePtr  curNode;
    char        filename[80];

    // build an XML tree from a the file
    sprintf(filename, "%s/msdp.xml", Path::Code);

    xmlDoc = xml::loadFile(filename, "MsdpVariables");
    if(xmlDoc == nullptr)
        return(false);

    curNode = xmlDocGetRootElement(xmlDoc);

    curNode = curNode->children;
    while(curNode && xmlIsBlankNode(curNode)) {
        curNode = curNode->next;
    }
    if(curNode == 0) {
        xmlFreeDoc(xmlDoc);
        xmlCleanupParser();
        return(false);
    }

    clearMsdpVariables();
    while(curNode != nullptr) {
        if(NODE_NAME(curNode, "MsdpVariable")) {
            MsdpVariable* msdpVar = new MsdpVariable(curNode);
            if(msdpVar)
                msdpVariables[msdpVar->getName()] = msdpVar;
        }
        curNode = curNode->next;
    }
    xmlFreeDoc(xmlDoc);
    xmlCleanupParser();
    return(true);
}

void Config::clearMsdpVariables() {
    for(std::pair<bstring,MsdpVariable*> p : msdpVariables) {
        delete p.second;
    }
    msdpVariables.empty();
    for(Socket* sock : gServer->sockets ) {
        if(sock->getAtcp() || sock->getMsdp())
            sock->msdpClearReporting();
    }
}


