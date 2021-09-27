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
 *  Copyright (C) 2007-2020 Jason Mitchell, Randi Mitchell
 *     Contributions by Tim Callahan, Jonathan Hseu
 *  Based on Mordor (C) Brooke Paul, Brett J. Vickers, John P. Freeman
 *
 */
#ifndef _HOOKS_H
#define _HOOKS_H

#include <libxml/parser.h>  // for xmlNodePtr
#include <map>
#include <set>

class MudObject;
class Swap;

class Hooks {
public:
    Hooks();
    Hooks& operator=(const Hooks& h);
    void doCopy(const Hooks& h);
    void save(xmlNodePtr curNode, const char* name) const;
    void load(xmlNodePtr curNode);
    bstring display() const;
    void add(std::string_view event, std::string_view code);
    bool execute(std::string_view event, MudObject* target=0, std::string_view param1="", std::string_view param2="", std::string_view param3="") const;
    bool executeWithReturn(std::string_view event, MudObject* target=0, std::string_view param1="", std::string_view param2="", std::string_view param3="") const;
    void setParent(MudObject* target);

    static bool run(MudObject* trigger1, std::string_view event1, MudObject* trigger2, std::string_view event2, std::string_view param1="", std::string_view param2="", std::string_view param3="");

    template<class Type, class Compare>
    inline static bool run(std::set<Type, Compare>& set, MudObject* trigger, std::string_view event, std::string_view param1="", std::string_view param2="", std::string_view param3="") {
        bool ran=false;
#ifndef PYTHON_CODE_GEN
        for(Type crt : set) {
            if(crt != trigger) {
                if(crt->hooks.execute(event, trigger, param1, param2, param3))
                    ran = true;
            }
        }
#endif
        return(ran);
    }

    bool swap(const Swap& s);
    bool swapIsInteresting(const Swap& s) const;
private:
    std::map<bstring,bstring> hooks;
    MudObject* parent;
};

#endif  /* _HOOKS_H */

