// headless account/auth validators: pure logic, no socket, no game data.
#include <gtest/gtest.h>

#include <filesystem>
#include <memory>

#include "login/accountAuthService.hpp"
#include "account.hpp"
#include "paths.hpp"
#include "apiTestSupport.hpp"

namespace {

using AccountAuth::EmailError;
using AccountAuth::NameError;

// Single fixture for the whole file: checkName's banned-name filter (parse_name)
// reads gConfig, and account-resolution/auth ops need the server singleton (the
// cache seam) too, so SetUp brings up both. Cases that also need real game data
// on disk (resolve/create) skip inside their bodies, as before.
struct AccountAuthService : ::testing::Test {
    void SetUp() override {
        ensureConfig();
        ensureServer();
    }
};

TEST_F(AccountAuthService, NameAcceptsPlainName) { EXPECT_EQ(AccountAuth::checkName("Tordek"), NameError::Ok); }

TEST_F(AccountAuthService, NameAcceptsOneHyphenOrApostropheWhenLongEnough) {
    EXPECT_EQ(AccountAuth::checkName("Al-ric"), NameError::Ok);
    EXPECT_EQ(AccountAuth::checkName("Da'ric"), NameError::Ok);
}

TEST_F(AccountAuthService, NameRejectsEmpty) { EXPECT_EQ(AccountAuth::checkName(""), NameError::FirstNotLetter); }

TEST_F(AccountAuthService, NameRejectsLeadingNonLetter) {
    EXPECT_EQ(AccountAuth::checkName("1bob"), NameError::FirstNotLetter);
    EXPECT_EQ(AccountAuth::checkName("'bob"), NameError::FirstNotLetter);
}

TEST_F(AccountAuthService, NameRejectsTooShort) { EXPECT_EQ(AccountAuth::checkName("Bo"), NameError::TooShort); }

TEST_F(AccountAuthService, NameRejectsTooLong) {
    EXPECT_EQ(AccountAuth::checkName("Abcdefghijklmnopqrst"), NameError::TooLong); // 20 chars
}

TEST_F(AccountAuthService, NameRejectsNonAlphaUnderSix) {
    // has a '-' but only 5 characters
    EXPECT_EQ(AccountAuth::checkName("Al-ic"), NameError::NonAlphaNeedsLength);
}

TEST_F(AccountAuthService, NameRejectsMultipleNonAlpha) { EXPECT_EQ(AccountAuth::checkName("A-b-cd"), NameError::TooManyNonAlpha); }

TEST_F(AccountAuthService, NameRejectsDisallowedCharacter) { EXPECT_EQ(AccountAuth::checkName("Bob_by"), NameError::NonAlphaChar); }

TEST_F(AccountAuthService, NameRejectsTrailingNonLetter) { EXPECT_EQ(AccountAuth::checkName("Robby-"), NameError::LastCharsNotLetters); }

static std::shared_ptr<Account> makeAccount(const std::string &name, const std::string &password) {
    auto account = std::make_shared<Account>(name);
    account->setPassword(password);
    return account;
}

TEST_F(AccountAuthService, OpsResolveReturnsCachedAccount) {
    auto seeded = makeAccount("resolvehit", "secret");
    gServer->cacheAccount(seeded);

    auto result = AccountAuth::resolveAccount("resolvehit");
    EXPECT_TRUE(result.found);
    EXPECT_FALSE(result.banned);
    EXPECT_EQ(result.account, seeded); // shared instance, no disk
}

TEST_F(AccountAuthService, OpsResolveMissingAccount) {
    auto result = AccountAuth::resolveAccount("nosuchaccountxyz");
    EXPECT_FALSE(result.found);
    EXPECT_EQ(result.account, nullptr);
}

TEST_F(AccountAuthService, OpsResolveSurfacesBan) {
    auto seeded = makeAccount("bannedacct", "secret");
    seeded->setBanned(true);
    seeded->setBanReason("cheating");
    gServer->cacheAccount(seeded);

    auto result = AccountAuth::resolveAccount("bannedacct");
    EXPECT_TRUE(result.found);
    EXPECT_TRUE(result.banned);
    EXPECT_EQ(result.banReason, "cheating");
}

TEST_F(AccountAuthService, OpsVerifyPasswordMatches) {
    auto account = makeAccount("verifyacct", "correct horse");
    EXPECT_TRUE(AccountAuth::verifyPassword(*account, "correct horse"));
    EXPECT_FALSE(AccountAuth::verifyPassword(*account, "wrong"));
}

TEST_F(AccountAuthService, OpsIsValidPasswordBoundaries) {
    using AccountAuth::kMaxPasswordLen;
    using AccountAuth::kMinPasswordLen;
    EXPECT_FALSE(AccountAuth::isValidPassword(std::string(kMinPasswordLen - 1, 'a')));
    EXPECT_TRUE(AccountAuth::isValidPassword(std::string(kMinPasswordLen, 'a')));
    EXPECT_TRUE(AccountAuth::isValidPassword(std::string(kMaxPasswordLen, 'a')));
    EXPECT_FALSE(AccountAuth::isValidPassword(std::string(kMaxPasswordLen + 1, 'a')));
}

TEST_F(AccountAuthService, OpsCreateAccountFailsOnEmptyName) {
    // Account::save() refuses an empty name; createAccount surfaces that as null.
    EXPECT_EQ(AccountAuth::createAccount("", "password"), nullptr);
}

TEST_F(AccountAuthService, OpsCreateAccountRoundtrips) {
    if (getenv("REALMS_BASE_PATH") == nullptr)
        GTEST_SKIP() << "needs writable game-data path for Account::save";

    const std::string name = "authtestacct";
    const auto file = (Path::Account / name).replace_extension("json");
    std::filesystem::remove(file);

    auto created = AccountAuth::createAccount(name, "newpassword");
    ASSERT_NE(created, nullptr);
    EXPECT_TRUE(AccountAuth::verifyPassword(*created, "newpassword"));
    // Cached, so resolve returns the same instance without re-reading disk.
    EXPECT_EQ(AccountAuth::resolveAccount(name).account, created);

    std::filesystem::remove(file);
}

TEST_F(AccountAuthService, OpsLegacyLookupMissingCharacter) {
    auto lookup = AccountAuth::lookupLegacyCharacter("nosuchcharxyz");
    EXPECT_FALSE(lookup.found);
    EXPECT_EQ(lookup.player, nullptr);
}

TEST_F(AccountAuthService, OpsVerifyCharacterPasswordMatches) {
    auto player = makePlayer(CreatureClass::FIGHTER);
    player->setPassword("charpass");
    EXPECT_TRUE(AccountAuth::verifyCharacterPassword(*player, "charpass"));
    EXPECT_FALSE(AccountAuth::verifyCharacterPassword(*player, "nope"));
}

TEST_F(AccountAuthService, OpsClaimCharacterLinksToAccount) {
    auto account = std::make_shared<Account>("claimacct");
    auto player = makePlayer(CreatureClass::FIGHTER);
    player->setName("Claimee");

    EXPECT_TRUE(AccountAuth::claimCharacter(*account, *player));
    EXPECT_EQ(player->getAccountName(), "claimacct");
    EXPECT_TRUE(account->hasCharacter("Claimee"));
}

TEST_F(AccountAuthService, OpsClaimCharacterRejectsUnnamed) {
    auto account = std::make_shared<Account>("claimacct2");
    auto player = makePlayer(CreatureClass::FIGHTER); // no name set

    EXPECT_FALSE(AccountAuth::claimCharacter(*account, *player));
    EXPECT_TRUE(player->getAccountName().empty());
}

TEST_F(AccountAuthService, EmailAcceptsBasic) { EXPECT_EQ(AccountAuth::checkEmail("a@b.com"), EmailError::Ok); }

TEST_F(AccountAuthService, EmailRejectsMissingAtOrDot) {
    EXPECT_EQ(AccountAuth::checkEmail("abc.com"), EmailError::BadFormat);
    EXPECT_EQ(AccountAuth::checkEmail("a@bcom"), EmailError::BadFormat);
}

TEST_F(AccountAuthService, EmailRejectsTooLong) { EXPECT_EQ(AccountAuth::checkEmail("a@" + std::string(260, 'b') + ".com"), EmailError::TooLong); }

} // namespace
