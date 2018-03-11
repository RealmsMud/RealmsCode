/*
 * logic.cpp
 *   Logic Creature Script added by Charles Marchant for 
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

#include "creatures.hpp"
#include "mud.hpp"


int loadCreature_actions( Creature* creature ) {
    ttag    *act,*a;
    char    crt_name[80], filename[256];
    char    cmdstr[80], responsestr[1024];
    int     i=0, count=0;
    char    *ptr;
    FILE    *fp;

    if(!creature->flagIsSet(M_LOGIC_MONSTER))
        return(0);
    if(creature->first_tlk)
        return(1);



    strcpy(crt_name,creature->getCName());
    for(i=0;crt_name[i];i++)
        if(crt_name[i] == ' ')
            crt_name[i] = '_';


    sprintf(filename,"%s/%s-%d-act.txt", Path::Talk, crt_name, creature->getLevel());

    fp = fopen(filename,"r");
    if(!fp)
        return(0);

    fgets(cmdstr,80,fp);
    while(!feof(fp)) {
        count++;
        act = new ttag;
        act->key = 0;
        act->target = 0;
        act->action = 0;
        act->response = 0;
        act->next_tag = 0;

        if(!act)
            merror("loadCreature_actions",FATAL);
        ptr = cmdstr;
        act->type = count;
        while(*ptr) {
            switch(*ptr) {
            case '?': // tester
                ptr++;
                act->test_for = *ptr++;
                if(*ptr == ':')
                    act->arg1 = atoi(++ptr);
                break;
            case '@': // unconditional goto cmd
                ptr++;
                act->goto_cmd = atoi(ptr);
                break;
            case '!': // if not last comand success goto cmd
                ptr++;
                act->if_cmd = atoi(ptr);
                for(;*ptr && *ptr != ':';ptr++)
                    ;
                ptr++;
                act->not_goto_cmd = atoi(ptr);
                break;
            case '=': // if last command succesful goto cmd
                ptr++;
                act->if_cmd = atoi(ptr);
                for(;*ptr && *ptr != ':';ptr++)
                    ;
                ptr++;
                act->if_goto_cmd = atoi(ptr);
                break;
            case '>':
                ptr++;
                act->do_act = *ptr++;
                if(*ptr == ':')
                    act->arg1 = atoi(++ptr);
                break;
            default:
                merror("ERROR - logic command", NONFATAL);
                logn("Errors", "UNKOWN LOGIC COMMAND [%s] in %s\n",cmdstr,filename);
                act->do_act = 0;
                act->goto_cmd = 1;
                for(;*ptr;ptr++)
                    ;
                break;
            }
            for(;*ptr && *ptr != ' ';ptr++)
                ;
            for(;*ptr && *ptr == ' ';ptr++)
                ;
        } // end of command string parsing
        
        fgets(responsestr,1024,fp);
        if(responsestr[0] != '*') {
            ptr = responsestr;
            while(*ptr)
                if(*ptr == '\n' || *ptr == '\r')
                    *ptr = 0;
                else
                    ptr++;

            act->response = new char[strlen(responsestr)+1];
            if(!act->response)
                merror("loadCreature_action",FATAL);
            strcpy(act->response,responsestr);
        } else
            act->response = 0;
        act->next_tag = 0;
        if(!creature->first_tlk) {
            creature->first_tlk = act;
            act->on_cmd = 1;
        } else {
            for(a = creature->first_tlk;a->next_tag;a = a->next_tag)
                ;
            a->next_tag = act;
        }
        fgets(cmdstr,80,fp);
        if(feof(fp))
            break;
    }
    fclose(fp);
    return(0);
}

