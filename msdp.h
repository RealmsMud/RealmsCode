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

#ifndef MDSP_H_
#define MDSP_H_

#include <sys/time.h>

#include "common.h"
#include "timer.h"

class Socket;

class MsdpVariable {
protected:
    void            init();

    bstring         name;               // Name of this variable
    bstring         sendScript;         // Python script to send this variable
    bstring         updateScript;       // Python script to update this variable
    bool            reportable;         // This variable is reportable
    bool            requiresPlayer;     // Variable requires a player attached to the socket
    bool            configurable;       // Can it be configured by the client?
    bool            writeOnce;          // Can only set this variable once
    int             updateInterval;     // Update interval (in 10ths of a second)
public:
    MsdpVariable(xmlNodePtr rootNode);
    MsdpVariable();
    // Todo: Make this have the server erase all reported variables of this type
    virtual ~MsdpVariable() { };

    bstring         getName() const;
    bstring         getSendScript() const;
    bool            hasSendScript() const;
    bstring         getUpdateScript() const;
    bool            hasUpdateScript() const;
    bool            isConfigurable() const;
    bool            isReportable() const;
    bool            isWriteOnce() const;
    bool            getRequiresPlayer() const;
    int             getUpdateInterval() const;

};

class ReportedMsdpVariable : public MsdpVariable {
protected:
    bstring         value;
    bool            dirty;
    Socket          *parentSock; // Parent Socket

    Timer           timer;

public:
    ReportedMsdpVariable(const MsdpVariable* mv, Socket *sock);

    bstring         getValue() const;
    void            setValue(bstring newValue);
    void            setValue(int newValue);
    void            setValue(long newValue);
    bool            checkTimer();       // True = ok to send, False = timer hasn't expired yet

    bool            isDirty() const;
    void            update();
    void            setDirty(bool pDirty = true);
};


#endif /* MDSP_H_ */
