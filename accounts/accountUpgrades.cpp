#include "accountUpgrades.hpp"

#include <algorithm>
#include <cctype>
#include <iterator>
#include <stdexcept>
#include <string>

constexpr AccountUpgradeDefinition kUpgradeData[] = {
    {AccountUpgradeId::ExperienceGain, std::string_view{"experience"}, std::string_view{"Experience Gain"},
     AccountUpgradeEffectType::PercentXp, 500000U, 10U, 1U},
    {AccountUpgradeId::Strength, std::string_view{"strength"}, std::string_view{"Strength"},
     AccountUpgradeEffectType::FlatStat, 5000000U, 3U, 1U},
    {AccountUpgradeId::Dexterity, std::string_view{"dexterity"}, std::string_view{"Dexterity"},
     AccountUpgradeEffectType::FlatStat, 5000000U, 3U, 1U},
    {AccountUpgradeId::Constitution, std::string_view{"constitution"}, std::string_view{"Constitution"},
     AccountUpgradeEffectType::FlatStat, 5000000U, 3U, 1U},
    {AccountUpgradeId::Intelligence, std::string_view{"intelligence"}, std::string_view{"Intelligence"},
     AccountUpgradeEffectType::FlatStat, 5000000U, 3U, 1U},
    {AccountUpgradeId::Piety, std::string_view{"piety"}, std::string_view{"Piety"},
     AccountUpgradeEffectType::FlatStat, 5000000U, 3U, 1U},
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

