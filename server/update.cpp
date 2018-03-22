/*
 * update.cpp
 *   Routines to handle game updates
 *   ____            _
 *  |  _ \ ___  __ _| |_ __ ___  ___
 *  | |_) / _ \/ _` | | '_ ` _ \/ __|
 *  |  _ <  __/ (_| | | | | | | \__ \
 *  |_| \_\___|\__,_|_|_| |_| |_|___/
 *
 * Permission to use, modify and distribute is granted via the
 *  GNU Affero General Public License v3 or later
 *
 *  Copyright (C) 2007-2018 Jason Mitchell, Randi Mitchell
 *     Contributions by Tim Callahan, Jonathan Hseu
 *  Based on Mordor (C) Brooke Paul, Brett J. Vickers, John P. Freeman
 *
 */

#include <sys/signal.h>

#include "catRefInfo.hpp"
#include "calendar.hpp"
#include "commands.hpp"
#include "config.hpp"
#include "creatures.hpp"
#include "factions.hpp"
#include "login.hpp"
#include "mud.hpp"
#include "move.hpp"
#include "rooms.hpp"
#include "server.hpp"
#include "ships.hpp"
#include "socket.hpp"
#include "web.hpp"
#include "xml.hpp"

bool            firstLoop=true;
long            last_track_update=0;
long            last_time_update=0;
long            last_weather_update=0;
static long     lastTickUpdate=0;
static long     last_update=0;
static long     last_shutdown_update=0;
static long     last_action_update=0;
long            TX_interval = 4200;
short           Random_update_interval = 6;
short           Action_update_interval = 6;

extern msparse  Mob[MMAX];

// over the course of time, ships begin to become a few seconds
// off from the main clock. this variable will help us keep
// things in check

//*********************************************************************
//                      update_game
//*********************************************************************
// This function handles all the updates that occur while players are
// typing.

void Server::updateGame() {
    long    t = time(0);

    if(t == last_update)
        return;
    last_update = t;

    gServer->parseDelayedActions(t);

    // update on the hour: ie, 3:00
    // Sometimes on startup, we don't get to this section of the code in 1 second,
    // meaning this won't run until 1 hour after the game has started. Throwing in
    // or-firstLoop gives us 2 seconds of time.
    if(!gConfig->currentMinutes() && (!(t%2) || firstLoop))
        update_time(t);

    // Run ships every other second.
    if(t%2)
        update_ships();

    // Prune Dns once a day
    if(t - lastDnsPrune >= 86400)
        pruneDns();
    if(t - lastTickUpdate >= 1) {
        pulseTicks(t);
        pulseCreatureEffects(t);
    }

    // pulse effects
    if(t - lastUserUpdate >= 20) {
        updateUsers(t);
    }
    // same cycle length as creature pulses, but offset 10 seconds
    if(t - lastRoomPulseUpdate >= 20)
        pulseRoomEffects(t);

    if(t - last_track_update >= 20)
        update_track(t);
    if(t - lastRandomUpdate >= Random_update_interval)
        updateRandom(t);
    if(t != lastActiveUpdate)
        updateActive(t);
    if(t - last_weather_update >= 60)
        gServer->updateWeather(t);
    if(t - last_action_update >= Action_update_interval)
        gServer->updateAction(t);
//      update_action(t);
    if(last_dust_output && last_dust_output < t)
        update_dust_oldPrint(t);
    if(t > gConfig->getLotteryRunTime())
        gConfig->runLottery();

    if(Shutdown.ltime && t - last_shutdown_update >= 30)
        if(Shutdown.ltime + Shutdown.interval <= t+500)
            update_shutdown(t);
}


//*********************************************************************
//                      weatherize
//*********************************************************************

bstring Config::weatherize(WeatherString w, const BaseRoom* room) const {
    const CatRefInfo* cri=0;
    const cWeather* weather=0;
    bstring txt = "";

    // get weather data out of catrefinfo
    cri = gConfig->getCatRefInfo(room, 0);
    if(cri) {
        weather = cri->getWeather();
        // if the requested string doesn't exist, default to config weather
        if(weather && weather->get(w) == "")
            weather = 0;
    }

    // try parent
    if(!weather) {
        cri = gConfig->getCatRefInfo(room, 1);
        if(cri) {
            weather = cri->getWeather();
            // if the requested string doesn't exist, default to config weather
            if(weather && weather->get(w) == "")
                weather = 0;
        }
    }

    // if that fails, we grab out of config
    if(!weather)
        weather = gConfig->getWeather();

    if(!weather)
        return("");

    return(weather->get(w));
}

//*********************************************************************
//                      weather
//*********************************************************************
// turn the weather enum into a hook event

bstring weatherToEvent(WeatherString w) {
    switch(w) {
        case WEATHER_SUNRISE:
            return("weatherSunrise");
        case WEATHER_SUNSET:
            return("weatherSunset");

        case WEATHER_EARTH_TREMBLES:
            return("weatherEarthTrembles");
        case WEATHER_HEAVY_FOG:
            return("weatherHeavyFog");

        case WEATHER_BEAUTIFUL_DAY:
            return("weatherBeautifulDay");
        case WEATHER_BRIGHT_SUN:
            return("weatherBrightSun");
        case WEATHER_GLARING_SUN:
            return("weatherGlaringSun");
        case WEATHER_HEAT:
            return("weatherHeat");

        case WEATHER_STILL:
            return("weatherStill");
        case WEATHER_LIGHT_BREEZE:
            return("weatherLightBreeze");
        case WEATHER_STRONG_WIND:
            return("weatherStrongWind");
        case WEATHER_WIND_GUSTS:
            return("weatherWindGusts");
        case WEATHER_GALE_FORCE:
            return("weatherGaleForce");

        case WEATHER_CLEAR_SKIES:
            return("weatherClearSkies");
        case WEATHER_LIGHT_CLOUDS:
            return("weatherLightClouds");
        case WEATHER_THUNDERHEADS:
            return("weatherThunderheads");

        case WEATHER_LIGHT_RAIN:
            return("weatherLightRain");
        case WEATHER_HEAVY_RAIN:
            return("weatherHeavyRain");
        case WEATHER_SHEETS_RAIN:
            return("weatherSheetsRain");
        case WEATHER_TORRENT_RAIN:
            return("weatherTorrentRain");

        case WEATHER_NO_MOON:
            return("weatherNoMoon");
        case WEATHER_SLIVER_MOON:
            return("weatherSliverMoon");
        case WEATHER_HALF_MOON:
            return("weatherHalfMoon");
        case WEATHER_WAXING_MOON:
            return("weatherWaxingMoon");
        case WEATHER_FULL_MOON:
            return("weatherFullMoon");
    }
    return("");
}

//*********************************************************************
//                      weather
//*********************************************************************

void Server::weather(WeatherString w) {
    bstring season = gConfig->calendar->getCurSeason()->getName().toLower();
    const bstring& event = weatherToEvent(w);
    bstring weather;
    char color;

    if(w == WEATHER_SUNRISE)
        color = 'Y';
    else if(w == WEATHER_SUNSET)
        color = 'D';
    else
        color = 'w';


    for(Socket* sock : gServer->sockets) {
        Player* player = sock->getPlayer();

        if(!player)
            continue;
        if(player->fd < 0)
            continue;
        if( (w == WEATHER_SUNRISE || w == WEATHER_SUNSET) &&
            player->isEffected("vampirism")
        ) {
            // if sunrise/sunset, vampires always see it
        } else {
            if(!player->getRoomParent()->isOutdoors())
                continue;
        }

        weather = gConfig->weatherize(w, player->getRoomParent());
        if(weather != "") {
            player->printColor("^%c%s\n", color, weather.c_str());
            player->hooks.execute(event, 0, season);
        }
    }


    MonsterList::iterator it = activeList.begin();
    while(it != activeList.end()) {
        // Increment the iterator in case this monster dies during the update and is removed from the active list
        Monster* monster = (*it++);

        if(!monster)
            continue;

        if( (w == WEATHER_SUNRISE || w == WEATHER_SUNSET) &&
            monster->isEffected("vampirism"))
        {
            // if sunrise/sunset, vampires always see it
        } else {
            if(!monster->getRoomParent()->isOutdoors())
                continue;
        }

        // monsters don't need to see the message, but we need to know if there is a message or not
        // as that will decide whether or not we tell them to execute a hook
        weather = gConfig->weatherize(w, monster->getRoomParent());
        if(weather != "")
            monster->hooks.execute(event, 0, season);
    }
}

//*********************************************************************
//                      update_time
//*********************************************************************
// This function updates the game time in hours. When it is 6am a getSunrise()
// message is broadcast. When it is 8pm a getSunset() message is broadcast.

void update_time(long t) {
    int     daytime=0;
    last_time_update = t;

    // do not do this immeditately on startup,
    // but do everything below this
    if(!firstLoop) {
        if(!gConfig->currentHour())
            gConfig->calendar->advance();
        // save on the hour
        gConfig->calendar->save();
        gConfig->saveShips();
    }
    firstLoop = false;


    daytime = gConfig->currentHour();
    if(daytime == SUNRISE-1) {
        for(std::pair<bstring, Player*> p : gServer->players) {
            if(p.second->isEffected("vampirism"))
                p.second->printColor("^YThe sun is about to rise.\n");
        }
    } else if(daytime == SUNRISE) {
        gServer->weather(WEATHER_SUNRISE);
        gServer->killMortalObjects();
    } else if(daytime == SUNSET-1) {
        for( std::pair<bstring, Player*> p : gServer->players) {
            if(p.second->isEffected("vampirism"))
                p.second->printColor("^DThe sun is about to set.\n");
        }
    } else if(daytime == SUNSET) {
        gServer->weather(WEATHER_SUNSET);
    }

    for(std::pair<bstring, Player*> p : gServer->players) {
        if(p.second->isEffected("vampirism"))
            p.second->computeAC();
    }

    // run at midnight
    if(!daytime && !gServer->isValgrind()) {
        runDemographics();
        gConfig->uniqueDecay();
        gServer->cleanUpAreas();
    }
}

//*********************************************************************
//                      update_shutdown
//*********************************************************************
// This function broadcasts a shutdown message every 30 seconds until
// shutdown is achieved. Then it saves off all rooms and players,
// and exits the game.

void update_shutdown(long t) {
    long    i;
    last_shutdown_update = t;

    i = Shutdown.ltime + Shutdown.interval;

    // fix for strange bug where *shut 0 gives you 1 second
    if(i-t==1)
        i = t;

    if(i > t) {
        if(i-t > 60)
            broadcast("### Game shutdown in %d:%02d minutes.", (i-t)/60L, (i-t)%60L);
        else
            broadcast("### Game shutdown in %d seconds.", i-t);
    } else {
        broadcast("### Shutting down now.");
        gServer->processOutput();
        loge("--- Game shut down ---\n");
        // run swapAbort before rooms/players get saved
        gConfig->swapAbort();
        gServer->resaveAllRooms(1);
        gServer->saveAllPly();
        gServer->disconnectAll();
        gConfig->save();
        cleanUpMemory();

        std::clog << "Goodbye.\n";
        exit(0);
        //kill(getpid(), 9);
    }
}


//*********************************************************************
//                      updateWeather
//*********************************************************************

void Server::updateWeather(long t) {
    int j=0, n=0;

    last_weather_update = t;

    for(j=0; j<5; j++) {
        if(Weather[j].ltime + Weather[j].interval < t) {
            switch (j) {
            case 0:
                if(mrand(1,100) > 80) {
                    n = mrand(0,2);
                    n -= 1;
                    n = Weather[j].misc + n;
                    if(n < 0 || n > 4)
                        n = 0;
                    switch (n) {
                    case 0:
                        Weather[j].ltime = t;
                        weather(WEATHER_EARTH_TREMBLES);
                        break;
                    case 1:
                        Weather[j].ltime = t;
                        weather(WEATHER_HEAVY_FOG);
                        break;
                    }
                    Weather[j].misc = (short)n;
                    Weather[j].interval = 3200;
                }
                break;

            case 1:
                if(mrand(1,100) > 50 && isDay()) {
                    n = mrand(0,2);
                    n -= 1;
                    n = Weather[j].misc + n;
                    if(n < 0 || n > 4)
                        n = 0;
                    switch (n) {
                    case 0:
                        Weather[j].ltime = t;
                        weather(WEATHER_BEAUTIFUL_DAY);
                        break;
                    case 1:
                        Weather[j].ltime = t;
                        weather(WEATHER_BRIGHT_SUN);
                        break;
                    case 3:
                        Weather[j].ltime = t;
                        weather(WEATHER_GLARING_SUN);
                        break;
                    case 4:
                        Weather[j].ltime = t;
                        weather(WEATHER_HEAT);
                        break;
                    }
                    Weather[j].misc = (short)n;
                    Weather[j].interval = 1600;
                }
                break;
            case 2:
                if(mrand(1,100) > 50) {
                    n = mrand(0,2);
                    n -= 1;
                    n = Weather[j].misc +n;
                    if(n< 0 || n> 4)
                        n = 0;
                    switch (n) {
                    case 0:
                        Weather[j].ltime = t;
                        weather(WEATHER_STILL);
                        break;
                    case 1:
                        Weather[j].ltime = t;
                        weather(WEATHER_LIGHT_BREEZE);
                        break;
                    case 2:
                        Weather[j].ltime = t;
                        weather(WEATHER_STRONG_WIND);
                        break;
                    case 3:
                        Weather[j].ltime = t;
                        weather(WEATHER_WIND_GUSTS);
                        break;
                    case 4:
                        Weather[j].ltime = t;
                        weather(WEATHER_GALE_FORCE);
                        break;
                    }
                    Weather[j].misc = (short)n;
                    Weather[j].interval = 900;
                }
                break;
            case 3:
                if(mrand(1,100) > 50) {
                    n = mrand(0,2);
                    n -= 1;
                    n = Weather[j].misc+n;
                    if(n< 0 || n> 6)
                        n = 0;
                    switch (n) {
                    case 0:
                        Weather[j].ltime = t;
                        weather(WEATHER_CLEAR_SKIES);
                        break;
                    case 1:
                        Weather[j].ltime = t;
                        weather(WEATHER_LIGHT_CLOUDS);
                        break;
                    case 2:
                        Weather[j].ltime = t;
                        weather(WEATHER_THUNDERHEADS);
                        break;
                    case 3:
                        Weather[j].ltime =t;
                        weather(WEATHER_LIGHT_RAIN);
                        break;
                    case 4:
                        Weather[j].ltime = t;
                        weather(WEATHER_HEAVY_RAIN);
                        break;
                    case 5:
                        Weather[j].ltime = t;
                        weather(WEATHER_SHEETS_RAIN);
                        break;
                    case 6:
                        Weather[j].ltime = t;
                        weather(WEATHER_TORRENT_RAIN);
                        break;
                    }
                    Weather[j].misc = (short)n;
                    Weather[j].interval = 1100;
                }
                break;
            case 4:
                if(mrand(1,100) > 50 && !isDay()) {
                    n = mrand(0,2);
                    n -= 1;
                    n += Weather[j].misc;
                    if(n< 0 || n> 4)
                        n = 0;
                    switch (n) {
                    case 0:
                        Weather[j].ltime = t;
                        weather(WEATHER_NO_MOON);
                        break;
                    case 1:
                        Weather[j].ltime = t;
                        weather(WEATHER_SLIVER_MOON);
                        break;
                    case 2:
                        Weather[j].ltime = t;
                        weather(WEATHER_HALF_MOON);
                        break;
                    case 3:
                        Weather[j].ltime = t;
                        weather(WEATHER_WAXING_MOON);
                        break;
                    case 4:
                        Weather[j].ltime = t;
                        weather(WEATHER_FULL_MOON);
                        break;
                    }
                    Weather[j].misc = (short)n;
                    Weather[j].interval = 1200;
                }
                break;
            }
        }
    }

    n = 11 - Weather[1].misc - Weather[2].misc - (Weather[3].misc - 2) + Weather[4].misc;
    if(n>25 || n < 2)
        n=11;
    //Random_update_interval = (short)n;
    Random_update_interval = 5;
    return;
}


//*********************************************************************
//                      update_action
//*********************************************************************
// update function for logic scripts

void Server::updateAction(long t) {
    Creature* victim=0;
    Object  *object=0;
    BaseRoom* room=0;
    ttag    *act=0, *tact=0;
    int     i=0, on_cmd=0, thresh=0;
    int     xdir=0, num=0;
    char    *resp;
    const char  *xits[] = { "n","ne","e","se","s","sw","w","nw","u","d" };
    cmd     cmnd;


    last_action_update = t;

    Monster *monster=0;
    MonsterList::iterator it = activeList.begin();
    while(it != activeList.end()) {
        monster = (*it++);
        if(monster) {
            room = monster->getRoomParent();
            if(room && monster->flagIsSet(M_LOGIC_MONSTER)) {
                if(!monster->first_tlk)
                    loadCreature_actions(monster);
                else {
                    act = monster->first_tlk;
                    on_cmd = act->on_cmd;
                    on_cmd--;
                    i = 0;
                    if(on_cmd)
                        while(i < on_cmd) {
                            act = act->next_tag;
                            i++;
                        }
                    on_cmd+=2; // set for next command, can be altered later
                    // proccess commands based on a higharcy
                    if(act)
                    if(act->test_for) {
                        switch(act->test_for) {
                        case 'P': // test for player
                            victim = room->findPlayer(monster, act->response, 1);
                            if(victim) {
                                if(monster->first_tlk->target)
                                    delete[] monster->first_tlk->target;
                                monster->first_tlk->target = new char[strlen(act->response)+1];
                                strcpy(monster->first_tlk->target, act->response);
                                act->success = 1;
                            } else {
                                if(monster->first_tlk->target)
                                    free(monster->first_tlk->target);
                                monster->first_tlk->target = 0;
                                act->success = 0;
                            }
                            break;
                        case 'C': // test for a player with class
                        case 'R': // test for a player with race
                            for(Player* ply : room->players) {
                                if(act->test_for == 'C')
                                    if(static_cast<int>(ply->getClass()) == act->arg1) {
                                        if(monster->first_tlk->target)
                                            delete[] monster->first_tlk->target;
                                        monster->first_tlk->target = new char[ply->getName().length()+1];
                                        strcpy(monster->first_tlk->target, ply->getCName());
                                        act->success = 1;
                                        break;
                                    }
                                if(act->test_for == 'R')
                                    if(ply->getRace() == act->arg1) {
                                        if(monster->first_tlk->target)
                                            delete monster->first_tlk->target;
                                        monster->first_tlk->target = new char[ply->getName().length()+1];
                                        strcpy(monster->first_tlk->target, ply->getCName());
                                        act->success = 1;
                                        break;
                                    }
                            }
                            if(!act->success) {
                                if(monster->first_tlk->target)
                                    delete monster->first_tlk->target;
                                monster->first_tlk->target = 0;
                                act->success = 0;
                            }
                            break;
                        case 'O': // test for object in room
                            object = room->findObject(monster, act->response, 1);

                            if(object) {
                                if(monster->first_tlk->target)
                                    delete monster->first_tlk->target;
                                monster->first_tlk->target = new char[strlen(act->response)+1];
                                strcpy(monster->first_tlk->target, act->response);
                                act->success = 1;
                                // loge(victim->name);
                            } else {
                                if(monster->first_tlk->target)
                                    free(monster->first_tlk->target);
                                monster->first_tlk->target = 0;
                                act->success = 0;
                            }
                            break;
                        case 'o': // test for object on players
                            break;
                        case 'M': // test for monster
                            victim = room->findMonster(monster, act->response, 1);
                            if(victim) {
                                if(monster->first_tlk->target)
                                    delete monster->first_tlk->target;
                                monster->first_tlk->target = new char[strlen(act->response)+1];
                                strcpy(monster->first_tlk->target, act->response);
                                act->success = 1;
                            } else {
                                if(monster->first_tlk->target)
                                    free(monster->first_tlk->target);
                                monster->first_tlk->target = 0;
                                act->success = 0;
                            }
                            break;
                        }

                    }
                    if(act->if_cmd) {
                        // test to see if command was successful
                        for(tact = monster->first_tlk; tact; tact = tact->next_tag) {
                            if(tact->type == act->if_cmd)
                                break;
                        }
                        if(tact) {
                            if(act->if_goto_cmd && tact->success)
                                on_cmd = act->if_goto_cmd;
                            if(act->not_goto_cmd && !tact->success)
                                on_cmd = act->not_goto_cmd;
                        } else {
                            if(act->not_goto_cmd)
                                on_cmd = act->not_goto_cmd;
                        }
                    }
                    // run a action
                    if(act->do_act) {
                        //  num = 0;
                        //  thresh = 0;
                        act->success = 1;

                        resp = act->response;
                        if(isdigit(*(resp))) {

                            num = 10*(atoi(resp));
                            ++resp;
                            num = (num == 0) ? 100:num;

                            thresh = mrand(1,200);

                            //  broadcast(isDm, -1, cp->crt->getRoom(), "*DM* NUM: %d Thresh: %d", num, thresh);

                        }


                        switch(act->do_act) {
                        case 'E': // broadcast response to room
                            if(thresh <= num)
                                broadcast(nullptr, monster->getRoomParent(), "%s", resp);

                            break;
                        case 'S': // say to room
                            if(thresh <= num)
                                broadcast(nullptr, monster->getRoomParent(), "%M says, \"%s\"", monster, resp);
                            break;
                        case 'T':   // Mob Trash-talk
                            if(mrand(1,100) <= 10) {

                                if(countTotalEnemies(monster) > 0 && !monster->getAsMonster()->nearEnemy()) {
                                    if(monster->daily[DL_BROAD].cur > 0) {
                                        broadcast("### %M broadcasted, \"%s\"", monster, resp);
                                        subtractMobBroadcast(monster, 0);
                                    }
                                }
                            }
                            break;
                        case 'B':   // Mob general random broadcasts
                            if(mrand(1,100) <= 10) {

                                if(countTotalEnemies(monster) < 1 && thresh <= num) {
                                    if(monster->daily[DL_BROAD].cur > 0) {
                                        broadcast("### %M broadcasted, \"%s\"", monster, resp);
                                        subtractMobBroadcast(monster, 0);
                                    }
                                }
                            }
                            break;
                        case 'A': // attack monster in target string
                            if(monster->first_tlk->target && !monster->getAsMonster()->hasEnemy()) {
                                victim = room->findMonster(monster, monster->first_tlk->target, 1);
                                if(!victim)
                                    return;
                                victim->getAsMonster()->monsterCombat((Monster*)monster);
                                if(monster->first_tlk->target)
                                    free(monster->first_tlk->target);
                                monster->first_tlk->target = 0;
                            }
                            break;
                        case 'a': // attack player target
                            break;
                        case 'c': // cast a spell on target
                            break;
                        case 'F': // force target to do somthing
                            break;
                        case '|': // set a flag on target
                            break;
                        case '&': // remove flag on target
                            break;
                        case 'P': // perform social
                            break;
                        case 'O': // open door
                            break;
                        case 'C': // close door
                            break;
                        case 'D': // delay action
                            break;
                        case 'G': // go into a keyword exit
                            break;
                        case '0': // go n
                        case '1': // go ne
                        case '2': // go e
                        case '3': // go se
                        case '4': // go s
                        case '5': // go sw
                        case '6': // go w
                        case '7': // go nw
                        case '8': // go up
                        case '9': // go down
                            xdir = act->do_act - '0';
                            strcpy(cmnd.str[0], xits[xdir]);
                            // TODO: Dom: fix update
                                std::clog << "action move\n";
                            //move(creature, &cmnd);
                            break;
                        }
                    }
                    // unconditional jump
                    if(act->goto_cmd) {
                        act->success = 1;
                        monster->first_tlk->on_cmd = act->goto_cmd;
                    } else
                        monster->first_tlk->on_cmd = on_cmd;
                }
            }
        }
    }
}

//*********************************************************************
//                      update_dust_oldPrint
//*********************************************************************

void update_dust_oldPrint(long t) {
    last_dust_output=0;
    broadcast("Ominous thunder rumbles in the distance.");
}

//*********************************************************************
//                      handle_pipe
//*********************************************************************

void handle_pipe( int sig) {
    // What should we do here?
    // Should just continue on
}

//*********************************************************************
//                      doCrash
//*********************************************************************

void doCrash(int sig) {
    long t = time(0);
    std::ostringstream oStr;
    Crash++;
    if(Crash > 1) exit(-12);

    // Uninstall signal handlers so we don't get back here again
    signal(SIGALRM, SIG_DFL);
    signal(SIGFPE, SIG_DFL);
    signal(SIGSEGV, SIG_DFL);
    gServer->sendCrash();

    gConfig->save();
    gServer->saveIds();
    broadcast("                   C R A S H !\n");
    broadcast(isStaff, "%s", ctime(&t));

    oStr << "People online: ";
    std::pair<bstring, Player*> p;
    Player* player=0;
    int i=0;
    for(Socket* sock : gServer->sockets) {
        player = sock->getPlayer();
        if(!player)
            continue;
        if(i++)
            oStr << ", ";

        if(!sock->isConnected())
            oStr << player->getName() << " (connecting)";
        else
            oStr << player->getName() << " (^xcmd:" + player->getLastCommand() << ":" << (t-sock->ltime) << "^W)";
    }
    if(!i)
        oStr << "No one";
    oStr << ".";
    broadcast(isDm, "^W%s", oStr.str().c_str());
    gServer->processOutput();

    logn("log.inbytes", "--- %ld\n", InBytes);
    logn("log.outbytes", "-- %ld\n", OutBytes);
    logn("log.crash","--- !CRASH! Game closed ---\n");
    loge("--- !CRASH! Game closed ---\n");
// for some reason, on cygwin this will (sometimes) corrupt room files
#ifndef __CYGWIN__
// Turning this off because of the xp->ext = nullptr bug which is erasing exits
//  gConfig->resaveAllRooms(1);
#endif
    gServer->saveAllPly();

    cleanUpMemory();


    oStr << "\n";
    //delimit(who);
    logn("log.crash", oStr.str().c_str());

    std::clog << "The mud has crashed :(.\n";

    if(sig != -69) {
//      char dbx[160];
//
//      sprintf(dbx, "gstack %d > realms.%d.dump", getpid(), getpid());
//      system(dbx);
//
    //  abort();
        signal(sig, SIG_DFL);
        kill(getpid(), sig);
    } else {
        // Crashing from *bane, don't bother with core dumps or backtraces
        exit(sig);
    }
}

//*********************************************************************
//                      crash
//*********************************************************************

void crash(int sig) {
    if(gConfig->sendTxtOnCrash())
        webCrash("");
    doCrash(sig);
}

void cleanup_spelling(void);

//*********************************************************************
//                      cleanUpMemory
//*********************************************************************
// Clean up all memory we know is in use -- this way valgrind and any other
// tools can accurately find memory leaks

void cleanUpMemory() {
    gConfig->clearQuestTable();

    gServer->flushRoom();
    gServer->flushObject();
    gServer->flushMonster();
    //flush_ext();

    Config::destroyInstance();
    Server::destroyInstance();
    // Clean up xml memory
    xmlCleanupParser();
    cleanup_spelling();
    // TODO: Clean up spelling memory

    std::clog << "Memory cleaned up.\n";
}

//*********************************************************************
//                      subtractMobBroadcast
//*********************************************************************

void subtractMobBroadcast(Creature *monster, int num) {
    if(monster->daily[DL_BROAD].cur > 0) {
        if(num)
            monster->daily[DL_BROAD].cur -= num;
        else
            monster->daily[DL_BROAD].cur --;
    }
}

//*********************************************************************
//                      countTotalEnemies
//*********************************************************************

int countTotalEnemies(Creature *monster) {
    if(monster->isPlayer())
        return(0);
    Monster* mons = monster->getAsMonster();
    return(mons->threatTable->size());
}

//*********************************************************************
//                      clearEnemyPlayer
//*********************************************************************
// This function removes a player's name from every single monster
// on the active list when the player dies, suicides, or is dusted.

void Server::clearAsEnemy(Player* player) {

    Monster *monster=0;
    MonsterList::iterator it = activeList.begin();
    while(it != activeList.end()) {
        // Increment the iterator in case this monster dies during the update and is removed from the active list
        monster = (*it++);

        if(!(monster->inUniqueRoom() && monster->getUniqueRoomParent()->info.id) && !monster->inAreaRoom()) continue;

        monster->clearEnemy(player);
    }

    return;
}

//*********************************************************************
//                      dmInRoom
//*********************************************************************
// This function will return 1 if anyone is in the room while dmInvis

int BaseRoom::dmInRoom() const {
    for(Player* ply : players) {
        if(ply->flagIsSet(P_DM_INVIS))
            return(1);
    }
    return(0);
}

//*********************************************************************
//                      checkOutlawAggro
//*********************************************************************

void Player::checkOutlawAggro() {
    if(!flagIsSet(P_OUTLAW_WILL_BE_ATTACKED))
        return;

    for(Monster* mons : getRoomParent()->monsters) {
        if(mons->flagIsSet(M_OUTLAW_AGGRO) && !mons->getAsMonster()->hasEnemy()) {
            mons->updateAttackTimer(true, DEFAULT_WEAPON_DELAY);
            mons->getAsMonster()->addEnemy(this, true);
        }
    }
}


//*********************************************************************
//                      update_track
//*********************************************************************

void update_track(long t) {
    std::list<Area*>::iterator it;

    if(last_track_update)
        for(it = gServer->areas.begin() ; it != gServer->areas.end() ; it++)
            (*it)->updateTrack((int)t - last_track_update);

    last_track_update = t;
}

//*********************************************************************
//                      update_ships
//*********************************************************************

void update_ships(int n) {
    std::list<Ship*>::iterator it;
    Ship    *ship=0;
    ShipStop *stop=0;

    // we must delay ships for a while
    if(gConfig->calendar->shipUpdates > gConfig->expectedShipUpdates())
        return;
    gConfig->calendar->shipUpdates++;

    for(it = gConfig->ships.begin() ; it != gConfig->ships.end() ; it++) {
        ship = (*it);
        ship->timeLeft--;
        stop = ship->stops.front();
        // only do last call if we're in port
        if(ship->inPort && ship->timeLeft==60)
            shipBroadcastRange(ship, stop, stop->lastcall);
        // while loop because a ship might take 0 time to get to the next stop
        while(ship->timeLeft <= 0) {
            if(ship->inPort) {
                ship->timeLeft = stop->to_next;
                shipDeleteExits(ship, stop);

                ship->stops.push_back(stop);
                ship->stops.pop_front();
            } else {
                ship->timeLeft = stop->in_dock;
                shipSetExits(ship, stop);
            }
            stop = ship->stops.front();
            ship->inPort = !ship->inPort;
        }
    }

    // we haven't updated enough!
    if(gConfig->calendar->shipUpdates < gConfig->expectedShipUpdates())
    	if(n > 100) {
    		broadcast(isDm, "Runaway ships!! Current updates: %d Expected updated: %d", gConfig->calendar->shipUpdates, gConfig->expectedShipUpdates());
    		gConfig->calendar->shipUpdates =  gConfig->expectedShipUpdates();
    	}
        update_ships(n+1);
}


//*********************************************************************
//                          list_act
//*********************************************************************
bstring Server::showActiveList() {
    Monster* monster;
    MonsterList::iterator it = activeList.begin();
    std::ostringstream oStr;
    oStr << "### Active monster list ###\n";
    oStr << "Monster    -    Room Number\n";

    while(it != activeList.end()) {
        monster = (*it++);
        if((!monster->inUniqueRoom() || !monster->getUniqueRoomParent()->info.id) && !monster->inAreaRoom()) {
            if(monster->getName()[0])
                oStr << "Bad Mob " << monster->getName() << ".\n";
            else
                oStr << "Bad Mob\n";
            continue;
        }
        if(!monster->getName()[0]) {
            oStr << "Bad Mob - Room " << monster->getRoomParent()->fullName() << "\n";
            continue;
        }
        oStr << monster->getName() << " - " << monster->getRoomParent()->fullName() << "\n";
    }
    return(oStr.str());

}
int list_act(Player* player, cmd* cmnd) {
    player->print("%s", gServer->showActiveList().c_str());
    gServer->processOutput();
    return(0);
}
