cmake_minimum_required(VERSION 2.8)

project(RealmsCode)

set(CMAKE_C_COMPILER "/usr/bin/clang")
set(CMAKE_CXX_COMPILER "/usr/bin/clang++")

set(Python_ADDITIONAL_VERSIONS 3.5)

find_package(LibXml2 REQUIRED)
find_package(ZLIB REQUIRED)
find_package(PythonLibs 3.4 REQUIRED)
find_package(ASPELL REQUIRED)
find_package(PythonInterp)
find_package(PythonLibs)
IF(APPLE)
    find_package(Boost COMPONENTS python system filesystem REQUIRED)
ELSE()
    find_package(Boost COMPONENTS python-py35 system filesystem REQUIRED)
ENDIF()

message(STATUS "PYTHON_LIBRARIES = ${PYTHON_LIBRARIES}")
message(STATUS "PYTHON_EXECUTABLE = ${PYTHON_EXECUTABLE}")
message(STATUS "PYTHON_INCLUDE_DIRS = ${PYTHON_INCLUDE_DIRS}")

message(STATUS "Boost_PYTHON_LIBRARY = ${Boost_PYTHON_LIBRARY}")

include_directories(${PYTHON_INCLUDE_DIRS} ${LIBXML2_INCLUDE_DIR})

# For indexing_suite and such
include_directories(.)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -std=c++14 -g -D_GLIBCXX_DEBUG ")
set(CMAKE_CXX_FLAGS_DEBUG_INIT "-g")

SET(CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} -g" )

SET(REALMS_SOURCE_FILES
    main.cpp
)

set(LIST_SOURCE_FILES
        list.cpp
)

set(COMMON_SOURCE_FILES
    abjuration.cpp
    access.cpp
    action.cpp
    afflictions.cpp
    alchemy.cpp
    alchemy.h
    alignment.cpp
    alphanum.cpp
    alphanum.hpp
    anchor.cpp
    anchor.h
    area.cpp
    area.h
    asynch.cpp
    asynch.h
    attack.cpp
    bank.cpp
    bank.h
    bans.cpp
    bans.h
    bstring.h
    builder.cpp
    calendar.cpp
    calendar.h
    carry.cpp
    carry.h
    catRef.cpp
    catRef.h
    catRefInfo.cpp
    catRefInfo.h
    clans.cpp
    clans.h
    cmd.cpp
    color.cpp
    combat.cpp
    combatSystem.cpp
    command1.cpp
    command2.cpp
    command4.cpp
    command5.cpp
    commands.h
    commerce.cpp
    common.h
    communication.cpp
    communication.h
    config.cpp
    config.h
    conjuration.cpp
    container.cpp
    container.h
    craft.cpp
    craft.h
    creature.cpp
    creature2.cpp
    creatureAttr.cpp
    creatures.cpp
    creatures.h
    creatureStreams.cpp
    creatureStreams.h
    damage.cpp
    damage.h
    data.cpp
    deityData.cpp
    deityData.h
    delayedAction.cpp
    delayedAction.h
    demographics.cpp
    dice.cpp
    dice.h
    die.cpp
    die.h
    divination.cpp
    dm.cpp
    dm.h
    dmcrt.cpp
    dmobj.cpp
    dmply.cpp
    dmroom.cpp
    duel.cpp
    effects.cpp
    effects.h
    enchantment.cpp
    equipment.cpp
    errors.cpp
    evocation.cpp
    exits.cpp
    exits.h
    factions.cpp
    factions.h
    fighters.cpp
    fighters.h
    files-xml-read.cpp
    files-xml-save.cpp
    fishing.cpp
    fishing.h
    flags.cpp
    flags.h
    global.cpp
    global.h
    gods.cpp
    group.cpp
    group.h
    groups.cpp
    guilds.cpp
    guilds.h
    healers.cpp
    healmagic.cpp
    help.cpp
    help.h
    hooks.cpp
    hooks.h
    illusion.cpp
    import.cpp
    import.h
    io.cpp
    lasttime.h
    levelGain.cpp
    levelGain.h
    location.cpp
    location.h
    log.cpp
    logic.cpp
    login.cpp
    login.h
    lottery.cpp
    magic.cpp
    magic.h
    magic1.cpp
    mccp.cpp
    md5.cpp
    md5.h
    memory.cpp
    misc.cpp
    money.cpp
    money.h
    monsters.cpp
    monType.cpp
    monType.h
    mordorMain.cpp
    move.h
    movement.cpp
    msdp.cpp
    msdp.h
    mud.h
    mudObject.cpp
    mudObject.h
    mxp.cpp
    mxp.h
    necromancy.cpp
    object.cpp
    objectAttr.cpp
    objects.cpp
    objects.h
    objIncrease.cpp
    objIncrease.h
    oldQuest.cpp
    os.h
    paths.h
    pets.cpp
    player.cpp
    player.h
    player2.cpp
    playerClass.cpp
    playerClass.h
    players.cpp
    playerTitle.cpp
    playerTitle.h
    post.cpp
    prefs.cpp
    proc.h
    property.cpp
    property.h
    proto.h
    proxy.cpp
    proxy.h
    pymud.h
    pythonHandler.cpp
    pythonHandler.h
    quests.cpp
    quests.h
    queue.cpp
    raceData.cpp
    raceData.h
    range.cpp
    range.h
    realm.h
    realms.cpp
    rogues.cpp
    room.cpp
    rooms.cpp
    rooms.h
    season.h
    security.cpp
    server.cpp
    server.h
    serverTimer.cpp
    serverTimer.h
    ships.cpp
    ships.h
    singers.cpp
    size.cpp
    size.h
    skillCommand.cpp
    skillGain.cpp
    skillGain.h
    skills.cpp
    skills.h
    socials.cpp
    socials.h
    socket.cpp
    socket.h
    songs.cpp
    songs.h
    special1.cpp
    specials.cpp
    specials.h
    spelling.cpp
    spells.cpp
    sql.cpp
    staff.cpp
    startlocs.cpp
    startlocs.h
    statistics.cpp
    statistics.h
    stats.cpp
    stats.h
    steal.cpp
    structs.h
    swap.cpp
    swap.h
    talk.cpp
    threat.cpp
    threat.h
    timer.cpp
    timer.h
    tokenizer.h
    track.cpp
    track.h
    translocation.cpp
    transmutation.cpp
    traps.cpp
    traps.h
    undead.cpp
    unique.cpp
    unique.h
    update.cpp
    utils.h
    version.h
    vprint.cpp
    vprint.h
    wanderInfo.cpp
    wanderInfo.h
    warriors.cpp
    watchers.cpp
    weather.h
    weaponless.cpp
    web.cpp
    web.h
    xml.cpp
    xml.h
        )

add_library(RealmsLib ${COMMON_SOURCE_FILES})
target_link_libraries(RealmsLib ${Boost_LIBRARIES} ${PYTHON_LIBRARIES} ${LIBXML2_LIBRARIES} ${ZLIB_LIBRARIES} ${ASPELL_LIBRARIES})

add_executable(RealmsCode ${REALMS_SOURCE_FILES})
target_link_libraries(RealmsCode RealmsLib)

add_executable(List ${LIST_SOURCE_FILES})
target_link_libraries(List RealmsLib)
