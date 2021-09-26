/*
 * misc.cpp
 *   Miscellaneous string, file and data structure manipulation routines
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

#include <bits/types/struct_tm.h>  // for tm
#include <cctype>                  // for isdigit, tolower, toupper
#include <fcntl.h>                 // for open, O_RDONLY, SEEK_SET, O_RDWR
#include <cmath>                   // for pow
#include <cstdarg>                 // for va_end, va_list, va_start
#include <cstdio>                  // for sprintf, fclose, fseek, fopen, ftell
#include <cstdlib>                 // for atol, free, ldiv_t, abort, labs, ldiv
#include <cstring>                 // for strlen, strstr, strcpy, strcmp
#include <unistd.h>                // for close, read, lseek, off_t, write
#include <ctime>                   // for time, localtime, ctime
#include <ostream>                 // for operator<<, basic_ostream::operator<<
#include <string>                  // for char_traits, operator==, basic_string

#include "bstring.hpp"             // for bstring
#include "catRef.hpp"              // for CatRef
#include "config.hpp"              // for Config, gConfig
#include "container.hpp"           // for MonsterSet, PlayerSet, Container
#include "creatures.hpp"           // for Creature, Player, Monster
#include "exits.hpp"               // for Exit
#include "flags.hpp"               // for P_READING_FILE, P_BUGGED
#include "global.hpp"              // for MAXALVL, FATAL, FIND_EXIT, FIND_MO...
#include "group.hpp"               // for CreatureList, Group
#include "money.hpp"               // for GOLD, Money
#include "mud.hpp"                 // for dmname, LT_KICK, LT_PLAYER_STUNNED
#include "objects.hpp"             // for Object
#include "os.hpp"                  // for merror
#include "paths.hpp"               // for BugLog, Config
#include "proto.hpp"               // for keyTxtEqual, broadcast, findExit
#include "random.hpp"              // for Random
#include "rooms.hpp"               // for BaseRoom
#include "server.hpp"              // for Server, gServer, PlayerMap
#include "socket.hpp"              // for Socket
#include "structs.hpp"             // for daily
#include "utils.hpp"               // for MAX, MIN

class MudObject;

//*********************************************************************
//                      validId functions
//*********************************************************************

bool validMobId(const CatRef& cr) {
    return(!cr.isArea("") && cr.id > 0 && cr.id < MMAX);
}
bool validObjId(const CatRef& cr) {
    // 0 = coins
    return(!cr.isArea("") && cr.id >= 0 && cr.id < OMAX);
}
bool validRoomId(const CatRef& cr) {
    // 0 = void
    return(!cr.isArea("") && cr.id >= 0 && cr.id < RMAX);
}



//*********************************************************************
//                      removeColor
//*********************************************************************
// we need to remove colors that might be in the name!

bstring removeColor(bstring obj) {
    int i=0, len=0;
    bstring name = "";

    for(len = obj.length(); i<len; i++) {
        while(i<len && obj.at(i) == '^')
            i += 2;
        if(i<len)
            name += obj.at(i);
    }

    return(name);
}



//*********************************************************************
//                      lowercize
//*********************************************************************
// This function takes the string passed in as the first parameter and
// converts it to lowercase. If the flag in the second parameter has
// its first bit set, then the first letter is capitalized.

void lowercize(bstring& str, int flag ) {
    int     i, n;

    n = str.length();
    for(i=0; i<n; i++)
        str[i] = (str[i] >= 'A' && str[i] <= 'Z') ? str[i]+32:str[i];
    if(flag & 1)
        str[0] = (str[0] >= 'a' && str[0] <= 'z') ? str[0]-32:str[0];
}

void lowercize(char *str, int flag ) {
    int     i, n;

    n = str ? strlen(str) : 0 ;
    for(i=0; i<n; i++)
        str[i] = (str[i] >= 'A' && str[i] <= 'Z') ? str[i]+32:str[i];
    if(flag & 1)
        str[0] = (str[0] >= 'a' && str[0] <= 'z') ? str[0]-32:str[0];
}

//*********************************************************************
//                      low
//*********************************************************************
// If the character passed in as the first parameter is an uppercase
// alphabetic character, then it is converted to lowercase and returned
// Otherwise, it is unchanged.
char low(char ch) {
    if(ch >= 'A' && ch <= 'Z')
        return(ch+32);
    else
        return(ch);
}

char up(char ch) {
    if(ch >= 'a' && ch <= 'z')
        return(ch-32);
    else
        return(ch);
}


int statBonus[MAXALVL] = {
    -4, -4, -4,         // 0 - 2
    -3, -3,             // 3 - 4
    -2, -2,             // 5 - 6
    -1,                 // 7
    0, 0, 0, 0, 0, 0,   // 8 - 13
    1, 1, 1,            // 14 - 16
    2, 2, 2, 2,         // 17 - 20
    3, 3, 3, 3,         // 21 - 24
    4, 4, 4,            // 25 - 27
    5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5 };   // 28+

int bonus(unsigned int num) {
    return( statBonus[MIN<int>(num/10, MAXALVL - 1)] );
}

int crtWisdom(Creature* creature) {
    return(bonus(creature->intelligence.getCur()) + bonus(creature->piety.getCur()))/2;
}

int crtAwareness(Creature* creature) {
    return(bonus(creature->intelligence.getCur()) + bonus(creature->dexterity.getCur()))/2;
}
//*********************************************************************
//                      zero
//*********************************************************************
// This function zeroes a block of bytes at the given pointer and the
// given length.

void zero( void *ptr, int size ) {
    char    *chptr;
    int i;
    chptr = (char *)ptr;
    for(i=0; i<size; i++)
        chptr[i] = 0;
}


//*********************************************************************
//                      delimit
//*********************************************************************
// This function takes a given string, and if it is greater than a given
// number of characters, then it is split up into several lines. This
// is done by replacing spaces with carriage returns before the end of/
// the line.

bstring delimit(const char *str, int wrap) {
    int     i=0, j=0, x=0, l=0, len=0, lastspace=0;
    char* work = strdup(str);
    std::ostringstream outStr;

    if(wrap <= 10)
        wrap = 78;

    j = (str) ? strlen(str) : 0;
    if(j < wrap) {
        free(work);
        return str;
    }

    len = 0;
    lastspace = -1;
    l = 0;
    for(i=0; i<j; i++) {
        if(work[i] == ' ')
            lastspace = i;
        if(work[i] == '\n') {
            len = 0;
            lastspace = -1;
        }
        if(work[i] == '^')
            x+=2;
        if(work[i] == '\033')
            x+=7;

        len++;
        if(len > (wrap + x) && lastspace > -1) {
            work[lastspace] = 0;
            outStr << &work[l] << "\n  ";
            l = lastspace + 1;
            len = i - lastspace + 3;
            lastspace = -1;
            x=0;
        }
    }
    outStr << &work[l];
    free(work);
    return(outStr.str());
}

//*********************************************************************
//                      dice
//*********************************************************************
// This function rolls n s-sided dice and adds p to the total.

int dice(int n, int s, int p) {
    int i;
    if(n==0 || s== 0)
        return(p);

    for(i=0; i<n; i++)
        p += Random::get(1,s);

    return(p);
}

//*********************************************************************
//                      exp_to_lev
//*********************************************************************
// This function takes a given amount of experience as its first
// argument returns the level that the experience reflects.

int exp_to_lev(unsigned long exp) {
    int level = 1;

    while(exp >= Config::expNeeded(level) && level < MAXALVL)
        level++;
    if(level == MAXALVL) {
        level = exp/(Config::expNeeded(MAXALVL));
        level++;
        level= MAX(MAXALVL, level);
    }

    return(MAX(1,level));
}

//*********************************************************************
//                      dec_daily
//*********************************************************************
// This function is called whenever a daily-use item or operation is
// used or performed. If the number of daily uses are used, up then
// a 0 is returned. Otherwise, the number of uses is decremented and
// a 1 is returned.

int dec_daily(struct daily *dly_ptr) {
    long        t;
    struct tm   *tm, time1{}, time2{};

    t = time(nullptr);
    tm = localtime(&t);
    time1 = *tm;
    tm = localtime(&dly_ptr->ltime);
    time2 = *tm;

    if(time1.tm_yday != time2.tm_yday) {
        dly_ptr->cur = dly_ptr->max;
        dly_ptr->ltime = t;
    }

    if(dly_ptr->cur == 0)
        return(0);

    dly_ptr->cur--;

    return(1);
}

//*********************************************************************
//                      update_daily
//*********************************************************************

int update_daily(struct daily *dly_ptr) {
    long        t = time(nullptr);
    struct tm   *tm, time1{}, time2{};

    tm = localtime(&t);
    time1 = *tm;
    tm = localtime(&dly_ptr->ltime);
    time2 = *tm;

    if(time1.tm_yday != time2.tm_yday) {
        dly_ptr->cur = dly_ptr->max;
        dly_ptr->ltime = t;
    }

    return(0);
}


//*********************************************************************
//                      file_exists
//*********************************************************************
// This function returns 1 if the filename specified by the first
// parameter exists, 0 if it doesn't.

bool file_exists(char *filename) {
    int ff=0;
    ff = open(filename, O_RDONLY);
    if(ff > -1) {
        close(ff);
        return(true);
    }
    return(false);
}

/*====================================================================*/
// checks if the given str contains all digits
bool is_num(char *str ) {
    int len, i;
    len = strlen(str);
    for(i=0;i < len; i++)
        if(!isdigit(str[i]))
            return(false);
    return(true);
}

//*********************************************************************
//                      isdm
//*********************************************************************
// returns 1 if the given player name is a dm

bool isdm(const bstring& name) {
    char **s = dmname;
    while(*s) {
        if(name == *s)
            return(true);
        s++;
    }
    return(false);
}
//*********************************************************************
//                      smashInvis
//*********************************************************************
int Creature::smashInvis() {
    if(isPlayer()) {
        unhide();
        removeEffect("invisibility");
        unmist();
    }
    return(0);
}

//*********************************************************************
//                      parse_name
//*********************************************************************
// Determine if a given name is acceptable

bool parse_name(const bstring& name) {
    FILE    *fp=nullptr;
    int     i = name.length() - 1;
    char    str[80], path[80], forbid[20];
    strcpy(str, name.c_str());

    if(isTitle(str) || isClass(str))
        return(false);
    if(gConfig->racetoNum(str) >= 0)
        return(false);
    if(gConfig->deitytoNum(str) >= 0)
        return(false);


    // don't allow names with all the same char
    str[0] = tolower(str[0]);
    for(; i>0; i--)
        if(str[i] != str[0])
            break;
    if(!i)
        return(false);
    str[0] = toupper(str[0]);


    // check the DM names
    i=0;
    while(dmname[i]) {
        // don't forbid names directly equal to DM
        if(strcmp(dmname[i], str) != 0) {
            if(!strncmp(dmname[i], str, strlen(str)))
                return(false);
            if(!strncmp(str, dmname[i], strlen(dmname[i])))
                return(false);
        }
        i++;
    }


    sprintf(path, "%s/forbidden_name.txt", Path::Config);
    fp = fopen(path, "r");
    if(!fp)
        merror("ERROR - forbidden name.txt", NONFATAL);
    else {
        while(!feof(fp)) {
            fscanf(fp, "%s", forbid);
            if(!strcmp(forbid, str)) {
                fclose(fp);
                return(false);
            }
        }
        fclose(fp);
    }


    lowercize(str, 0);
    if(strstr(str, "fuck"))
        return(false);
    if(strstr(str, "shit"))
        return(false);
    if(strstr(str, "suck"))
        return(false);
    if(strstr(str, "gay"))
        return(false);
    if(strstr(str, "isen"))
        return(false);
    if(strstr(str, "cock"))
        return(false);
    if(strstr(str, "realm"))
        return(false);
    if(strstr(str, "piss"))
        return(false);
    if(strstr(str, "dick"))
        return(false);
    if(strstr(str, "pussy"))
        return(false);
    if(strstr(str, "dollar"))
        return(false);
    if(strstr(str, "cunt"))
        return(false);

    return(true);
}

//*********************************************************************
//                      dmIson
//*********************************************************************

int dmIson() {
    Player* player=nullptr;
    int     idle=0;
    long    t = time(nullptr);

    for( const auto& p : gServer->players) {
        player = p.second;

        if(!player->isConnected())
            continue;
        idle = t - player->getSock()->ltime;
        if(player->isDm() && idle < 600)
            return(1);
    }

    return(0);
}

//*********************************************************************
//                      bug
//*********************************************************************

void Player::bug(const char *fmt, ...) const {
    char    file[80];
    char    str[2048];
    int     fd;
    long    t = time(nullptr);
    va_list ap;

    if(!flagIsSet(P_BUGGED))
        return;

    va_start(ap, fmt);

    sprintf(file, "%s/%s.txt", Path::BugLog, getCName());
    fd = open(file, O_RDWR | O_APPEND, 0);
    if(fd < 0) {
        fd = open(file, O_RDWR | O_CREAT, ACC);
        if(fd < 0)
            return;
    }
    lseek(fd, 0L, 2);

    // prevent string overruns with vsn
    strcpy(str, ctime(&t));
    str[24] = ':';
    str[25] = ' ';
    vsnprintf(str + 26, 2000, fmt, ap);
    va_end(ap);

    write(fd, str, strlen(str));

    close(fd);
}

//*********************************************************************
//                      autosplit
//*********************************************************************

int Player::autosplit(long amount) {
    int     remain=0, split=0, party=0;

    if(isStaff())
        return(0);
    if(!ableToDoCommand())
        return(0);

    if(amount <= 5)
        return(0);

    Group* group = getGroup();
    if(!group)
        return(0);

    for(Creature* crt : group->members) {
        if(crt->isPlayer() && !crt->isStaff() && crt->inSameRoom(this))
            party++;
    }
    // If group is 1, return with no split.
    if(party < 2)
        return(0);
    // If less gold then people, there is no split.
    if(amount < party)
        return(0);

    remain = amount % party;    // Find remaining odd coins.
    split = ((amount - remain) / party);  // Determine split minus the remaining odd coins.

    for(Creature* crt : group->members) {
        if(crt->isPlayer() && !crt->isStaff() && crt->inSameRoom(this)) {
            if(crt == this) {
                crt->print("You received %d gold as your split.\n", split+remain);
                crt->coins.add(split+remain, GOLD);
            } else {
                crt->print("You received %d gold as your split from %N.\n", split, this);
                crt->coins.add(split, GOLD);
                crt->print("You now have %d gold coins.\n", crt->coins[GOLD]);
            }
        }

    }
    return(1);
}

//*********************************************************************
//                      strPrefix
//*********************************************************************
// Determines if needle is a prefix to haystack
// Returns 1 if it is, 0 otherwise

int strPrefix(const char *haystack, const char *needle) {
    if(!haystack)
        return(0);
    if(!needle)
        return(0);

    for(; *haystack && *needle; haystack++, needle++)
        if(*haystack != *needle)
            return(0);

    return(1);
}

//*********************************************************************
//                      strSuffix
//*********************************************************************
// Determines if needle is a suffix of haystack
// Returns 1 if it is, 0 otherwise

int strSuffix(const char *haystack, const char *needle) {
    int hayLen = strlen(haystack);
    int needleLen = strlen(needle);

    if(hayLen >= needleLen && !strcmp(needle, haystack + hayLen - needleLen))
        return(1);

    return(0);
}

//*********************************************************************
//                      pkillPercent
//*********************************************************************

int pkillPercent(int pkillsWon, int pkillsIn) {
    double percent=0;
    double pkWon = pkillsWon * 1.0, pkIn = pkillsIn * 1.0;

    if(pkillsIn == pkillsWon) {
        percent = 100;
    } else {
        //  if(pkillsWon == 0)
        //      pkillsWon = 1;
        if(pkillsIn == 0)
            pkIn = 1.0;

        if(pkillsWon > pkillsIn)
            pkWon = pkIn;

        percent = (pkWon/pkIn) * 100.0;
    }
    return((int)percent);
}

//*********************************************************************
//                      stun
//*********************************************************************
// stops a creature or player from attacking, casting a spell, or reading
// a scroll for <delay> seconds.  Used by stun (befuddle), circle

void Creature::stun(int delay) {
    if(!delay)
        return;
    updateAttackTimer(true, (delay+1)*10);
    lasttime[LT_KICK].ltime = time(nullptr);
    lasttime[LT_SPELL].ltime = time(nullptr);
    lasttime[LT_READ_SCROLL].ltime = time(nullptr);
    lasttime[LT_KICK].interval = delay+1;
    lasttime[LT_SPELL].interval = delay+1;
    lasttime[LT_READ_SCROLL].interval = delay+1;
    if(isPlayer()) {
        lasttime[LT_PLAYER_STUNNED].ltime = time(nullptr);
        lasttime[LT_PLAYER_STUNNED].interval = delay+1;
        setFlag(P_STUNNED);
    }
}

//*********************************************************************
//                      numEnemyMonInRoom
//*********************************************************************

int numEnemyMonInRoom(Creature* player) {
    int     count=0;
    for(Monster* mons : player->getRoomParent()->monsters) {
        if(mons->getAsMonster()->isEnemy(player))
            count++;
    }

    return(count);
}

//*********************************************************************
//                      stripLineFeeds
//*********************************************************************

char *stripLineFeeds(char *str) {
    int n=0, i=0;
    char *name=nullptr;

    name = str;
    n = strlen(name);

    for(i = n; i > 0; i--) {
        if(name[i] == '\n') {
            name[i] = ' ';
            break;
        }
    }

    return(name);
}

//*********************************************************************
//                      stripBadChars
//*********************************************************************

void stripBadChars(char *str) {
    int n=0, i=0;

    n = (str) ? strlen(str) : 0;

    for(i = 0; i < n; i++) {
        if(str[i] == '/') {
            str[i] = ' ';
        }
    }
}

void stripBadChars(bstring str) {
    int n=0, i=0;

    n = str.length();

    for(i = 0; i < n; i++) {
        if(str[i] == '/') {
            str[i] = ' ';
        }
    }
}

//*********************************************************************
//                      getLastDigit
//*********************************************************************
// This function returns the last digits of any integer n sent
// to it. digits specifies how many. -Tim C.

int getLastDigit(int n, int digits) {
    double  num=0.0, sub=0.0;
    int     a=0, temp=0, digit=0;

    digits = MAX(1,MIN(4,digits));

    num = (double)n;
    for(a=5; a>digits-1; a--)   // start with 10^5th power (100000)
    {
        sub = pow(10,a);
        if(num - sub < 0)
            continue;
        else {
            temp = (int)num;
            temp %= (int)sub;
            num = (double)temp;
        }

    }

    digit = (int)num;
    return(digit);
}

//*********************************************************************
//                      ltoa
//*********************************************************************

char *ltoa(
    long val,   // value to be converted
    char *buf,  // output string
    int base)   // conversion base
{
    ldiv_t r;   // result of val / base

    // no conversion if wrong base
    if(base > 36 || base < 2) {
        *buf = '\0';
        return buf;
    }
    if(val < 0)
        *buf++ = '-';
    r = ldiv (labs(val), base);

    // output digits of val/base first

    if(r.quot > 0)
        buf = ltoa ( r.quot, buf, base);

    // output last digit
    *buf++ = "0123456789abcdefghijklmnopqrstuvwxyz"[(int)r.rem];
    *buf   = '\0';
    return(buf);
}
//*********************************************************************
//                      findCrt
//*********************************************************************

template<class SetType>
MudObject* findCrtTarget(Creature * player, SetType& set, int findFlags, const char *str, int val, int* match) {

    if(!player || !str || set.empty())
        return(nullptr);

    for(typename SetType::key_type crt : set) {
        if(!crt) {
            continue;
        }
        if(!player->canSee(crt)) {
            continue;
        }

        if(keyTxtEqual(crt, str)) {
            (*match)++;
            if(*match == val) {
                return(crt);
            }
        }

    }
    return(nullptr);
}



//*********************************************************************
//                      findTarget
//*********************************************************************

MudObject* Creature::findTarget(unsigned int findWhere, unsigned int findFlags, const bstring& str, int val) {
    int match=0;
    MudObject* target;
    do {
        if(findWhere & FIND_OBJ_INVENTORY) {
            if((target = findObjTarget(objects, findFlags, str, val, &match))) {
                break;
            }
        }
        // See if we should look for a match in the player's equipment
        // -- Do this after looking for the object in their inventory
        if(findWhere & FIND_OBJ_EQUIPMENT) {
            int n;
            bool found=false;
            for(n=0; n<MAXWEAR; n++) {
                if(!ready[n])
                    continue;
                if(keyTxtEqual(ready[n], str.c_str()))
                    match++;
                else
                    continue;
                if(val == match) {
                    target = ready[n];
                    found = true;
                    break;
                }
            }
            if(found)
                break;
        }

        if(findWhere & FIND_OBJ_ROOM) {
            if((target = findObjTarget(getRoomParent()->objects, findFlags, str, val, &match))) {
                break;
            }
        }

        if(findWhere & FIND_MON_ROOM) {
            if((target = findCrtTarget<MonsterSet>(this, getParent()->monsters, findFlags, str.c_str(), val, &match))) {
                break;
            }
        }

        if(findWhere & FIND_PLY_ROOM) {
            if((target = findCrtTarget<PlayerSet>(this, getParent()->players, findFlags, str.c_str(), val, &match))) {
                break;
            }
        }

        if(findWhere & FIND_EXIT) {
            if((target = findExit(this, str, val, getRoomParent()))) {
                break;
            }
        }
    } while(false);

    return(target);
}

//**********************************************************************
//                      new_merror
//**********************************************************************
// merror is called whenever an error message should be output to the
// log file.  If the error is fatal, then the program is aborted

void new_merror(const char *str, char errtype, const char *file, const int line) {
    std::clog << "\nError: " << str << " @ " << file << " " << line << ".\n";
    logn("error.log", "Error occured in %s in file %s line %d\n", str, file, line);
    if(errtype == FATAL) {
        abort();
    }
//  exit(-1);
}

//*********************************************************************
//                      timeStr
//*********************************************************************

bstring timeStr(int secs) {
    int hours = 0;
    int minutes = 0;
    int seconds = 0;
    hours = secs / 3600;
    secs -= hours * 3600;
    minutes = secs / 60;
    seconds = secs - minutes * 60;

    std::ostringstream timeStr;
    if(hours)
        timeStr << hours << " hour(s) ";
    if(minutes)
        timeStr << minutes << " minutes(s) ";
    // Always show seconds
    timeStr << seconds << " second(s)";

    return(timeStr.str());
}

//*********************************************************************
//                      progressBar
//*********************************************************************

bstring progressBar(int barLength, float percentFull, const bstring& text, char progressChar, bool enclosed) {
    bstring str = "";
    int i=0, progress = (int)(barLength * percentFull);
    int lowTextBound=-1, highTextBound=-1;

    if(text.getLength()) {
        if(text.getLength() >= barLength)
            return(text);
        lowTextBound = (barLength - text.getLength())/2;
        highTextBound = lowTextBound + text.getLength();
    }

    if(enclosed)
        str += "[";

    for(i=0; i<barLength; i++) {
        // if we're in the range of the text we want to include, skip
        if(i == lowTextBound) {
            str += text;
            continue;
        }
        if(i > lowTextBound && i < highTextBound)
            continue;

        // otherwise give us a character
        if(i < progress)
            str += progressChar;
        else
            str += " ";
    }

    if(enclosed)
        str += "]";
    return(str);
}

