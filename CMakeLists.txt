cmake_minimum_required(VERSION 3.12)

project(RealmsCode)

set(CMAKE_C_COMPILER "/usr/bin/clang-10")
set(CMAKE_CXX_COMPILER "/usr/bin/clang++-10")

set(Python_ADDITIONAL_VERSIONS 3.6)

find_package(LibXml2 REQUIRED)
find_package(ZLIB REQUIRED)
find_package(PythonLibs 3.6 REQUIRED)
find_package(ASPELL REQUIRED)
find_package(PythonInterp)
find_package(PythonLibs)
find_package(Boost COMPONENTS python36 system filesystem REQUIRED)

message(STATUS "PYTHON_LIBRARIES = ${PYTHON_LIBRARIES}")
message(STATUS "PYTHON_EXECUTABLE = ${PYTHON_EXECUTABLE}")
message(STATUS "PYTHON_INCLUDE_DIRS = ${PYTHON_INCLUDE_DIRS}")

message(STATUS "Boost_PYTHON_LIBRARY = ${Boost_PYTHON_LIBRARY}")

include_directories(${PYTHON_INCLUDE_DIRS} ${LIBXML2_INCLUDE_DIR})

# For indexing_suite and such
include_directories("include")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -std=c++14 -g -D_GLIBCXX_DEBUG")
set(CMAKE_CXX_FLAGS_DEBUG_INIT "-g")

if($ENV{LEAK})
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O1 -fsanitize=address -fno-omit-frame-pointer -fsanitize-blacklist=${CMAKE_SOURCE_DIR}/blacklist.txt")
ELSE()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2")
ENDIF()

message(STATUS "USING CXX FLAGS = '${CMAKE_CXX_FLAGS}'")

SET(CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} -g" )

SET(REALMS_SOURCE_FILES
    main/main.cpp
)

set(LIST_SOURCE_FILES
    main/list.cpp
)

set(COMMON_HEADER_FILES
    include/alchemy.hpp
    include/anchor.hpp
    include/area.hpp
    include/asynch.hpp
    include/bank.hpp
    include/bans.hpp
    include/bstring.hpp
    include/calendar.hpp
    include/carry.hpp
    include/catRef.hpp
    include/catRefInfo.hpp
    include/clans.hpp
    include/cmd.hpp
    include/commands.hpp
    include/common.hpp
    include/communication.hpp
    include/config.hpp
    include/container.hpp
    include/craft.hpp
    include/creatures.hpp
    include/creatureStreams.hpp
    include/damage.hpp
    include/deityData.hpp
    include/delayedAction.hpp
    include/dice.hpp
    include/die.hpp
    include/dm.hpp
    include/effects.hpp
    include/exits.hpp
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
    include/lasttime.hpp
    include/levelGain.hpp
    include/location.hpp
    include/login.hpp
    include/magic.hpp
    include/md5.hpp
    include/money.hpp
    include/monType.hpp
    include/move.hpp
    include/msdp.hpp
    include/mud.hpp
    include/mudObject.hpp
    include/mxp.hpp
    include/objects.hpp
    include/objIncrease.hpp
    include/oldquest.hpp
    include/os.hpp
    include/paths.hpp
    include/playerClass.hpp
    include/player.hpp
    include/playerTitle.hpp
    include/proc.hpp
    include/property.hpp
    include/proto.hpp
    include/proxy.hpp
    include/pymud.hpp
    include/pythonHandler.hpp
    include/quests.hpp
    include/queue.hpp
    include/raceData.hpp
    include/range.hpp
    include/realm.hpp
    include/rooms.hpp
    include/season.hpp
    include/server.hpp
    include/serverTimer.hpp
    include/ships.hpp
    include/size.hpp
    include/skillGain.hpp
    include/skills.hpp
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
)

set(COMMON_SOURCE_FILES
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
    
    combat/attack.cpp
    combat/combat.cpp
    combat/combatSystem.cpp
    combat/damage.cpp
    combat/die.cpp
    combat/duel.cpp
    combat/gods.cpp
    combat/singers.cpp
    combat/specials.cpp
    combat/threat.cpp
    combat/undead.cpp
    combat/warriors.cpp
    combat/weaponless.cpp

    commands/action.cpp
    commands/cmd.cpp
    commands/commandBase.cpp
    commands/command2.cpp
    commands/command4.cpp
    commands/command5.cpp
    commands/communication.cpp
    commands/socials.cpp

    crafting/alchemy.cpp
    crafting/craft.cpp
    crafting/fishing.cpp
        
    creatures/afflictions.cpp
    creatures/alignment.cpp
    creatures/carry.cpp
    creatures/creature.cpp
    creatures/creature2.cpp
    creatures/creatureAttr.cpp
    creatures/creatures.cpp
    creatures/monsters.cpp
    creatures/monType.cpp
    creatures/raceData.cpp
    creatures/size.cpp
    creatures/stats.cpp
    
    economy/bank.cpp
    economy/commerce.cpp
    economy/lottery.cpp
    economy/money.cpp
    economy/unique.cpp

    effects/effects.cpp
    
    io/creatureStreams.cpp
    io/color.cpp
    io/files-xml-read.cpp
    io/files-xml-save.cpp
    io/io.cpp
    io/socket.cpp
    io/vprint.cpp
    io/xml.cpp

    main/mordorMain.cpp

    # Magic
    magic/magic.cpp
    magic/magic1.cpp
    magic/realms.cpp
    magic/spells.cpp
    magic/divine/healers.cpp
    magic/divine/healmagic.cpp
    magic/schools/abjuration.cpp
    magic/schools/conjuration.cpp
    magic/schools/divination.cpp
    magic/schools/enchantment.cpp
    magic/schools/evocation.cpp
    magic/schools/illusion.cpp
    magic/schools/necromancy.cpp
    magic/schools/translocation.cpp
    magic/schools/transmutation.cpp

    movement/anchor.cpp
    movement/exits.cpp
    movement/movement.cpp
    movement/ships.cpp
    movement/track.cpp
    
    objects/object.cpp
    objects/objectAttr.cpp
    objects/objects.cpp
    objects/objIncrease.cpp
    objects/special1.cpp
    
    players/clans.cpp
    players/equipment.cpp
    players/guilds.cpp
    players/levelGain.cpp
    players/player.cpp
    players/playerClass.cpp
    players/playerInfo.cpp
    players/player2.cpp
    players/players.cpp
    players/playerTitle.cpp
    players/pets.cpp
    players/post.cpp
    players/prefs.cpp
    players/proxy.cpp
    players/statistics.cpp
    
    groups/group.cpp
    groups/groups.cpp
    
    roguelike/rogues.cpp
    roguelike/steal.cpp
    roguelike/traps.cpp
    
    server/access.cpp
    server/asynch.cpp
    server/bans.cpp
    server/calendar.cpp
    server/container.cpp
    server/config.cpp
    server/delayedAction.cpp
    server/deityData.cpp
    server/demographics.cpp
    server/flags.cpp
    server/global.cpp
    server/hooks.cpp
    server/log.cpp
    server/login.cpp
    server/memory.cpp
    server/mccp.cpp
    server/mudObject.cpp
    server/msdp.cpp
    server/mxp.cpp
    server/pythonHandler.cpp
    server/security.cpp
    server/server.cpp
    server/serverTimer.cpp
    server/swap.cpp
    server/sql.cpp
    server/update.cpp
    server/queue.cpp
    server/web.cpp
    
    skills/skillCommand.cpp
    skills/skillGain.cpp
    skills/skills.cpp
    server/web.cpp
    
    staff/builder.cpp
    staff/dm.cpp
    staff/dmcrt.cpp
    staff/dmobj.cpp
    staff/dmply.cpp
    staff/dmroom.cpp
    staff/spelling.cpp
    staff/staff.cpp
    staff/watchers.cpp
    
    
    questing/factions.cpp
    questing/logic.cpp
    questing/oldQuest.cpp
    questing/quests.cpp
    questing/talk.cpp
        
    util/alphanum.cpp
    util/dice.cpp
    util/help.cpp
    util/errors.cpp
    util/misc.cpp
    util/md5.cpp
    util/timer.cpp
    

    wip/fighters.cpp
    wip/songs.cpp

    )

add_library(RealmsLib ${COMMON_HEADER_FILES} ${COMMON_SOURCE_FILES})
target_link_libraries(RealmsLib ${Boost_LIBRARIES} ${PYTHON_LIBRARIES} ${LIBXML2_LIBRARIES} ${ZLIB_LIBRARIES} ${ASPELL_LIBRARIES})

add_executable(RealmsCode ${REALMS_SOURCE_FILES})
target_link_libraries(RealmsCode RealmsLib)

add_executable(List ${LIST_SOURCE_FILES})
target_link_libraries(List RealmsLib)
