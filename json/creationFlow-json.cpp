/*
 * creationFlow-json.cpp
 *   The draft is never serialized; the server rebuilds it from the path.
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

#include <stdexcept>
#include <string>

#include "json.hpp"
#include "login/creationFlow.hpp"

namespace Create {

void to_json(nlohmann::json &j, const Option &o) {
    j = nlohmann::json{{"value", o.value}, {"label", o.label}, {"group", o.group}};
}

} // namespace Create

namespace Create::Flow {

static std::string inputKindToken(InputKind k) {
    switch(k) {
        case InputKind::None:    return "none";
        case InputKind::Letter:  return "letter";
        case InputKind::Text:    return "text";
        case InputKind::Stats:   return "stats";
        case InputKind::Confirm: return "confirm";
    }
    return "none";
}

void to_json(nlohmann::json &j, const ViewBlock &b) {
    j = nlohmann::json{
        {"kind", b.kind == BlockKind::Art ? "art" : "text"},
        {"text", b.text},
        {"lines", b.lines}
    };
}

void to_json(nlohmann::json &j, const StepView &v) {
    j = nlohmann::json{
        {"step", stepToken(v.step)},
        {"inputKind", inputKindToken(v.inputKind)},
        {"prompt", v.prompt},
        {"blocks", v.blocks},
        {"options", v.options}
    };
}

void to_json(nlohmann::json &j, const Choice &c) {
    j = nlohmann::json{{"step", stepToken(c.step)}, {"value", c.value}};
}

void from_json(const nlohmann::json &j, Choice &c) {
    const auto step = stepFromToken(j.at("step").get<std::string>());
    if(!step)
        throw std::invalid_argument("unknown creation step token");
    c.step = *step;
    c.value = j.at("value").get<std::string>();
}

} // namespace Create::Flow
