/*
 * talk.cpp
 *   Creature talk routines.
 *   ____            _
 *  |  _ \ ___  __ _| |_ __ ___  ___ 
 *  | |_) / _ \/ _` | | '_ ` _ \/ __|
 *  |  _ <  __/ (_| | | | | | | \__ \
 *  |_| \_\___|\__,_|_|_| |_| |_|___/
 *
 * Permission to use, modify and distribute is granted via the
 *  GNU Affero General Public License v3 or later
 *  
 *  Copyright (C) 2007-2020 Jason Mitchell, Randi Mitchell
 *     Contributions by Tim Callahan, Jonathan Hseu
 *  Based on Mordor (C) Brooke Paul, Brett J. Vickers, John P. Freeman
 *
 */

#include <cctype>         // for isalpha, isdigit, isspace
#include <cstdio>         // for fgets, fclose, feof, fopen, sprintf, FILE
#include <cstring>        // for strcmp, strcpy, strlen, memcpy

#include "creatures.hpp"  // for Creature
#include "flags.hpp"      // for M_TALKS
#include "global.hpp"     // for FATAL
#include "os.hpp"         // for merror
#include "paths.hpp"      // for Talk
#include "proto.hpp"      // for loadCreature_tlk, talk_crt_act
#include "structs.hpp"    // for ttag

//***********************************************************************
//              loadCreature_tlk
//***********************************************************************
// TODO: Redo this into XML
// This function loads a creature's talk responses if they exist.

int loadCreature_tlk( Creature* creature ) {
    char    crt_name[80], path[256];
    char    keystr[80], responsestr[1024];
    int i, len1, len2;
    ttag    *tp, **prev;
    FILE    *fp;

    if(!creature->flagIsSet(M_TALKS) || creature->first_tlk)
        return(0);

    strcpy(crt_name, creature->getCName());
    for(i=0; crt_name[i]; i++)
        if(crt_name[i] == ' ')
            crt_name[i] = '_';

    sprintf(path, "%s/%s-%d.txt", Path::Talk, crt_name, creature->getLevel());
    fp = fopen(path, "r");
    if(!fp)
        return(0);

    i = 0;
    prev = &creature->first_tlk;
    while(!feof(fp)) {
        fgets(keystr, 80, fp);
        len1 = strlen(keystr);
        if(!len1)
            break;
        keystr[len1-1] = 0;
        fgets(responsestr, 1024, fp);
        len2 = strlen(responsestr);
        if(!len2)
            break;
        responsestr[len2-1] = 0;

        i++;

        tp = new ttag;
        if(!tp)
            merror("loadCreature_tlk", FATAL);
        // LEAK: Next line reported to be leaky: 21 count
        tp->key = new char[len1];
        if(!tp->key)
            merror("loadCreature_tlk", FATAL);
        tp->response = new char[len2];
        if(!tp->response)
            merror("loadCreature_tlk", FATAL);
        tp->next_tag = nullptr;

        strcpy(tp->key, keystr);
        talk_crt_act(keystr,tp);
        strcpy(tp->response, responsestr);

        *prev = tp;
        prev = &tp->next_tag;
    }

    fclose(fp);
    return(i);
}


//*********************************************************************
//                      talk_crt_act
//*********************************************************************
// the talk_crt_act function, is passed the  key word line from a
// monster talk file, and parses the key word, as well as any monster
// monster action (i.e. cast a spell, attack, do a social command)
// The parsed information is then assigned to the fields of the
// monster talkstructure.

int talk_crt_act(char *str, ttag *tlk ) {

    int     index =0, num =0,i, n;
    char    *word[4];


    if(!str) {
        tlk->key = nullptr;
        tlk->action = nullptr;
        tlk->target = nullptr;
        tlk->type = 0;
        return(0);
    }


    for(i=0;i<4;i++)
        word[i] = nullptr;

    for(n=0;n<4;n++) {

        i=0;
        while(  isalpha(str[i +index]) ||
                isdigit(str[i +index]) ||
                str[i +index] == '-'
        )
            i++;
        // LEAK: Next line reported to be leaky: 4 count
        word[n] = new char[sizeof(char)*i + 1];
        if(!word[n])
            merror("talk_crt_act", FATAL);

        memcpy(word[n],&str[index],i);
        word[n][i] = 0;

        while(isspace(str[index +i]))
            i++;

        index += i;
        num++;
        if(str[index] == 0)
            break;

    }

    tlk->key = word[0];

    if(num < 2) {
        tlk->action = nullptr;
        tlk->target = nullptr;
        tlk->type = 0;
        return(0);
    }

    if(!strcmp(word[1],"ATTACK")) {
        tlk->type = 1;
        tlk->target = nullptr;
        tlk->action = nullptr;
    } else if(!strcmp(word[1],"ACTION") && num > 2) {
        tlk->type = 2;
        tlk->action = word[2];
        tlk->target = word[3];
    } else if(!strcmp(word[1],"CAST") && num > 2) {
        tlk->type = 3;
        tlk->action = word[2];
        tlk->target = word[3];
    } else if(!strcmp(word[1],"GIVE")) {
        tlk->type = 4;
        tlk->action = word[2];
        tlk->target = nullptr;
    } else {
        tlk->type = 0;
        tlk->action = nullptr;
        tlk->target = nullptr;
    }
    return(0);
}
