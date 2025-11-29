#pragma once

#include <string>
#include <string_view>
#include <vector>

enum class AccountUpgradeId {
    ExperienceGain = 0,
    Strength,
    Dexterity,
    Constitution,
    Intelligence,
    Piety,
    COUNT
};

enum class AccountUpgradeEffectType {
    PercentXp,
    FlatStat
};

struct AccountUpgradeDefinition {
    AccountUpgradeId id;
    std::string_view token;
    std::string_view displayName;
    AccountUpgradeEffectType effectType;
    unsigned int costPerRank;
    unsigned int maxRank;
    unsigned int magnitudePerRank;
};

const std::vector<AccountUpgradeDefinition>& getAccountUpgradeDefinitions();

const AccountUpgradeDefinition& getAccountUpgrade(AccountUpgradeId id);

const AccountUpgradeDefinition* findAccountUpgradeByToken(std::string_view token);

bool isStatUpgrade(AccountUpgradeId id);
std::string_view getStatName(AccountUpgradeId id);

const AccountUpgradeDefinition* matchAccountUpgrade(std::string_view input);
std::string describeAccountUpgradeBonus(const AccountUpgradeDefinition& def, unsigned value);
