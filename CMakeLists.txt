cmake_minimum_required(VERSION 3.16...3.26)

project(RealmsCode)
include(FetchContent)

list(PREPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

if(APPLE AND NOT CMAKE_OSX_DEPLOYMENT_TARGET)
    execute_process(COMMAND sw_vers -productVersion
        OUTPUT_VARIABLE REALMS_MACOS_VERSION OUTPUT_STRIP_TRAILING_WHITESPACE)
    string(REGEX MATCH "^[0-9]+" REALMS_MACOS_MAJOR "${REALMS_MACOS_VERSION}")
    set(CMAKE_OSX_DEPLOYMENT_TARGET "${REALMS_MACOS_MAJOR}" CACHE STRING "macOS deployment target" FORCE)
    message(STATUS "macOS deployment target: ${CMAKE_OSX_DEPLOYMENT_TARGET}")
endif()

set(CMAKE_CXX_STANDARD 20 CACHE STRING "The C++ standard to use")
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(DPP_LIB_NAME "dpp")
set(FMT_LIB_NAME "fmt::fmt")

set(JSON_LIB_NAME "nlohmann_json::nlohmann_json")
set(JSON_MultipleHeaders ON)

set(HTTP_LIB_NAME "Crow::Crow")
set(CROW_FEATURES ssl compression)

set(JWT_LIB_NAME "jwt-cpp")

find_package(LibXml2 REQUIRED)
find_package(ZLIB REQUIRED)
find_package(Python 3.8 COMPONENTS Interpreter Development REQUIRED)
find_package(ASPELL REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(Threads REQUIRED)
find_package(Boost CONFIG REQUIRED COMPONENTS regex)

message(STATUS "Python_LIBRARIES = ${Python_LIBRARIES}")
message(STATUS "Python_EXECUTABLE = ${Python_EXECUTABLE}")
message(STATUS "Python_INCLUDE_DIRS = ${Python_INCLUDE_DIRS}")


# -stdlib is an ABI choice and must match the fetched deps, so it stays global. Everything
# else (-g/-O0/-Wall/sanitizer) lives on the realms_flags target below so it hits only our code.
# macOS clang has no libstdc++; it defaults to libc++
if(NOT APPLE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libstdc++")
endif()

# Warning flags for our code only - applied via realms_flags below
#  -Wall                                baseline.
#  -Wno-vla-cxx-extension               VLAs are a GCC/Clang extension; sizes used here are bounded.
#  -Wno-undefined-bool-conversion       legacy null-`this` guards; UB on paper but they have caught
#  -Wno-tautological-undefined-compare  real bugs, and at -O0 the compiler keeps them. Revisit if -O on.
set(REALMS_WARNINGS -Wall -Wno-vla-cxx-extension -Wno-undefined-bool-conversion -Wno-tautological-undefined-compare)

# Our compile/link flags. Linked PRIVATE into our four targets (below) so -g/-O0/-Wall and the
# sanitizer never reach the fetched deps. Keep -O0/-g for debug visibility + core dumps for now.
add_library(realms_flags INTERFACE)
target_compile_options(realms_flags INTERFACE -g -O0 ${REALMS_WARNINGS})
if($ENV{LEAK})
    target_compile_options(realms_flags INTERFACE
        -fsanitize=address,signed-integer-overflow,null,alignment,undefined
        -fno-omit-frame-pointer
        -fsanitize-blacklist=${CMAKE_SOURCE_DIR}/blacklist.txt)
    target_link_options(realms_flags INTERFACE
        -fsanitize=address,signed-integer-overflow,null,alignment,undefined)
    message(STATUS "Enabling address sanitizer")
endif()

FetchContent_Declare(nlohmann_json URL https://github.com/nlohmann/json/releases/download/v3.12.0/json.tar.xz    SYSTEM)
FetchContent_Declare(fmt         GIT_REPOSITORY https://github.com/fmtlib/fmt.git          GIT_TAG 12.1.0   SYSTEM)
FetchContent_Declare(pybind11    GIT_REPOSITORY https://github.com/pybind/pybind11.git     GIT_TAG v3.0.4   SYSTEM)
FetchContent_Declare(crow        GIT_REPOSITORY https://github.com/CrowCpp/Crow.git        GIT_TAG v1.2.1.2 SYSTEM)
FetchContent_Declare(dpp         GIT_REPOSITORY https://github.com/brainboxdotcc/DPP.git   GIT_TAG v10.1.5  SYSTEM)
FetchContent_Declare(jwt-cpp     GIT_REPOSITORY https://github.com/Thalhammer/jwt-cpp.git  GIT_TAG v0.7.2   SYSTEM)
FetchContent_Declare(googletest  GIT_REPOSITORY https://github.com/google/googletest.git   GIT_TAG v1.17.0  SYSTEM)


FetchContent_MakeAvailable(pybind11 fmt nlohmann_json crow dpp jwt-cpp googletest)

# DPP's bundled nlohmann/json declares `operator "" _json` (old spelling), which DPP's own build
# flags warn on under clang. Third-party noise; silence just that warning for the DPP target.
if(TARGET dpp)
    target_compile_options(dpp PRIVATE -Wno-deprecated-literal-operator)
endif()

SET(REALMS_SOURCE_FILES
    main/main.cpp
)

set(LIST_SOURCE_FILES
    main/list.cpp
)

set(UPDATER_SOURCE_FILES
    main/updater.cpp
    )

set(COMMON_HEADER_FILES

    include/builders/alchemyBuilder.hpp
    include/builders/effectBuilder.hpp
    include/builders/fluentBuilder.hpp
    include/builders/fluentBuilderEnd.hpp
    include/builders/msdpBuilder.hpp
    include/builders/mxpBuilder.hpp
    include/builders/skillBuilder.hpp
    include/builders/songBuilder.hpp

    include/mudObjects/areaRooms.hpp
    include/mudObjects/container.hpp
    include/mudObjects/creatures.hpp
    include/mudObjects/exits.hpp
    include/mudObjects/monsters.hpp
    include/mudObjects/mudObject.hpp
    include/mudObjects/objects.hpp
    include/mudObjects/players.hpp
    include/mudObjects/rooms.hpp
    include/mudObjects/uniqueRooms.hpp

    include/account.hpp
    include/alchemy.hpp
    include/anchor.hpp
    include/area.hpp
    include/async.hpp
    include/bank.hpp
    include/bans.hpp
    include/calendar.hpp
    include/carry.hpp
    include/catRef.hpp
    include/catRefInfo.hpp
    include/clans.hpp
    include/cmd.hpp
    include/color.hpp
    include/commands.hpp
    include/communication.hpp
    include/config.hpp
    include/craft.hpp
    include/creatureStreams.hpp
    include/damage.hpp
    include/deityData.hpp
    include/delayedAction.hpp
    include/dice.hpp
    include/dm.hpp
    include/effects.hpp
    include/enums/bits.hpp
    include/enums/loadType.hpp
    include/factions.hpp
    include/fighters.hpp
    include/fishing.hpp
    include/flags.hpp
    include/global.hpp
    include/group.hpp
    include/guilds.hpp
    include/help.hpp
    include/hooks.hpp
    include/import.hpp
    include/json.hpp
    include/lasttime.hpp
    include/levelGain.hpp
    include/location.hpp
    include/login.hpp
    include/magic.hpp
    include/md5.hpp
    include/monType.hpp
    include/money.hpp
    include/move.hpp
    include/gmcp.hpp
    include/gmcpEvents.hpp
    include/msdp.hpp
    include/mud.hpp
    include/mxp.hpp
    include/namable.hpp
    include/objIncrease.hpp
    include/oldquest.hpp
    include/paths.hpp
    include/platform.hpp
    include/playerClass.hpp
    include/playerTitle.hpp
    include/post.hpp
    include/proc.hpp
    include/property.hpp
    include/proto.hpp
    include/proxy.hpp
    include/pythonHandler.hpp
    include/quests.hpp
    include/raceData.hpp
    include/random.hpp
    include/range.hpp
    include/realm.hpp
    include/season.hpp
    include/security.hpp
    include/server.hpp
    include/serverTimer.hpp
    include/ships.hpp
    include/size.hpp
    include/skills.hpp
    include/skillGain.hpp
    include/skillCommand.hpp
    include/socials.hpp
    include/socket.hpp
    include/songs.hpp
    include/specials.hpp
    include/startlocs.hpp
    include/statistics.hpp
    include/stats.hpp
    include/structs.hpp
    include/swap.hpp
    include/threat.hpp
    include/timer.hpp
    include/toNum.hpp
    include/tokenizer.hpp
    include/track.hpp
    include/traps.hpp
    include/unique.hpp
    include/version.hpp
    include/vprint.hpp
    include/wanderInfo.hpp
    include/weather.hpp
    include/web.hpp
    include/xml.hpp
    include/zone.hpp
    include/httpServer.hpp
    include/apiQueue.hpp
    include/commerce.hpp
    )

set(COMMON_SOURCE_FILES
    accounts/account.cpp

    areas/area.cpp
    areas/catRef.cpp
    areas/catRefInfo.cpp
    areas/location.cpp
    areas/property.cpp
    areas/range.cpp
    areas/room.cpp
    areas/rooms.cpp
    areas/startlocs.cpp
    areas/wanderInfo.cpp
    areas/zone.cpp

    bards/creatureSongs.cpp
    bards/identify.cpp
    bards/songLoader.cpp
    bards/songs.cpp

    combat/attack.cpp
    combat/combat.cpp
    combat/combatSystem.cpp
    combat/damage.cpp
    combat/die.cpp
    combat/duel.cpp
    combat/singers.cpp
    combat/specials.cpp
    combat/threat.cpp
    combat/undead.cpp
    combat/warriors.cpp
    combat/weaponless.cpp

    commands/action.cpp
    commands/cmd.cpp
    commands/command2.cpp
    commands/command4.cpp
    commands/command5.cpp
    commands/commandBase.cpp
    commands/communication.cpp
    commands/socials.cpp

    crafting/alchemy/alchemy.cpp
    crafting/alchemy/alchemyLoader.cpp
    crafting/craft.cpp
    crafting/fishing.cpp

    creatures/afflictions.cpp
    creatures/alignment.cpp
    creatures/carry.cpp
    creatures/creature.cpp
    creatures/creature2.cpp
    creatures/creatureAttr.cpp
    creatures/creatureEffects.cpp
    creatures/creatureSkills.cpp
    creatures/creatures.cpp
    creatures/inventory.cpp
    creatures/lasttime.cpp
    creatures/monType.cpp
    creatures/monsters.cpp
    creatures/raceData.cpp
    creatures/size.cpp
    creatures/stats.cpp

    economy/bank.cpp
    economy/commerce.cpp
    economy/playerCommerce.cpp
    economy/lottery.cpp
    economy/money.cpp
    economy/unique.cpp

    effects/effects.cpp
    effects/effectsLoader.cpp


    groups/group.cpp
    groups/groups.cpp

    io/color.cpp
    io/creatureStreams.cpp
    io/io.cpp
    io/socket.cpp
    io/vprint.cpp

    magic/divine/healers.cpp
    magic/divine/healmagic.cpp
    magic/magic.cpp
    magic/magic1.cpp
    magic/realms.cpp
    magic/schools/abjuration.cpp
    magic/schools/conjuration.cpp
    magic/schools/divination.cpp
    magic/schools/enchantment.cpp
    magic/schools/evocation.cpp
    magic/schools/illusion.cpp
    magic/schools/necromancy.cpp
    magic/schools/translocation.cpp
    magic/schools/transmutation.cpp
    magic/spells.cpp

    main/mordorMain.cpp

    movement/anchor.cpp
    movement/exits.cpp
    movement/movement.cpp
    movement/ships.cpp
    movement/track.cpp

    objects/objIncrease.cpp
    objects/object.cpp
    objects/objectAttr.cpp
    objects/objects.cpp
    objects/special1.cpp

    players/clans.cpp
    players/equipment.cpp
    players/guilds.cpp
    players/levelGain.cpp
    players/pets.cpp
    players/player.cpp
    players/playerClass.cpp
    players/playerEffects.cpp
    players/playerInfo.cpp
    players/playerTitle.cpp
    players/players.cpp
    players/post.cpp
    players/prefs.cpp
    players/prompt.cpp
    players/proxy.cpp
    players/statistics.cpp

    questing/factions.cpp
    questing/logic.cpp
    questing/oldQuest.cpp
    questing/quests.cpp
    questing/talk.cpp

    roguelike/rogues.cpp
    roguelike/steal.cpp
    roguelike/traps.cpp
    roguelike/cards.cpp
    roguelike/blackjack.cpp

    server/access.cpp
    server/async.cpp
    server/bans.cpp
    server/calendar.cpp
    server/config.cpp
    server/container.cpp
    server/deityData.cpp
    server/delayedAction.cpp
    server/demographics.cpp
    server/discordBot.cpp
    server/flags.cpp
    server/global.cpp
    server/hooks.cpp
    server/log.cpp
    server/login.cpp
    server/mccp.cpp
    server/gmcp.cpp
    server/gmcpEvents.cpp
    server/memory.cpp
    server/msdp.cpp
    server/msdpLoader.cpp
    server/mxpLoader.cpp
    server/mudObject.cpp
    server/mxp.cpp
    server/pythonHandler.cpp
    server/queue.cpp
    server/security.cpp
    server/server.cpp
    server/serverTimer.cpp
    server/sql.cpp
    server/swap.cpp
    server/update.cpp

    server/web.cpp
    server/httpServer.cpp
    server/apiQueue.cpp
    server/apiAuthCache.cpp
    server/apiQuests.cpp
    server/zoneIndex.cpp
    server/zoneIndexBuilder.cpp
    server/http/zones-http.cpp
    server/http/index-http.cpp
    server/http/auth-http.cpp


    skills/skillCommand.cpp
    skills/skillGain.cpp
    skills/skillLoader.cpp
    skills/skills.cpp

    staff/builder.cpp
    staff/catref-staff.xml.cpp
    staff/dm.cpp
    staff/dmcrt.cpp
    staff/dmobj.cpp
    staff/dmply.cpp
    staff/dmroom.cpp
    staff/spelling.cpp
    staff/staff.cpp
    staff/watchers.cpp

    util/alphanum.cpp
    util/csv.cpp
    util/dice.cpp
    util/help.cpp
    util/md5.cpp
    util/misc.cpp
    util/platform.cpp
    util/timer.cpp

    json/accounts-json.cpp
    json/catref-json.cpp
    json/effects-json.cpp
    json/hooks-json.cpp
    json/money-json.cpp
    json/mudObject-json.cpp
    json/objects-json.cpp
    json/quests-json.cpp
    json/rooms-json.cpp
    json/monsters-json.cpp
    json/realms-json.cpp
    json/zones-json.cpp

    xml/alchemy-xml.cpp
    xml/area-xml.cpp
    xml/bans-xml.cpp
    xml/calendar-xml.cpp
    xml/catref-xml.cpp
    xml/clans-xml.cpp
    xml/config-xml.cpp
    xml/craft-xml.cpp
    xml/creatures-xml.cpp
    xml/dice-xml.cpp
    xml/diety-xml.cpp
    xml/effects-xml.cpp
    xml/exits-xml.cpp
    xml/factions-xml.cpp
    xml/files-xml-read.cpp
    xml/files-xml-save.cpp
    xml/fishing-xml.cpp
    xml/guilds-xml.cpp
    xml/hooks-xml.cpp
    xml/location-xml.cpp
    xml/monsters-xml.cpp
    xml/objects-xml.cpp
    xml/players-xml.cpp
    xml/property-xml.cpp
    xml/quests-xml.cpp
    xml/range-xml.cpp
    xml/rooms-xml.cpp
    xml/ships-xml.cpp
    xml/skills-xml.cpp
    xml/socials-xml.cpp
    xml/specials-xml.cpp
    xml/spells-xml.cpp
    xml/startlocs-xml.cpp
    xml/statistics-xml.cpp
    xml/stats-xml.cpp
    xml/wanderInfo-xml.cpp
    xml/xml.cpp

    python/mud.cpp
    python/mudEffect.cpp
    python/mudObject.cpp
    python/mudStat.cpp

    wip/fighters.cpp
        )

add_library(RealmsLib ${COMMON_HEADER_FILES} ${COMMON_SOURCE_FILES})

target_include_directories(RealmsLib PUBLIC include)
target_include_directories(RealmsLib PRIVATE ${ASPELL_INCLUDE_DIRS})
target_compile_definitions(RealmsLib PUBLIC REALMS_SOURCE_DIR="${CMAKE_SOURCE_DIR}")

if(APPLE)
    target_compile_definitions(RealmsLib PUBLIC BOOST_STACKTRACE_GNU_SOURCE_NOT_REQUIRED)
endif()

target_link_libraries(
    RealmsLib
    PUBLIC
    Boost::regex
    LibXml2::LibXml2
    pybind11::embed
    ${OPENSSL_LIBRARIES}
    ${ZLIB_LIBRARIES}
    ${ASPELL_LIBRARIES}
    ${DPP_LIB_NAME}
    ${JSON_LIB_NAME}
    ${HTTP_LIB_NAME}
    ${JWT_LIB_NAME}
    ${FMT_LIB_NAME}
)
target_link_libraries(RealmsLib PRIVATE realms_flags)

add_executable(RealmsCode ${REALMS_SOURCE_FILES})
target_link_libraries(RealmsCode PRIVATE RealmsLib realms_flags)

add_executable(List ${LIST_SOURCE_FILES})
target_link_libraries(List PRIVATE RealmsLib realms_flags)

add_executable(Updater ${UPDATER_SOURCE_FILES})
target_link_libraries(Updater PRIVATE RealmsLib realms_flags)

if(APPLE)
    set_target_properties(RealmsCode List Updater PROPERTIES BUILD_RPATH "@loader_path")
endif()

# Enable with -DREALMS_BUILD_TESTS=ON, then `ctest` or ./RealmsTests.
option(REALMS_BUILD_TESTS "Build unit tests (gtest)" OFF)
if(REALMS_BUILD_TESTS)
    enable_testing()
    add_executable(RealmsTests
        # REST API (server/http)
        test/api/apiQuests_test.cpp
        test/api/apiQueue_test.cpp
        test/api/authCache_test.cpp
        test/api/endpoint_test.cpp
        # JSON persistence (json/)
        test/json/questsJson_test.cpp
        test/json/monstersJson_test.cpp
        test/json/roomsJson_test.cpp
        test/json/objectsJson_test.cpp
        test/json/zoneIndex_test.cpp
        test/json/zoneIndexBuilder_test.cpp
        test/json/questRemap_test.cpp
        # XML helpers (xml/)
        test/xml/xmlNum_test.cpp
        test/xml/xmlReader_test.cpp
        # Telnet protocol + output (server/, io/)
        test/io/protocol_test.cpp
        test/io/processInput_test.cpp
        test/io/colorWidth_test.cpp
        test/io/vprintFormat_test.cpp
        test/io/gmcp_test.cpp
        test/io/spy_test.cpp
        test/io/mccp_test.cpp
        test/io/viewFileReverse_test.cpp
        # General utilities (util/)
        test/util/csv_test.cpp
        test/util/range_test.cpp
        test/util/md5_test.cpp
        # Game logic
        test/game/commandBase_test.cpp
        test/game/startlocs_test.cpp
    )
    set_property(SOURCE io/socket.cpp APPEND PROPERTY COMPILE_DEFINITIONS _GLIBCXX_ASSERTIONS)
    target_include_directories(RealmsTests PRIVATE ${CMAKE_SOURCE_DIR}/test)
    target_link_libraries(RealmsTests PRIVATE RealmsLib gtest_main realms_flags)
    add_test(NAME RealmsTests COMMAND RealmsTests)
endif()
