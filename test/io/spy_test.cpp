/*
 * spy_test.cpp
 */
#include <gtest/gtest.h>

#include <memory>

#include "socket.hpp"
#include "login.hpp"            // CON_* states
#include "apiTestSupport.hpp"   // makePlayer, ensureServer

// this->myPlayer null: a spies on b, a loses its player, then a tears down -> b->removeSpy(a)
TEST(Spy, RemoveSpyToleratesNullPlayerOnSpiedSocket) {
    ensureServer();
    auto a = std::make_shared<Socket>(-1);
    auto b = std::make_shared<Socket>(-1);
    a->setPlayer(makePlayer(CreatureClass::FIGHTER));
    b->setPlayer(makePlayer(CreatureClass::FIGHTER));

    a->setSpying(b);            // a spies on b
    b->setSpying(a);            // mutual: b spies on a

    a->clearPlayer();           // a's player gone, as in cleanUp() before the peer tears down
    b->clearSpying();           // locks a, calls a->removeSpy(b); a->myPlayer is null
    SUCCEED();                  // reaching here = no null deref
}

// sock->myPlayer null: the socket invoking clearSpying lost its player first
TEST(Spy, RemoveSpyToleratesNullPlayerOnSpyingSocket) {
    ensureServer();
    auto a = std::make_shared<Socket>(-1);
    auto b = std::make_shared<Socket>(-1);
    a->setPlayer(makePlayer(CreatureClass::FIGHTER));
    b->setPlayer(makePlayer(CreatureClass::FIGHTER));

    a->setSpying(b);            // a spies on b
    a->clearPlayer();           // a (the spy) loses its player
    a->clearSpying();           // locks b, calls b->removeSpy(a); a (sock) ->myPlayer null
    SUCCEED();
}

// Both players present: removeSpy still runs to completion without crashing.
TEST(Spy, RemoveSpyWithBothPlayers) {
    ensureServer();
    auto a = std::make_shared<Socket>(-1);
    auto b = std::make_shared<Socket>(-1);
    a->setPlayer(makePlayer(CreatureClass::FIGHTER));
    b->setPlayer(makePlayer(CreatureClass::FIGHTER));

    a->setSpying(b);
    a->clearSpying();
    SUCCEED();
}
