/*
 * hooks.h
 *   Dynamic hooks
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

#pragma once

#include <libxml/parser.h>  // for xmlNodePtr
#include <map>
#include <set>
#include "json.hpp"

class MudObject;
class Swap;

class Hooks {
private:
    std::map<std::string,std::string> hooks;
    MudObject* parent{};

public:
    friend void to_json(nlohmann::json &j, const Hooks &h);
    friend void from_json(const nlohmann::json &j, Hooks &h);

    Hooks(MudObject* target);
    Hooks& operator=(const Hooks& h);
    void doCopy(const Hooks& h);
    void save(xmlNodePtr curNode, const char* name) const;
    void load(xmlNodePtr curNode);

    [[nodiscard]] bool empty() const;
    [[nodiscard]] std::string display() const;
    void add(std::string_view event, std::string_view code);
    [[nodiscard]] bool execute(const std::string &event, const std::shared_ptr<MudObject>& target= nullptr, const std::string &param1="", const std::string &param2="", const std::string &param3="") const;
    [[nodiscard]] bool executeWithReturn(const std::string &event, const std::shared_ptr<MudObject>& target=nullptr, const std::string &param1="", const std::string &param2="", const std::string &param3="") const;
    void setParent(MudObject* target);

    static bool run(const std::shared_ptr<MudObject>& trigger1, const std::string &event1, const std::shared_ptr<MudObject>& trigger2, const std::string &event2, const std::string &param1="", const std::string &param2="", const std::string &param3="");

    template<class Type, class Compare>
    inline static bool run(std::set<Type, Compare>& set, std::shared_ptr<MudObject> trigger, const std::string &event, const std::string &param1= "", const std::string &param2= "", const std::string &param3= "") {
        bool ran=false;
        for(Type crt : set) {
            if(crt != trigger) {
                if(crt->hooks.execute(event, trigger, param1, param2, param3))
                    ran = true;
            }
        }
        return(ran);
    }

    bool swap(const Swap& s);
    [[nodiscard]] bool swapIsInteresting(const Swap& s) const;

};

