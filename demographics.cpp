/*
 * demographics.cpp
 *   Handles the generation of demographics.
 *   ____            _
 *  |  _ \ ___  __ _| |_ __ ___  ___
 *  | |_) / _ \/ _` | | '_ ` _ \/ __|
 *  |  _ <  __/ (_| | | | | | | \__ \
 *  |_| \_\___|\__,_|_|_| |_| |_|___/
 *
 * Permission to use, modify and distribute is granted via the
 *  GNU Affero General Public License v3 or later
 *
 *  Copyright (C) 2007-2012 Jason Mitchell, Randi Mitchell
 *     Contributions by Tim Callahan, Jonathan Hseu
 *  Based on Mordor (C) Brooke Paul, Brett J. Vickers, John P. Freeman
 *
 */
#include "mud.h"
#include "calendar.h"

#include <dirent.h>
#include <fcntl.h>

#define MINIMUM_LEVEL   3

/* The array gets broken into sections
 * [0] - class
 * [1] - multi
 * [2] - race
 * [3] - deity
 * [4] - misc titles
 */

#define CLASS_START     0
#define CLASS_END   (CLASS_START+static_cast<int>(STAFF)-1)

#define MULTI_START         CLASS_END
#define M_FM                MULTI_START
#define M_FT                (MULTI_START+1)
#define M_CLA               (MULTI_START+2)
#define M_MT                (MULTI_START+3)
#define M_TM                (MULTI_START+4)
#define M_MA                (MULTI_START+5)
#define MULTI_END           (M_MA+1)

#define RACE_START      MULTI_END
#define RACE_END        (RACE_START+MAX_PLAYABLE_RACE-1)

#define DEITY_START     RACE_END
#define DEITY_END       (DEITY_START+DEITY_COUNT-1)


#define HIGHEST_START   DEITY_END

#define HIGHEST_PLY     HIGHEST_START
// highnum will be empty because RICHEST_PLY uses a long
#define RICHEST_PLY     (HIGHEST_START + 1)
#define OLDEST_PLY      (HIGHEST_START + 2)
#define LONGEST_PLY     (HIGHEST_START + 3)
// highnum will be empty because BEST_PK uses a float
#define BEST_PK         (HIGHEST_START + 4)
#define MOST_SPELLS     (HIGHEST_START + 5)
#define MOST_ROOMS      (HIGHEST_START + 6)

#define HIGHEST_END     (MOST_ROOMS+1)


int get_multi_id(int cClass, int cClass2) {
    if(static_cast<CreatureClass>(cClass) ==CreatureClass::FIGHTER && static_cast<CreatureClass>(cClass2)==CreatureClass::MAGE)
        return(M_FM);
    else if(static_cast<CreatureClass>(cClass)==CreatureClass::FIGHTER && static_cast<CreatureClass>(cClass2)==CreatureClass::THIEF)
        return(M_FT);
    else if(static_cast<CreatureClass>(cClass)==CreatureClass::CLERIC && static_cast<CreatureClass>(cClass2)==CreatureClass::ASSASSIN)
        return(M_CLA);
    else if(static_cast<CreatureClass>(cClass)==CreatureClass::MAGE && static_cast<CreatureClass>(cClass2)==CreatureClass::THIEF)
        return(M_MT);
    else if(static_cast<CreatureClass>(cClass)==CreatureClass::THIEF && static_cast<CreatureClass>(cClass2)==CreatureClass::MAGE)
        return(M_TM);
    else if(static_cast<CreatureClass>(cClass)==CreatureClass::MAGE && static_cast<CreatureClass>(cClass2)==CreatureClass::ASSASSIN)
        return(M_MA);
    return(0);
}

const char *get_multi_string(int id) {
    switch(id) {
    case M_FM:
        return("Fighter/Mage");
    case M_FT:
        return("Fighter/Thief");
    case M_CLA:
        return("Cleric/Assassin");
    case M_MT:
        return("Mage/Thief");
    case M_TM:
        return("Thief/Mage");
    case M_MA:
        return("Mage/Assassin");
    default:
        return("");
    }
}

void printEnding(int ff) {
    char    outstr[120];
    sprintf(outstr, "    |                                      |\n");
    write(ff, outstr, strlen(outstr));
    sprintf(outstr, "    |    __________________________________|_\n");
    write(ff, outstr, strlen(outstr));
    sprintf(outstr, "    \\   /                                   /\n");
    write(ff, outstr, strlen(outstr));
    sprintf(outstr, "     \\_/___________________________________/\n\n");
    write(ff, outstr, strlen(outstr));
    close(ff);
}


void doDemographics() {

#ifndef NODEMOGRAPHICS

    xmlDocPtr   xmlDoc;
    xmlNodePtr  rootNode, curNode, childNode;
    struct      dirent *dirp=0;
    struct      lasttime lasttime[128];
    DIR         *dir=0;
    char        highest[HIGHEST_END][80], name[80], outstr[120], *str, spells[32];
    char        percent1[10], percent2[10];
    long        exp;
    int         highnum[HIGHEST_END], breakdown[DEITY_END], level=0, numRooms=0;
    int         cClass=0, cClass2=0, race=0, deity=0, total=0;
    int         ff=0, i=0, n=0, age=0;
    cDay        *birthday;
    long        money[5], cgold=0, t=0, richest=0;
    float       best_pk=0, percent=0;
    const Calendar* calendar = gConfig->getCalendar();
    Statistics statistics;

    t = time(0);
    printf("Begining demographics routine.\n");


    // load the directory all the players are stored in
    printf("Opening %s...", Path::Player);
    if((dir = opendir(Path::Player)) == nullptr) {
        printf("Directory could not be opened.\n");
        return;
    }
    printf("done.\n");

    zero(highest, sizeof(highest));
    zero(highnum, sizeof(highnum));
    zero(breakdown, sizeof(breakdown));
    /*
    for(i=CLASS_START; i<CLASS_END; i++) {
        strcpy(highest[i], "class");
        //write(ff, outstr, strlen(outstr));
    }
    for(i=MULTI_START; i<MULTI_END; i++) {
        strcpy(highest[i], "multi");
        //write(ff, outstr, strlen(outstr));
    }
    for(i=RACE_START; i<RACE_END; i++) {
        strcpy(highest[i], "race");
        //write(ff, outstr, strlen(outstr));
    }
    for(i=DEITY_START; i<DEITY_END; i++) {
        strcpy(highest[i], "deity");
        //write(ff, outstr, strlen(outstr));
    }
    for(i=0; i<HIGHEST_END; i++) {
        sprintf(outstr, "%d: %s:%d\n", i, highest[i], highnum[i]);
        //write(ff, outstr, strlen(outstr));
    }
    */


    printf("Reading player directory...");
    while((dirp = readdir(dir)) != nullptr) {
        // is this a player file?
        if(dirp->d_name[0] == '.')
            continue;
        if(!isupper(dirp->d_name[0]))
            continue;

        // is this a readable player file?
        sprintf(name, "%s/%s", Path::Player, dirp->d_name);
        if((xmlDoc = xml::loadFile(name, "Player")) == nullptr)
            continue;

        // get the information we need
        rootNode = xmlDocGetRootElement(xmlDoc);
        curNode = rootNode->children;


        // reset some variables
        zero(spells, sizeof(spells));
        exp = level = cClass = cClass2 = race = deity = age = cgold = numRooms = 0;
        birthday = new cDay;
        statistics.reset();


        xml::copyPropToCString(name, rootNode, "Name");

        // not for these characters
        if(!strcmp(name, "Johny") || !strcmp(name, "Min") || !strcmp(name, "Bob"))
            continue;

        // load the information we need
        while(curNode) {
            if(NODE_NAME(curNode, "Experience")) xml::copyToNum(exp, curNode);
            else if(NODE_NAME(curNode, "Level")) {
                xml::copyToNum(level, curNode);
                if(level < MINIMUM_LEVEL)
                    break;
            }
            else if(NODE_NAME(curNode, "Class")) xml::copyToNum(cClass, curNode);
            else if(NODE_NAME(curNode, "Class2")) xml::copyToNum(cClass2, curNode);
            else if(NODE_NAME(curNode, "Race")) xml::copyToNum(race, curNode);

            // ok, deity is spelled wrong in the xml files, so try both
            else if(!deity && NODE_NAME(curNode, "Deity")) xml::copyToNum(deity, curNode);

            else if(NODE_NAME(curNode, "LastTimes")) {
                loadLastTimes(curNode, lasttime);
            } else if(NODE_NAME(curNode, "Birthday")) {
                birthday->load(curNode);
            } else if(NODE_NAME(curNode, "Bank")) {
                xml::loadNumArray<long>(curNode, money, "Coin", 5);
                cgold += money[2];
                for(i=0; i<5; i++)
                    money[i] = 0;
            } else if(NODE_NAME(curNode, "Coins")) {
                xml::loadNumArray<long>(curNode, money, "Coin", 5);
                cgold += money[2];
                for(i=0; i<5; i++)
                    money[i] = 0;
            }
            else if(NODE_NAME(curNode, "Statistics")) statistics.load(curNode);
            else if(NODE_NAME(curNode, "Spells")) loadBits(curNode, spells);
            else if(NODE_NAME(curNode, "RoomExp")) {
                childNode = curNode->children;
                while(childNode) {
                    if(NODE_NAME(childNode, "Room")) numRooms++;
                    childNode = childNode->next;
                }
            }

            curNode = curNode->next;
        }

        // don't do staff!
        if(cClass >= static_cast<int>(STAFF))
            continue;
        if(level < MINIMUM_LEVEL)
            continue;

        // highest player, or highest in their class?
        if(cClass && cClass2)
            cClass = get_multi_id(cClass, cClass2);
        else
            cClass = cClass - 1 + CLASS_START;

        if(exp > highnum[cClass]) {
            highnum[cClass] = exp;
            //sprintf(highest[cClass], "%s, %d", name, level);
            sprintf(highest[cClass], "%s", name);
            if(exp > highnum[HIGHEST_PLY]) {
                highnum[HIGHEST_PLY] = exp;
                //sprintf(highest[HIGHEST_PLY], "%s, %d", name, level);
                sprintf(highest[HIGHEST_PLY], "%s", name);
            }
        }
        // highest in their race?
        race = race - 1 + RACE_START;
        if(exp > highnum[race]) {
            highnum[race] = exp;
            //sprintf(highest[race], "%s, %d", name, level);
            sprintf(highest[race], "%s", name);
        }
        // highest in their deity?
        if(deity) {
            deity = deity - 1 + DEITY_START;
            if(exp > highnum[deity]) {
                highnum[deity] = exp;
                //sprintf(highest[deity], "%s, %d", name, level);
                sprintf(highest[deity], "%s", name);
            }
        }

        n=0;
        for(i=0; i < get_spell_list_size() ; i++)
            if(spells[i/8] & 1<<(i%8))
                n++;
        if(n > highnum[MOST_SPELLS]) {
            highnum[MOST_SPELLS] = n;
            //sprintf(highest[MOST_SPELLS], "%s, %d", name, level);
            sprintf(highest[MOST_SPELLS], "%s", name);
        }


        if(numRooms > highnum[MOST_ROOMS]) {
            highnum[MOST_ROOMS] = numRooms;
            //sprintf(highest[MOST_ROOMS], "%s, %d", name, level);
            sprintf(highest[MOST_ROOMS], "%s", name);
        }


        breakdown[cClass]++;
        breakdown[race]++;
        total++;
        if(deity)
            breakdown[deity]++;

        // longest played?
        if(lasttime[LT_AGE].interval > highnum[LONGEST_PLY]) {
            highnum[LONGEST_PLY] = lasttime[LT_AGE].interval;
            //sprintf(highest[LONGEST_PLY], "%s, %d", name, level);
            sprintf(highest[LONGEST_PLY], "%s", name);
        }

        // character age?
        if(birthday) {
            age = calendar->getCurYear() - birthday->getYear();
            if(calendar->getCurMonth() < birthday->getMonth())
                age--;
            if(calendar->getCurMonth() == birthday->getMonth() && calendar->getCurDay() < birthday->getDay())
                age--;
            if(age > highnum[OLDEST_PLY]) {
                highnum[OLDEST_PLY] = age;
                //sprintf(highest[OLDEST_PLY], "%s, %d", name, level);
                sprintf(highest[OLDEST_PLY], "%s", name);
            }
        }

        // richest character?
        if(cgold > richest) {
            richest = cgold;
            //sprintf(highest[RICHEST_PLY], "%s, %d", name, level);
            sprintf(highest[RICHEST_PLY], "%s", name);
        }

        // best non-100% pk
        if(statistics.pkDemographics() > best_pk) {
            best_pk = statistics.pkDemographics();
            //sprintf(highest[BEST_PK], "%s, %d", name, level);
            sprintf(highest[BEST_PK], "%s", name);
        }
        delete birthday;
        birthday=0;
    }
    printf("done.\n");
    closedir(dir);
    xmlFreeDoc(xmlDoc);
    xmlCleanupParser();

    printf("Formatting stone scrolls...");

    // load the file we will be printing to
    sprintf(name, "%s/stone_scroll.txt", Path::Sign);
    unlink(name);
    ff = open(name, O_CREAT | O_RDWR, ACC);
    if(ff < 0) {
        printf("Couldn't open stone scroll: %s!\n", name);
        close(ff);
        return;
    }

    sprintf(outstr, "   _______________________________________\n");
    write(ff, outstr, strlen(outstr));
    sprintf(outstr, "  /\\                                      \\\n");
    write(ff, outstr, strlen(outstr));
    sprintf(outstr, " /  | Only characters who have reached     |\n");
    write(ff, outstr, strlen(outstr));
    sprintf(outstr, " \\  | atleast level 3 are counted for the  |\n");
    write(ff, outstr, strlen(outstr));
    sprintf(outstr, "  \\_| stone scroll.                        |\n");
    write(ff, outstr, strlen(outstr));
    sprintf(outstr, "    |                                      |\n");
    write(ff, outstr, strlen(outstr));
    sprintf(outstr, "    |   Highest Player : %-17s |\n", highest[HIGHEST_PLY]);
    write(ff, outstr, strlen(outstr));
    sprintf(outstr, "    |   Richest Player : %-17s |\n", highest[RICHEST_PLY]);
    //write(ff, outstr, strlen(outstr));
    //sprintf(outstr, "    |   Oldest Player  : %-17s |\n", highest[OLDEST_PLY]);
    write(ff, outstr, strlen(outstr));
    sprintf(outstr, "    |   Longest Played : %-17s |\n", highest[LONGEST_PLY]);
    write(ff, outstr, strlen(outstr));
    sprintf(outstr, "    |   Best Pkiller   : %-17s |\n", highest[BEST_PK]);
    write(ff, outstr, strlen(outstr));
    sprintf(outstr, "    |   Most Spells    : %-17s |\n", highest[MOST_SPELLS]);
    write(ff, outstr, strlen(outstr));
    sprintf(outstr, "    |   Explorer       : %-17s |\n", highest[MOST_ROOMS]);
    write(ff, outstr, strlen(outstr));
    sprintf(outstr, "    |                                      |\n");
    write(ff, outstr, strlen(outstr));
    sprintf(outstr, "    | To view a specific category, type    |\n");
    write(ff, outstr, strlen(outstr));
    sprintf(outstr, "    | \"look stone <category>\". The         |\n");
    write(ff, outstr, strlen(outstr));
    sprintf(outstr, "    | categories are:                      |\n");
    write(ff, outstr, strlen(outstr));
    sprintf(outstr, "    |         class                        |\n");
    write(ff, outstr, strlen(outstr));
    sprintf(outstr, "    |         race                         |\n");
    write(ff, outstr, strlen(outstr));
    sprintf(outstr, "    |         deity                        |\n");
    write(ff, outstr, strlen(outstr));
    sprintf(outstr, "    |         breakdown                    |\n");
    write(ff, outstr, strlen(outstr));
    sprintf(outstr, "    |                                      |\n");
    write(ff, outstr, strlen(outstr));
    sprintf(outstr, "    | Last Updated:                        |\n");
    write(ff, outstr, strlen(outstr));

    // let them know when we ran this program
    str = ctime(&t);
    str[strlen(str) - 1] = 0;
    strcat(str, " (");
    strcat(str, gServer->getTimeZone().c_str());
    strcat(str, ").");

    sprintf(outstr, "    | %36s |\n", str);
    write(ff, outstr, strlen(outstr));
    /*
    for(i=0; i<HIGHEST_END; i++) {
        sprintf(outstr, "%d: %s\n", i, highest[i]);
        write(ff, outstr, strlen(outstr));
    }
    */
    printEnding(ff);



    // load the file we will be printing to
    sprintf(name, "%s/stone_scroll_class.txt", Path::Sign);
    unlink(name);
    ff = open(name, O_CREAT | O_RDWR, ACC);
    if(ff < 0) {
        printf("Couldn't open stone scroll: %s!\n", name);
        close(ff);
        return;
    }

    sprintf(outstr, "   _______________________________________\n");
    write(ff, outstr, strlen(outstr));
    sprintf(outstr, "  /\\                                      \\\n");
    write(ff, outstr, strlen(outstr));
    sprintf(outstr, " /  | Highest Player Based on Class        |\n");
    write(ff, outstr, strlen(outstr));
    sprintf(outstr, " \\  |                                      |\n");
    write(ff, outstr, strlen(outstr));
    for(i=CLASS_START; i<CLASS_END; i++) {
        sprintf(outstr, "%s %15s : %-18s |\n", i==CLASS_START ? "  \\_|" : "    |", get_class_string(i-CLASS_START+1), highest[i]);
        write(ff, outstr, strlen(outstr));
    }
    sprintf(outstr, "    |                                      |\n");
    write(ff, outstr, strlen(outstr));
    for(i=MULTI_START; i<MULTI_END; i++) {
        sprintf(outstr, "    | %15s : %-18s |\n", get_multi_string(i), highest[i]);
        write(ff, outstr, strlen(outstr));
    }

    printEnding(ff);


    // load the file we will be printing to
    sprintf(name, "%s/stone_scroll_race.txt", Path::Sign);
    unlink(name);
    ff = open(name, O_CREAT | O_RDWR, ACC);
    if(ff < 0) {
        printf("Couldn't open stone scroll: %s!\n", name);
        close(ff);
        return;
    }

    sprintf(outstr, "   _______________________________________\n");
    write(ff, outstr, strlen(outstr));
    sprintf(outstr, "  /\\                                      \\\n");
    write(ff, outstr, strlen(outstr));
    sprintf(outstr, " /  | Highest Player Based on Race         |\n");
    write(ff, outstr, strlen(outstr));
    sprintf(outstr, " \\  |                                      |\n");
    write(ff, outstr, strlen(outstr));
    for(i=RACE_START; i<RACE_END; i++) {
        sprintf(outstr, "%s %13s : %-20s |\n", i==RACE_START ? "  \\_|" : "    |",
            gConfig->getRace(i-RACE_START+1)->getName().c_str(), highest[i]);
        write(ff, outstr, strlen(outstr));
    }

    printEnding(ff);

    // load the file we will be printing to
    sprintf(name, "%s/stone_scroll_deity.txt", Path::Sign);
    unlink(name);
    ff = open(name, O_CREAT | O_RDWR, ACC);
    if(ff < 0) {
        printf("Couldn't open stone scroll: %s!\n", name);
        close(ff);
        return;
    }

    sprintf(outstr, "   _______________________________________\n");
    write(ff, outstr, strlen(outstr));
    sprintf(outstr, "  /\\                                      \\\n");
    write(ff, outstr, strlen(outstr));
    sprintf(outstr, " /  | Highest Player Based on Deity        |\n");
    write(ff, outstr, strlen(outstr));
    sprintf(outstr, " \\  |                                      |\n");
    write(ff, outstr, strlen(outstr));
    for(i=DEITY_START; i<DEITY_END; i++) {
        sprintf(outstr, "%s %13s : %-20s |\n", i==DEITY_START ? "  \\_|" : "    |", gConfig->getDeity(i-DEITY_START+1)->getName().c_str(), highest[i]);
        write(ff, outstr, strlen(outstr));
    }

    printEnding(ff);

    // load the file we will be printing to
    sprintf(name, "%s/stone_scroll_breakdown.txt", Path::Sign);
    unlink(name);
    ff = open(name, O_CREAT | O_RDWR, ACC);
    if(ff < 0) {
        printf("Couldn't open stone scroll: %s!\n", name);
        close(ff);
        return;
    }

    sprintf(outstr, "   ________________________________________________\n");
    write(ff, outstr, strlen(outstr));
    sprintf(outstr, "  /\\                                               \\\n");
    write(ff, outstr, strlen(outstr));
    sprintf(outstr, " /  | Player Breakdown                              |\n");
    write(ff, outstr, strlen(outstr));
    sprintf(outstr, " \\  |                                               |\n");
    write(ff, outstr, strlen(outstr));

    // go through all classes
    for(i=CLASS_START; i<CLASS_END; i++) {
        // this will get us to races
        n = i-CLASS_START+RACE_START;

        percent = (float) breakdown[i] / (float) total * 100;
        sprintf(percent1, "%s%.1f%%", percent >= 10 ? "" : " ", percent);
        percent = (float) breakdown[n] / (float) total * 100;
        sprintf(percent2, "%s%.1f%%", percent >= 10 ? "" : " ", percent);

        sprintf(outstr, "%s %15s : %-5s %12s : %-5s  |\n", i==CLASS_START ? "  \\_|" : "    |",
            get_class_string(i-CLASS_START+1), percent1,
            gConfig->getRace(n-RACE_START+1)->getName().c_str(), percent2);
        write(ff, outstr, strlen(outstr));
    }

    // make a space for multiclass to come
    i = RACE_START + CLASS_END;
    percent = (float) breakdown[i] / (float) total * 100;
    sprintf(percent1, "%s%.1f%%", percent >= 10 ? "" : " ", percent);
    sprintf(outstr, "    | %36s : %-5s  |\n", gConfig->getRace(i-RACE_START+1)->getName().c_str(), percent1);
    write(ff, outstr, strlen(outstr));

    // finish off races
    for(i=MULTI_START; i<MULTI_END-(RACE_END-RACE_START-CLASS_END-1); i++) {
        // this will get us to where we left off
        n = RACE_START + CLASS_END + i - MULTI_START + 1;

        percent = (float) breakdown[i] / (float) total * 100;
        sprintf(percent1, "%s%.1f%%", percent >= 10 ? "" : " ", percent);
        percent = (float) breakdown[n] / (float) total * 100;
        sprintf(percent2, "%s%.1f%%", percent >= 10 ? "" : " ", percent);

        sprintf(outstr, "    | %15s : %-5s %12s : %-5s  |\n",
            get_multi_string(i), percent1,
            gConfig->getRace(n-RACE_START+1)->getName().c_str(), percent2);
        write(ff, outstr, strlen(outstr));
    }

    // finish off multiclass
    for(i=MULTI_END-(RACE_END-RACE_START-CLASS_END-1); i<MULTI_END; i++) {
        percent = (float) breakdown[i] / (float) total * 100;
        sprintf(percent1, "%s%.1f%%", percent >= 10 ? "" : " ", percent);

        sprintf(outstr, "    | %15s : %-5s%21s  |\n",
            get_multi_string(i), percent1, " ");
        write(ff, outstr, strlen(outstr));
    }

    sprintf(outstr, "    |                                               |\n");
    write(ff, outstr, strlen(outstr));

    // deity isnt used, so make this the total number of deity chars
    deity = 0;
    for(i=DEITY_START; i<DEITY_END; i++)
        deity += breakdown[i];

    // finish off dieties
    for(i=DEITY_START; i<DEITY_END; i++) {
        percent = (float) breakdown[i] / (float) deity * 100;
        sprintf(percent1, "%s%.1f%%", percent >= 10 ? "" : " ", percent);

        sprintf(outstr, "    | %15s : %-5s%21s  |\n",
                gConfig->getDeity(i-DEITY_START+1)->getName().c_str(), percent1, " ");
        write(ff, outstr, strlen(outstr));
    }

    sprintf(outstr, "    |                                               |\n");
    write(ff, outstr, strlen(outstr));
    sprintf(outstr, "    | deity breakdown is limited to classes         |\n");
    write(ff, outstr, strlen(outstr));
    sprintf(outstr, "    | pledged to a god.                             |\n");
    write(ff, outstr, strlen(outstr));
    sprintf(outstr, "    |                                               |\n");
    write(ff, outstr, strlen(outstr));
    sprintf(outstr, "    |    ___________________________________________|_\n");
    write(ff, outstr, strlen(outstr));
    sprintf(outstr, "    \\   /                                            /\n");
    write(ff, outstr, strlen(outstr));
    sprintf(outstr, "     \\_/____________________________________________/\n\n");
    write(ff, outstr, strlen(outstr));
    close(ff);

    printf("done.\nDone.\n");
#endif
}

void runDemographics() {
#ifndef NODEMOGRAPHICS
    if(!fork()) {
        doDemographics();
        exit(0);
    }
#endif
}

int cmdDemographics(Player* player, cmd* cmnd) {
    player->print("Beginning demographics routine.\nCheck for results in a few seconds.\n");
    runDemographics();
    return(0);
}
