/*
 * objectsJson_test.cpp
 *   Shape/contract guards for the Object JSON read-path serializer.
 *
 * Regression: obj.compass (MapMarker*) and obj.increase (ObjIncrease*) must be
 * emitted as "compass"/"increase" for every saveType when set, including LS_REF
 * (previously these were only written for LS_FULL/LS_PROTOTYPE).
 */
#include <gtest/gtest.h>

#include "json.hpp"
#include "area.hpp"
#include "objIncrease.hpp"
#include "mudObjects/objects.hpp"
#include "apiTestSupport.hpp"

using json = nlohmann::json;

TEST(ObjectJson, CompassAndIncreaseEmittedForRef) {
    ensureConfig();
    ensureServer();
    auto obj = std::make_shared<Object>();
    obj->compass = new MapMarker();
    obj->increase = new ObjIncrease();

    json j;
    to_json(j, *obj, false, LoadType::LS_REF, 1, false, nullptr);

    EXPECT_TRUE(j.contains("compass"));
    EXPECT_TRUE(j.contains("increase"));
}

TEST(ObjectJson, CompassAndIncreaseEmittedForPrototype) {
    ensureConfig();
    ensureServer();
    auto obj = std::make_shared<Object>();
    obj->compass = new MapMarker();
    obj->increase = new ObjIncrease();

    json j;
    to_json(j, *obj, false, LoadType::LS_PROTOTYPE, 1, false, nullptr);

    EXPECT_TRUE(j.contains("compass"));
    EXPECT_TRUE(j.contains("increase"));
}
