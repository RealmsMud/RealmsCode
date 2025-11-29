#include "accountUpgrades.hpp"

#include <algorithm>
#include <cctype>
#include <iterator>
#include <sstream>
#include <stdexcept>
#include <string>

#include "proto.hpp"

constexpr AccountUpgradeDefinition kUpgradeData[] = {
    {AccountUpgradeId::ExperienceGain, std::string_view{"experience"}, std::string_view{"Experience Gain"},
     AccountUpgradeEffectType::PercentXp, 500000U, 10U, 1U},
    {AccountUpgradeId::Strength, std::string_view{"strength"}, std::string_view{"Strength"},
     AccountUpgradeEffectType::FlatStat, 5000000U, 3U, 10U},
    {AccountUpgradeId::Dexterity, std::string_view{"dexterity"}, std::string_view{"Dexterity"},
     AccountUpgradeEffectType::FlatStat, 5000000U, 3U, 10U},
    {AccountUpgradeId::Constitution, std::string_view{"constitution"}, std::string_view{"Constitution"},
     AccountUpgradeEffectType::FlatStat, 5000000U, 3U, 10U},
    {AccountUpgradeId::Intelligence, std::string_view{"intelligence"}, std::string_view{"Intelligence"},
     AccountUpgradeEffectType::FlatStat, 5000000U, 3U, 10U},
    {AccountUpgradeId::Piety, std::string_view{"piety"}, std::string_view{"Piety"},
     AccountUpgradeEffectType::FlatStat, 5000000U, 3U, 10U},
};

const std::vector<AccountUpgradeDefinition>& getAccountUpgradeDefinitions() {
    static const std::vector<AccountUpgradeDefinition> definitions = [] {
        std::vector<AccountUpgradeDefinition> defs;
        defs.reserve(std::size(kUpgradeData));
        defs.insert(defs.end(), std::begin(kUpgradeData), std::end(kUpgradeData));
        return defs;
    }();
    return definitions;
}

const AccountUpgradeDefinition& getAccountUpgrade(AccountUpgradeId id) {
    const auto& defs = getAccountUpgradeDefinitions();
    auto it = std::find_if(defs.begin(), defs.end(), [id](const AccountUpgradeDefinition& def) {
        return def.id == id;
    });
    if (it == defs.end()) {
        throw std::runtime_error("Unknown AccountUpgradeId");
    }
    return *it;
}

const AccountUpgradeDefinition* findAccountUpgradeByToken(std::string_view token) {
    if (token.empty()) {
        return nullptr;
    }
    std::string normalized(token);
    std::transform(normalized.begin(), normalized.end(), normalized.begin(),
                   [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
    const auto& defs = getAccountUpgradeDefinitions();
    auto it = std::find_if(defs.begin(), defs.end(), [&normalized](const AccountUpgradeDefinition& def) {
        return def.token == normalized;
    });
    if (it == defs.end()) {
        return nullptr;
    }
    return &(*it);
}

bool isStatUpgrade(AccountUpgradeId id) {
    switch (id) {
        case AccountUpgradeId::Strength:
        case AccountUpgradeId::Dexterity:
        case AccountUpgradeId::Constitution:
        case AccountUpgradeId::Intelligence:
        case AccountUpgradeId::Piety:
            return true;
        case AccountUpgradeId::ExperienceGain:
            return false;
        default:
            return false;
    }
}

std::string_view getStatName(AccountUpgradeId id) {
    switch (id) {
        case AccountUpgradeId::Strength:
            return "strength";
        case AccountUpgradeId::Dexterity:
            return "dexterity";
        case AccountUpgradeId::Constitution:
            return "constitution";
        case AccountUpgradeId::Intelligence:
            return "intelligence";
        case AccountUpgradeId::Piety:
            return "piety";
        default:
            return {};
    }
}

const AccountUpgradeDefinition* matchAccountUpgrade(std::string_view input, bool& ambiguous) {
    ambiguous = false;
    if(input.empty()) {
        return nullptr;
    }

    std::string lowered(input);
    std::transform(lowered.begin(), lowered.end(), lowered.begin(),
                   [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });

    const AccountUpgradeDefinition* candidate = nullptr;
    const auto& defs = getAccountUpgradeDefinitions();
    for(const auto& def : defs) {
        std::string token(def.token);
        if(partialMatch(lowered, token.c_str(), token.size())) {
            if(candidate) {
                ambiguous = true;
                return nullptr;
            }
            candidate = &def;
        }
    }
    return candidate;
}

std::string describeAccountUpgradeBonus(const AccountUpgradeDefinition& def, unsigned value) {
    std::ostringstream oss;
    if(def.effectType == AccountUpgradeEffectType::PercentXp) {
        oss << "+" << value << "% experience gain";
        return oss.str();
    }

    auto statName = getStatName(def.id);
    if(!statName.empty()) {
        oss << "+" << value << " " << statName;
        return oss.str();
    }

    oss << "+" << value;
    return oss.str();
}

