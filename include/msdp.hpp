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
 *  Copyright (C) 2007-2020 Jason Mitchell, Randi Mitchell
 *     Contributions by Tim Callahan, Jonathan Hseu
 *  Based on Mordor (C) Brooke Paul, Brett J. Vickers, John P. Freeman
 *
 */

#ifndef MDSP_H_
#define MDSP_H_

#include <sys/time.h>

#include "timer.hpp"

class Socket;

enum class MSDPVar {
    SERVER_ID,
    SERVER_TIME,
    CHARACTER_NAME,
    HEALTH,
    HEALTH_MAX,
    MANA,
    MANA_MAX,
    EXPERIENCE,
    EXPERIENCE_MAX,
    EXPERIENCE_TNL,
    EXPERIENCE_TNL_MAX,
    WIMPY,
    MONEY,
    BANK,
    ARMOR,
    ARMOR_ABSORB,
    GROUP,
    TARGET,
    TARGET_ID,
    TARGET_HEALTH,
    TARGET_HEALTH_MAX,
    TARGET_STRENGTH,
    ROOM,
    CLIENT_ID,
    CLIENT_VERSION,
    PLUGIN_ID,
    ANSI_COLORS,
    XTERM_256_COLORS,
    UTF_8,
    SOUND,
    MXP,

    UNKNOWN,
};




class MsdpVariable {
    friend class ReportedMsdpVariable;
public:
    static bstring getValue(MSDPVar var, Socket* sock, Player* player);

protected:
    void            init();

    bstring         name;               // Name of this variable
    MSDPVar         varId;
    bool            reportable{};         // This variable is reportable
    bool            requiresPlayer{};     // Variable requires a player attached to the socket
    bool            configurable{};       // Can it be configured by the client?
    bool            writeOnce{};          // Can only set this variable once
    int             updateInterval{};     // Update interval (in 10ths of a second)
    bool            sendFn{};             // Does this have a send function?
    bool            updateFn{};           // Does this have an update function?
    bool            isGroup{};            // Is this a group of related variables?


public:
    MsdpVariable();
    MsdpVariable(const bstring& pName, MSDPVar pVar, bool pReportable, bool pRequiresPlayer, bool pConfigurable,
                 bool pWriteOnce, int pUpdateInterval, bool pSendFn = false, bool pUpdateFn = false,
                 bool pIsGroup = false);
    // Todo: Make this have the server erase all reported variables of this type
    virtual ~MsdpVariable() = default;

    [[nodiscard]] bstring         getName() const;
    [[nodiscard]] MSDPVar         getId() const;
    [[nodiscard]] bool            hasSendFn() const;
    [[nodiscard]] bool            hasUpdateFn() const;
    [[nodiscard]] bool            isConfigurable() const;
    [[nodiscard]] bool            isReportable() const;
    [[nodiscard]] bool            isWriteOnce() const;
    [[nodiscard]] bool            getRequiresPlayer() const;
    [[nodiscard]] int             getUpdateInterval() const;

    bool            send(Socket* sock) const;

};

class ReportedMsdpVariable : public MsdpVariable {
protected:
    bstring         value;
    bool            dirty;
    Socket          *parentSock; // Parent Socket

    Timer           timer;

public:
    ReportedMsdpVariable(const MsdpVariable* mv, Socket *sock);

    [[nodiscard]] bstring         getValue() const;
    void            setValue(const bstring& newValue);
    void            setValue(int newValue);
    void            setValue(long newValue);
    bool            checkTimer();       // True = ok to send, False = timer hasn't expired yet

    [[nodiscard]] bool            isDirty() const;
    void            update();
    void            setDirty(bool pDirty = true);
};


#endif /* MDSP_H_ */
