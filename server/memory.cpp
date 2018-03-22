/*
 * memory.cpp
 *   Memory added by Charles Marchant for Mordor 3.x
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

#include "config.hpp"
#include "creatures.hpp"
#include "mud.hpp"
#include "rooms.hpp"
#include "server.hpp"
#include "socket.hpp"

//*********************************************************************
//                      sizeInfo
//*********************************************************************

bstring sizeInfo(long size) {
    int i=0;
    for(; size > 1024 && i < 4; i++)
        size /= 1024;

    switch(i) {
    case 0:
        return((bstring)size + " bytes");
    case 1:
        return((bstring)size + "kb");
    case 2:
        return((bstring)size + "mb");
    case 3:
        return((bstring)size + "gb");
    default:
        return((bstring)size + "tb");
    }
}

//*********************************************************************
//                      showMemory
//*********************************************************************

void Server::showMemory(Socket* sock, bool extended) {
    char buf[80];
    int  crts    = 0;
    int  rooms   = 0;
    int  objects = 0;
    int  talks   = 0;
    int  actions  = 0;
    int  badtalk  = 0;
    long crt_mem    = 0L;
    long rom_mem    = 0L;
    long obj_mem    = 0L;
    long talk_mem   = 0L;
    long act_mem    = 0L;
    //  long total_mem  = 0L;
    long bt_mem     = 0L;
    long total=0;
    ttag    *tlk;
    UniqueRoom* r=0;

    std::map<bstring, rsparse>::iterator it;

    for(auto it : roomCache) {
        r = it.second->second;
        if(!r)
            continue;
        rooms++;
        rom_mem += sizeof(UniqueRoom);
        for(Monster* mons : r->monsters) {
            crts++;
            crt_mem += sizeof(Monster);
            // add object counting on crts
            // and object wear on crts

            if(mons->first_tlk) {
                tlk = mons->first_tlk;
                if(mons->flagIsSet(M_TALKS)) {
                    for(; tlk; tlk = tlk->next_tag) {
                        talks++;
                        talk_mem += sizeof(ttag);
                        if(tlk->key)
                            talk_mem += strlen(tlk->key);
                        if(tlk->response)
                            talk_mem += strlen(tlk->response);
                        if(tlk->action)
                            talk_mem += strlen(tlk->action);
                        if(tlk->target)
                            talk_mem += strlen(tlk->target);
                    }
                } else if(mons->flagIsSet(M_LOGIC_MONSTER)) {
                    for(; tlk; tlk = tlk->next_tag) {
                        actions++;
                        act_mem += sizeof(ttag);
                        if(tlk->response)
                            act_mem += strlen(tlk->response);
                        if(tlk->action)
                            act_mem += strlen(tlk->action);
                        if(tlk->target)
                            act_mem += strlen(tlk->target);
                    }
                } else {
                    sprintf(buf, "%s has a talk and should not.", mons->getCName());
                    loge(buf);
                    for(; tlk; tlk = tlk->next_tag) {
                        badtalk++;
                        bt_mem += sizeof(ttag);
                        if(tlk->key)
                            bt_mem += strlen(tlk->key);
                        if(tlk->response)
                            bt_mem += strlen(tlk->response);
                        if(tlk->action)
                            bt_mem += strlen(tlk->action);
                        if(tlk->target)
                            bt_mem += strlen(tlk->target);
                    }
                }
            }
        }
        objects += r->objects.size();
        obj_mem += r->objects.size() * sizeof(Object);
    }

    total = bt_mem + rom_mem + obj_mem + crt_mem + act_mem + talk_mem;

    sock->print("Memory Status:\n");
    sock->print("Total Rooms  :   %-5d", rooms);
    sock->print("  %ld -> Total memory\n", rom_mem);
    sock->print("Total Objects:   %-5d", objects);
    sock->print("  %ld -> Total memory\n", obj_mem);
    sock->print("Total Creatures: %-5d", crts);
    sock->print("  %ld -> Total memory\n", crt_mem);
    sock->print("Total Actions:   %-5d", actions);
    sock->print("  %ld -> Total memory\n", act_mem);
    sock->print("Total Bad Talks: %-5d", badtalk);
    sock->print("  %ld -> Total memory\n", bt_mem);
    sock->print("Total Talks:     %-5d", talks);
    sock->print("  %ld -> Total memory\n", talk_mem);
    sock->print("Total Memory:    %ld  (%s)\n\n", total, sizeInfo(total).c_str());

    sock->print("\n\n");
    sock->print("Cache Stats:\n");
    sock->print("Room: %s\n", gServer->roomCache.get_stat_info(extended).c_str());
    sock->print("Monster: %s\n", gServer->monsterCache.get_stat_info(extended).c_str());
    sock->print("Object: %s\n", gServer->objectCache.get_stat_info(extended).c_str());
}

//*********************************************************************
//                      dmMemory
//*********************************************************************

int dmMemory(Player* player, cmd* cmnd) {
	bool extended = false;
	if(cmnd->num==2 && !strcmp(cmnd->str[1], "-h"))
		extended = true;

	gServer->showMemory(player->getSock(), extended);
    return(0);
}
