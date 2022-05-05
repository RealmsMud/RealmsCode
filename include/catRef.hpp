/*
 * catRef.h
 *   CatRef object
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
#include <iosfwd>           // for size_t
#include <string>           // for hash, string
#include <nlohmann/json_fwd.hpp>

class Creature;

class CatRef {
public:
    friend std::ostream& operator<<(std::ostream& out, const CatRef& group);

    CatRef();
    CatRef(std::string& pArea, short pId);
    void    setDefault(const std::shared_ptr<Creature> & target);
    void    clear();
    void    load(xmlNodePtr curNode);

    xmlNodePtr save(xmlNodePtr curNode, const char* childName, bool saveNonZero, int pos=0) const;
    CatRef& operator=(const CatRef& cr);
    bool    operator==(const CatRef& cr) const;
    bool    operator!=(const CatRef& cr) const;
    [[nodiscard]] std::string str() const;
    [[nodiscard]] std::string displayStr(std::string_view current = "", char color = '\0') const;
    [[nodiscard]] bool    isArea(std::string_view c) const;

    void    setArea(std::string c);
    std::string area;
    short   id{};

public:
    friend void to_json(nlohmann::json &j, const CatRef &cr);
    friend void from_json(const nlohmann::json &j, CatRef &cr);
};

namespace std {
    template <> struct hash<CatRef>{
        size_t operator()(const CatRef &cr ) const {
            return hash<string>()(cr.str());
        }
    };

    template<> struct less<CatRef>{
        bool operator() (const CatRef& lhs, const CatRef& rhs) const {
            return std::tie(lhs.area, lhs.id) < std::tie(rhs.area, rhs.id);
        }
    };
};


class QuestCatRef : public CatRef {
public:
    QuestCatRef();
    explicit QuestCatRef(xmlNodePtr rootNode);
    xmlNodePtr save(xmlNodePtr rootNode, const std::string& saveName = "QuestCatRef") const;

    int curNum;
    int reqNum;     // How many

public:
    friend void to_json(nlohmann::json &j, const QuestCatRef &cr);
    friend void from_json(const nlohmann::json &j, QuestCatRef &cr);

};
