cmake_minimum_required(VERSION 3.15...3.19)

project(RealmsCode)
include(FetchContent)

set(CMAKE_C_COMPILER "/usr/bin/clang")
set(CMAKE_CXX_COMPILER "/usr/bin/clang++")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(DPP_LIB_NAME "dpp")

set(JSON_LIB_NAME "nlohmann_json::nlohmann_json")
set(JSON_MultipleHeaders ON)

set(HTTP_LIB_NAME "Crow::Crow")
set(CROW_FEATURES ssl compression)

set(Python_ADDITIONAL_VERSIONS 3.8)

find_package(LibXml2 REQUIRED)
find_package(ZLIB REQUIRED)
find_package(Python COMPONENTS Interpreter Development REQUIRED)
find_package(PythonLibs 3.8 REQUIRED)
find_package(ASPELL REQUIRED)
find_package(PythonInterp)
find_package(PythonLibs)
find_package(OpenSSL REQUIRED)
find_package(Boost COMPONENTS system filesystem REQUIRED)

message(STATUS "PYTHON_LIBRARIES = ${PYTHON_LIBRARIES}")
message(STATUS "PYTHON_EXECUTABLE = ${PYTHON_EXECUTABLE}")
message(STATUS "PYTHON_INCLUDE_DIRS = ${PYTHON_INCLUDE_DIRS}")

include_directories("include")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -g -stdlib=libstdc++ -fmodules")
set(CMAKE_CXX_FLAGS_DEBUG_INIT "-g")

if($ENV{LEAK})
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O1 -fsanitize=address -fno-omit-frame-pointer -fsanitize-blacklist=${CMAKE_SOURCE_DIR}/blacklist.txt")
ELSE()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0")
ENDIF()

message(STATUS "USING CXX FLAGS = '${CMAKE_CXX_FLAGS}'")

FetchContent_Declare(json     URL https://github.com/nlohmann/json/releases/download/v3.10.5/json.tar.xz    )
FetchContent_Declare(pybind11 GIT_REPOSITORY https://github.com/pybind/pybind11.git     GIT_TAG v2.9.2      )
FetchContent_Declare(crow     GIT_REPOSITORY https://github.com/CrowCpp/Crow.git        GIT_TAG v1.0+1      )
FetchContent_Declare(dpp      GIT_REPOSITORY https://github.com/brainboxdotcc/DPP.git   GIT_TAG v10.0.6     )

FetchContent_MakeAvailable(pybind11 json crow dpp )

include_directories(${PYTHON_INCLUDE_DIRS} ${LIBXML2_INCLUDE_DIR} ${PYBIND11_INCLUDE_DIR} "include")

SET(CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} -g" )

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
    include/free_crt.hpp
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
    include/msdp.hpp
    include/mud.hpp
    include/mxp.hpp
    include/namable.hpp
    include/objIncrease.hpp
    include/oldquest.hpp
    include/os.hpp
    include/paths.hpp
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
    include/utils.hpp
    include/version.hpp
    include/vprint.hpp
    include/wanderInfo.hpp
    include/weather.hpp
    include/web.hpp
    include/xml.hpp
    include/zone.hpp
    )

set(COMMON_SOURCE_FILES
    fmt/format.cc

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
    util/dice.cpp
    util/errors.cpp
    util/help.cpp
    util/md5.cpp
    util/misc.cpp
    util/timer.cpp

    json/catref-json.cpp
    json/money-json.cpp
    json/quests-json.cpp
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
    server/httpServer.cpp)

add_library(RealmsLib ${COMMON_HEADER_FILES} ${COMMON_SOURCE_FILES})

target_link_libraries(
    RealmsLib
    ${Boost_LIBRARIES}
    ${PYTHON_LIBRARIES}
    ${LIBXML2_LIBRARIES}
    ${OPENSSL_LIBRARIES}
    ${ZLIB_LIBRARIES}
    ${ASPELL_LIBRARIES}
    ${DPP_LIB_NAME}
    ${JSON_LIB_NAME}
    ${HTTP_LIB_NAME}
)

add_executable(RealmsCode ${REALMS_SOURCE_FILES})

target_link_libraries(RealmsCode RealmsLib pybind11::embed)

add_executable(List ${LIST_SOURCE_FILES})
target_link_libraries(List RealmsLib)

add_executable(Updater ${UPDATER_SOURCE_FILES})
target_link_libraries(Updater RealmsLib)
