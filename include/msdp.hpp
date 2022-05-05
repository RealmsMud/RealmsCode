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
 *  Copyright (C) 2007-2021 Jason Mitchell, Randi Mitchell
 *     Contributions by Tim Callahan, Jonathan Hseu
 *  Based on Mordor (C) Brooke Paul, Brett J. Vickers, John P. Freeman
 *
 */

#ifndef MDSP_H_
#define MDSP_H_

#include <functional>
#include <ctime>

#include "timer.hpp"

class Socket;
class Player;
class MsdpBuilder;


class MsdpVariable {
    friend class ReportedMsdpVariable;
    friend class MsdpBuilder;

protected:
    MsdpVariable() = default;

    std::string name;          // Name of this variable
    bool reportable{};         // This variable is reportable
    bool requiresPlayer{};     // Variable requires a player attached to the socket
    bool configurable{};       // Can it be configured by the client?
    bool writeOnce{};          // Can only set this variable once
    int updateInterval{};      // Update interval (in 10ths of a second)
    bool updateable{};         // Does this have an update function?
    bool isGroup{};            // Is this a group of related variables?

    std::function<std::string(Socket&, std::shared_ptr<Player>)> valueFn{nullptr}; // Function to send the variable

    MsdpVariable(const MsdpVariable&) = default;
public:
    MsdpVariable(MsdpVariable&&) = default;

    virtual ~MsdpVariable() = default;

    [[nodiscard]] const std::string & getName() const;
    [[nodiscard]] bool hasValueFn() const;
    [[nodiscard]] bool isUpdatable() const;
    [[nodiscard]] bool isConfigurable() const;
    [[nodiscard]] bool isReportable() const;
    [[nodiscard]] bool isWriteOnce() const;
    [[nodiscard]] bool getRequiresPlayer() const;
    [[nodiscard]] int getUpdateInterval() const;

    bool send(Socket &sock) const;

};

class ReportedMsdpVariable : public MsdpVariable {
protected:
    std::string value;
    bool dirty;
    Socket *parentSock; // Parent Socket

    Timer timer;

public:
    ReportedMsdpVariable(const ReportedMsdpVariable&) = default;
    ReportedMsdpVariable(const MsdpVariable *mv, Socket *sock);

    [[nodiscard]] std::string getValue() const;
    void setValue(std::string_view newValue);
    void setValue(int newValue);
    void setValue(long newValue);
    bool checkTimer();       // True = ok to send, False = timer hasn't expired yet

    [[nodiscard]] bool isDirty() const;
    void update();
    void setDirty(bool pDirty = true);
};

namespace msdp {

    // Reporting Functions
    std::string getServerId(Socket &sock, const std::shared_ptr<Player>& player);
    std::string getServerTime(Socket &sock, const std::shared_ptr<Player>& player);
    const std::string &getCharacterName(Socket &sock, const std::shared_ptr<Player>& player);
    std::string getHealth(Socket &sock, const std::shared_ptr<Player>& player);
    std::string getHealthMax(Socket &sock, const std::shared_ptr<Player>& player);
    std::string getMana(Socket &sock, const std::shared_ptr<Player>& player);
    std::string getManaMax(Socket &sock, const std::shared_ptr<Player>& player);
    std::string getExperience(Socket &sock, const std::shared_ptr<Player>& player);
    std::string getExperienceMax(Socket &sock, const std::shared_ptr<Player>& player);
    std::string getExperienceTNL(Socket &sock, const std::shared_ptr<Player>& player);
    std::string getExperienceTNLMax(Socket &sock, const std::shared_ptr<Player>& player);
    std::string getWimpy(Socket &sock, const std::shared_ptr<Player>& player);
    std::string getMoney(Socket &sock, const std::shared_ptr<Player>& player);
    std::string getBank(Socket &sock, const std::shared_ptr<Player>& player);
    std::string getArmor(Socket &sock, const std::shared_ptr<Player>& player);
    std::string getArmorAbsorb(Socket &sock, const std::shared_ptr<Player>& player);
    std::string getGroup(Socket &sock, const std::shared_ptr<Player>& player);
    const std::string& getTarget(Socket &sock, const std::shared_ptr<Player>& player);
    const std::string& getTargetID(Socket &sock, const std::shared_ptr<Player>& player);
    std::string getTargetHealth(Socket &sock, const std::shared_ptr<Player>& player);
    std::string getTargetHealthMax(Socket &sock, const std::shared_ptr<Player>& player);
    std::string getTargetStrength(Socket &sock, const std::shared_ptr<Player>& player);
    std::string getRoom(Socket &sock, const std::shared_ptr<Player>& player);
};
#endif /* MDSP_H_ */
