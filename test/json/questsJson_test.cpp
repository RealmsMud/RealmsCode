/*
 * questsJson_test.cpp
 *   Regression guards for the CatRef/QuestCatRef JSON serializers. to_json(CatRef)
 *   used to emit a one-element array (double-brace bug), which made from_json fail
 *   and QuestCatRef::to_json throw on j.update(). That broke every quest with
 *   item/mob requirements and blocked the per-zone JSON migration.
 */
#include <gtest/gtest.h>

#include "json.hpp"
#include "catRef.hpp"
#include "apiTestSupport.hpp"   // ensureConfig: QuestCatRef() reads gConfig->getDefaultArea()

using json = nlohmann::json;

TEST(CatRefJson, SerializesAsObject) {
    CatRef cr;
    cr.setArea("misc");
    cr.id = 5;

    json j;
    to_json(j, cr);

    ASSERT_TRUE(j.is_object());          // was a 1-element array before the fix
    EXPECT_EQ(j.at("area").get<std::string>(), "misc");
    EXPECT_EQ(j.at("id").get<int>(), 5);
}

TEST(CatRefJson, RoundTrips) {
    CatRef cr;
    cr.setArea("baladus");
    cr.id = 163;

    json j;
    to_json(j, cr);
    CatRef back;
    from_json(j, back);

    EXPECT_EQ(back.area, "baladus");
    EXPECT_EQ(back.id, 163);
}

TEST(CatRefJson, QuestCatRefRoundTrips) {
    ensureConfig();
    QuestCatRef qcr;
    qcr.setArea("misc");
    qcr.id = 9570;
    qcr.curNum = 0;
    qcr.reqNum = 1;

    json j;
    to_json(j, qcr);                     // used to throw: j.update() on an array

    ASSERT_TRUE(j.is_object());
    EXPECT_EQ(j.at("area").get<std::string>(), "misc");
    EXPECT_EQ(j.at("id").get<int>(), 9570);
    EXPECT_EQ(j.at("reqNum").get<int>(), 1);

    QuestCatRef back;
    from_json(j, back);
    EXPECT_EQ(back.area, "misc");
    EXPECT_EQ(back.id, 9570);
    EXPECT_EQ(back.reqNum, 1);
}
