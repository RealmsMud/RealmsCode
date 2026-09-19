// drives login() against Socket(-1)
// per-operation logic lives in accountAuthService_test
#include <filesystem>
#include <memory>
#include <string>

#include <gtest/gtest.h>

#include "account.hpp"
#include "login.hpp"
#include "login/accountAuthService.hpp"
#include "login/loginSession.hpp"
#include "paths.hpp"
#include "socket.hpp"
#include "apiTestSupport.hpp"

namespace {

struct TestLoginSocket : public Socket {
    TestLoginSocket() : Socket(-1) {}
};

// Position the socket at the account/login entry choice (a = account, b = legacy).
static void startLogin(const std::shared_ptr<Socket>& sock) {
    sock->getLoginSession().step = LoginSession::Step::EntryChoice;
    sock->setState(LOGIN_ACCOUNT);
}

struct LoginFlow : ::testing::Test {
    std::shared_ptr<TestLoginSocket> sock;
    void SetUp() override {
        ensureConfig();
        ensureServer();
        sock = std::make_shared<TestLoginSocket>();
    }
    void TearDown() override {
        if(sock) sock->cleanUp();  // satisfy ~Socket's teardown-once invariant
    }
    void removeAccountFile(const std::string& name) {
        std::filesystem::remove((Path::Account / name).replace_extension("json"));
    }
};

TEST_F(LoginFlow, EntryChoiceInvalidStaysPut) {
    startLogin(sock);
    login(sock, "x");
    EXPECT_NE(sock->getState(), CON_DISCONNECTING);
    EXPECT_TRUE(sock->getLoginSession().accountName.empty());
}

TEST_F(LoginFlow, AccountNameStoredNormalized) {
    auto acct = std::make_shared<Account>("Zqxnamed");
    acct->setPassword("pw12345");
    gServer->cacheAccount(acct);

    startLogin(sock);
    login(sock, "a");
    login(sock, "zqxnamed");   // typed lowercase; stored proper-cased

    EXPECT_EQ(sock->getLoginSession().accountName, "Zqxnamed");
    EXPECT_NE(sock->getState(), CON_DISCONNECTING);
}

// AccountAuth::kMaxNameLen bounds the transport; the name is rejected outright rather
// than clipped into something that would pass checkName.
TEST_F(LoginFlow, OverlongAccountNameRejected) {
    startLogin(sock);
    login(sock, "a");
    login(sock, std::string(AccountAuth::kMaxNameLen + 5, 'z'));

    EXPECT_TRUE(sock->getLoginSession().accountName.empty());
    EXPECT_NE(sock->getState(), CON_DISCONNECTING);
}

TEST_F(LoginFlow, UnknownAccountOffersCreation) {
    startLogin(sock);
    login(sock, "a");
    login(sock, "zqxunknownacct");  // not cached, not on disk, not a DM name

    // name retained for the create-confirm step
    EXPECT_EQ(sock->getLoginSession().accountName, "Zqxunknownacct");
    EXPECT_NE(sock->getState(), CON_DISCONNECTING);
}

TEST_F(LoginFlow, DecliningCreationClearsName) {
    startLogin(sock);
    login(sock, "a");
    login(sock, "zqxunknownacct");
    login(sock, "no");

    EXPECT_TRUE(sock->getLoginSession().accountName.empty());
    EXPECT_NE(sock->getState(), CON_DISCONNECTING);
}

TEST_F(LoginFlow, WrongPasswordDisconnects) {
    auto acct = std::make_shared<Account>("Zqxwrongpw");
    acct->setPassword("rightpass");
    gServer->cacheAccount(acct);

    startLogin(sock);
    login(sock, "a");
    login(sock, "zqxwrongpw");
    login(sock, "nope");

    EXPECT_EQ(sock->getState(), CON_DISCONNECTING);
    EXPECT_EQ(sock->getAccount(), nullptr);
}

TEST_F(LoginFlow, ValidLoginBindsAccount) {
    if(getenv("REALMS_BASE_PATH") == nullptr)
        GTEST_SKIP() << "successful login records lastLogin (Account::save), needs writable path";

    auto acct = std::make_shared<Account>("Zqxgoodpw");
    acct->setPassword("rightpass");
    gServer->cacheAccount(acct);
    removeAccountFile("Zqxgoodpw");

    startLogin(sock);
    login(sock, "a");
    login(sock, "zqxgoodpw");
    login(sock, "rightpass");

    EXPECT_EQ(sock->getAccount(), acct);
    EXPECT_NE(sock->getState(), CON_DISCONNECTING);

    removeAccountFile("Zqxgoodpw");
}

TEST_F(LoginFlow, CreateAccountWithValidPassword) {
    if(getenv("REALMS_BASE_PATH") == nullptr)
        GTEST_SKIP() << "account creation persists (Account::save), needs writable path";

    removeAccountFile("Zqxcreated");

    startLogin(sock);
    login(sock, "a");
    login(sock, "zqxcreated");   // unknown -> create
    login(sock, "yes");          // confirm creation
    login(sock, "newpass123");   // valid password

    auto account = sock->getAccount();
    ASSERT_NE(account, nullptr);
    EXPECT_EQ(account->getName(), "Zqxcreated");

    removeAccountFile("Zqxcreated");
}

TEST_F(LoginFlow, ShortCreatePasswordReprompts) {
    startLogin(sock);
    login(sock, "a");
    login(sock, "zqxshortpw");
    login(sock, "yes");
    login(sock, "abc");   // too short (<5): re-prompts, no account created

    EXPECT_EQ(sock->getAccount(), nullptr);
    EXPECT_NE(sock->getState(), CON_DISCONNECTING);
}

TEST_F(LoginFlow, LegacyUnknownCharacterReprompts) {
    startLogin(sock);
    login(sock, "b");
    login(sock, "zqxnosuchchar");   // no such player on disk

    EXPECT_TRUE(sock->getLoginSession().targetCharacter.empty());
    EXPECT_NE(sock->getState(), CON_DISCONNECTING);
}

} // namespace
