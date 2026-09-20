/*
 * configXml_test.cpp
 *   Round-trip of the HTTP/throttle config keys through loadConfigFile/saveConfigFile.
 */
#include <filesystem>
#include <fstream>
#include <string>
#include <system_error>

#include <gtest/gtest.h>

#include "config.hpp"
#include "apiTestSupport.hpp"

namespace fs = std::filesystem;

namespace {

struct TempFile {
    fs::path path;
    explicit TempFile(const std::string &tag) : path(fs::temp_directory_path() / ("realms_config_" + tag + ".xml")) {}
    void write(const std::string &content) const { std::ofstream(path) << content; }
    ~TempFile() {
        std::error_code ec;
        fs::remove(path, ec);
    }
};

const std::string kEmptyGeneral = "<Config><General/></Config>";

std::string general(const std::string &body) { return "<Config><General>" + body + "</General></Config>"; }

struct ConfigXml : ::testing::Test {
    void SetUp() override { ensureConfig(); }
    // Other suites read gConfig; leave it on documented defaults, not this test's values.
    void TearDown() override {
        TempFile empty("teardown");
        empty.write(kEmptyGeneral);
        gConfig->loadConfigFile(empty.path, true);
    }
};

TEST_F(ConfigXml, DefaultsFromEmptyGeneral) {
    TempFile f("defaults");
    f.write(kEmptyGeneral);
    ASSERT_TRUE(gConfig->loadConfigFile(f.path, true));

    EXPECT_EQ(gConfig->getHttpPort(), 8080);
    EXPECT_EQ(gConfig->getJwtSecret(), "");
    EXPECT_EQ(gConfig->getJwtIssuer(), "realms");
    EXPECT_EQ(gConfig->getMaxLoginFailures(), 10);
    EXPECT_EQ(gConfig->getLoginFailureWindow(), 300);
    EXPECT_EQ(gConfig->getLoginLockoutTime(), 600);
    EXPECT_FALSE(gConfig->getTrustXForwardedFor());
}

TEST_F(ConfigXml, LoadsEveryKey) {
    TempFile f("load");
    f.write(general("<HttpPort>9999</HttpPort><JwtSecret>s3cret</JwtSecret><JwtIssuer>elsewhere</JwtIssuer>"
                    "<MaxLoginFailures>4</MaxLoginFailures><LoginFailureWindow>30</LoginFailureWindow>"
                    "<LoginLockoutTime>90</LoginLockoutTime><TrustXForwardedFor>yes</TrustXForwardedFor>"));
    ASSERT_TRUE(gConfig->loadConfigFile(f.path, true));

    EXPECT_EQ(gConfig->getHttpPort(), 9999);
    EXPECT_EQ(gConfig->getJwtSecret(), "s3cret");
    EXPECT_EQ(gConfig->getJwtIssuer(), "elsewhere");
    EXPECT_EQ(gConfig->getMaxLoginFailures(), 4);
    EXPECT_EQ(gConfig->getLoginFailureWindow(), 30);
    EXPECT_EQ(gConfig->getLoginLockoutTime(), 90);
    EXPECT_TRUE(gConfig->getTrustXForwardedFor());
}

TEST_F(ConfigXml, SaveRoundTripsEveryKey) {
    TempFile src("rt_src");
    TempFile dst("rt_dst");
    src.write(general("<HttpPort>9999</HttpPort><JwtSecret>s3cret</JwtSecret><JwtIssuer>elsewhere</JwtIssuer>"
                      "<MaxLoginFailures>4</MaxLoginFailures><LoginFailureWindow>30</LoginFailureWindow>"
                      "<LoginLockoutTime>90</LoginLockoutTime><TrustXForwardedFor>yes</TrustXForwardedFor>"));
    ASSERT_TRUE(gConfig->loadConfigFile(src.path, true));
    ASSERT_TRUE(gConfig->saveConfigFile(dst.path));

    TempFile empty("rt_empty");
    empty.write(kEmptyGeneral);
    ASSERT_TRUE(gConfig->loadConfigFile(empty.path, true));
    ASSERT_EQ(gConfig->getHttpPort(), 8080);

    ASSERT_TRUE(gConfig->loadConfigFile(dst.path, true));
    EXPECT_EQ(gConfig->getHttpPort(), 9999);
    EXPECT_EQ(gConfig->getJwtSecret(), "s3cret");
    EXPECT_EQ(gConfig->getJwtIssuer(), "elsewhere");
    EXPECT_EQ(gConfig->getMaxLoginFailures(), 4);
    EXPECT_EQ(gConfig->getLoginFailureWindow(), 30);
    EXPECT_EQ(gConfig->getLoginLockoutTime(), 90);
    EXPECT_TRUE(gConfig->getTrustXForwardedFor());
}

// 0 means "HTTP disabled", so it must survive a save rather than being dropped as a zero.
TEST_F(ConfigXml, HttpPortZeroRoundTrips) {
    TempFile src("zero_src");
    TempFile dst("zero_dst");
    src.write(general("<HttpPort>0</HttpPort>"));
    ASSERT_TRUE(gConfig->loadConfigFile(src.path, true));
    ASSERT_EQ(gConfig->getHttpPort(), 0);
    ASSERT_TRUE(gConfig->saveConfigFile(dst.path));

    ASSERT_TRUE(gConfig->loadConfigFile(dst.path, true));
    EXPECT_EQ(gConfig->getHttpPort(), 0);
}

TEST_F(ConfigXml, MissingFileFails) { EXPECT_FALSE(gConfig->loadConfigFile(fs::temp_directory_path() / "realms_config_nosuch.xml", true)); }

} // namespace
