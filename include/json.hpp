/*
 * json.hpp
 *   Realms JSON functions
 *   ____            _
 *  |  _ \ ___  __ _| |_ __ ___  ___
 *  | |_) / _ \/ _` | | '_ ` _ \/ __|
 *  |  _ <  __/ (_| | | | | | | \__ \
 *  |_| \_\___|\__,_|_|_| |_| |_|___/
 *
 * Permission to use, modify and distribute is granted via the
 *  GNU Affero General Public License v3 or later

 *  Copyright (C) 2007-2021 Jason Mitchell, Randi Mitchell
 *     Contributions by Tim Callahan, Jonathan Hseu
 *  Based on Mordor (C) Brooke Paul, Brett J. Vickers, John P. Freeman
 *
 */

#pragma once

#include <nlohmann/json.hpp>
#include <boost/dynamic_bitset.hpp>

using json = nlohmann::json;

namespace boost {
    void to_json(nlohmann::json &j, const boost::dynamic_bitset<> &b);
    void from_json(const nlohmann::json &j, boost::dynamic_bitset<> &b);
}

// Character-creation flow wire types (see login/creationFlow.hpp). Defined in
// json/creationFlow-json.cpp; in namespace Create::Flow for ADL.
namespace Create {
    struct Option;   // Flow::ViewOption aliases this, so ADL lands here
    void to_json(nlohmann::json &j, const Option &o);
}
namespace Create::Flow {
    struct StepView;
    struct ViewBlock;
    struct Choice;
    void to_json(nlohmann::json &j, const StepView &v);
    void to_json(nlohmann::json &j, const ViewBlock &b);
    void to_json(nlohmann::json &j, const Choice &c);
    void from_json(const nlohmann::json &j, Choice &c);
}
