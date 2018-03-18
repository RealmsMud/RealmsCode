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
 *  Copyright (C) 2007-2018 Jason Mitchell, Randi Mitchell
 *     Contributions by Tim Callahan, Jonathan Hseu
 *  Based on Mordor (C) Brooke Paul, Brett J. Vickers, John P. Freeman
 *
 */

#include <ctime>

#include "commands.hpp"
#include "config.hpp"
#include "creatures.hpp"
#include "login.hpp"
#include "mud.hpp"
#include "rooms.hpp"
#include "server.hpp"
#include "socket.hpp"

//*********************************************************************
//                      validId functions
//*********************************************************************

bool validMobId(const CatRef cr) {
    return(!cr.isArea("") && cr.id > 0 && cr.id < MMAX);
}
bool validObjId(const CatRef cr) {
    // 0 = coins
    return(!cr.isArea("") && cr.id >= 0 && cr.id < OMAX);
}
bool validRoomId(const CatRef cr) {
    // 0 = void
    return(!cr.isArea("") && cr.id >= 0 && cr.id < RMAX);
}



//*********************************************************************
//                      nameEqual
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
//                      nameEqual
//*********************************************************************
// checks to see if the names are equal.

bool nameEqual(bstring obj, bstring str) {
    if(obj=="" || str=="")
        return(false);

    if(!strncasecmp(removeColor(obj).c_str(), str.c_str(), str.length()))
        return(true);

    return(false);
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

int bonus(int num) {
    return(statBonus[MIN(num, MAX_STAT_NUM)/10]);
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
//                      viewFile
//*********************************************************************
// This function views a file whose name is given by the third
// parameter. If the file is longer than 20 lines, then the user is
// prompted to hit return to continue, thus dividing the output into
// several pages.

#define FBUF    800

void viewFileReal(Socket* sock, bstring str ) {
    char    buf[FBUF+1];
    int i, l, n, ff, line;
    long    offset;

    buf[FBUF] = 0;
    switch(sock->getParam()) {
    case 1:
        offset = 0L;
        strncpy(sock->tempstr[1], str.c_str(),255);
        sock->tempstr[1][255] = 0;
        ff = open(str.c_str(), O_RDONLY, 0);
        if(ff < 0) {
            sock->print("File could not be opened.\n");
            if(sock->getPlayer())
                sock->getPlayer()->clearFlag(P_READING_FILE);
            sock->restoreState();
            return;
        }
        line = 0;
        while(1) {
            n = read(ff, buf, FBUF);
            l = 0;
            for(i=0; i<n; i++) {
                if(buf[i] == '\n') {
                    buf[i] = 0;
                    line++;
                    sock->bprint(&buf[l]);
                    sock->bprint("\n");
                    offset += (i-l+1);
                    l = i+1;
                }
                if(line > 20)
                    break;
            }
            if(line > 20) {
                sprintf(sock->tempstr[2], "%ld", offset);
                break;
            } else if(l != n) {
                sock->bprint(&buf[l]);
                offset += (i-l);
            }
            if(n<FBUF)
                break;
        }
        if(n==FBUF || line>20) {
            sock->getPlayer()->setFlag(P_READING_FILE);
            if(!sock->getPlayer()->flagIsSet(P_MIRC))
                sock->print("[Hit Return, Q to Quit]: ");
            else
                sock->print("[Hit C to continue]: ");
            gServer->processOutput();
        }
        if(n<FBUF && line <= 20) {
            close(ff);
            sock->restoreState();
            return;
        } else {
            close(ff);
            sock->getPlayer()->setFlag(P_READING_FILE);
            sock->setState(CON_VIEWING_FILE, 2);
            return;
        }
    case 2:
        if(str[0] != 0  && str[0] != 'c' && str[0] != 'C') {
            sock->print("Aborted.\n");
            if(sock->getPlayer())
                sock->getPlayer()->clearFlag(P_READING_FILE);
            sock->restoreState();
            return;
        }
        offset = atol(sock->tempstr[2]);
        ff = open(sock->tempstr[1], O_RDONLY, 0);
        if(ff < 0) {
            sock->print("File could not be opened [%s].\n", sock->tempstr);
            if(sock->getPlayer())
                sock->getPlayer()->clearFlag(P_READING_FILE);
            sock->restoreState();
            return;
        }
        lseek(ff, offset, 0);
        line = 0;
        while(1) {
            n = read(ff, buf, FBUF);
            l = 0;
            for(i=0; i<n; i++) {
                if(buf[i] == '\n') {
                    buf[i] = 0;
                    line++;
                    sock->bprint(&buf[l]);
                    sock->bprint("\n");
                    offset += (i-l+1);
                    l = i+1;
                }
                if(line > 20)
                    break;
            }
            if(line > 20) {
                sprintf(sock->tempstr[2], "%ld", offset);
                break;
            } else if(l != n) {
                sock->bprint(&buf[l]);
                offset += (i-l);
            }
            if(n<FBUF)
                break;
        }
        if(n==FBUF || line > 20) {
            if(sock->getPlayer())
                sock->getPlayer()->setFlag(P_READING_FILE);
            sock->print("[Hit Return, Q to Quit]: ");
            gServer->processOutput();
        }
        if(n<FBUF && line <= 20) {
            close(ff);
            if(sock->getPlayer())
                sock->getPlayer()->clearFlag(P_READING_FILE);
            sock->restoreState();
            return;
        } else {
            close(ff);
            sock->setState(CON_VIEWING_FILE, 2);
        }
    }
}
// Wrapper function for viewFile_real that will set the correct connected state
void viewFile(Socket* sock, bstring str) {
    if(sock->getState() != CON_VIEWING_FILE)
        sock->setState(CON_VIEWING_FILE);

    viewFileReal(sock, str);
}

//*********************************************************************
//                      viewLoginFile
//*********************************************************************
// This function views a file whose name is given by the third
// parameter. If the file is longer than 20 lines, then the user is
// prompted to hit return to continue, thus dividing the output into
// several pages.

#define FBUF_L  1024

void viewLoginFile(Socket* sock, bstring str, bool showError) {
    char    buf[FBUF_L + 1];
    int     i=0, l=0, n=0, ff=0, line=0;
    long    offset=0;
    zero(buf, sizeof(buf));

    buf[FBUF_L] = 0;
    {
        offset = 0L;
        strcpy(sock->tempstr[1], str.c_str());
        ff = open(str.c_str(), O_RDONLY, 0);
        if(ff < 0) {
            if(showError) {
                sock->print("File could not be opened.\n");
                broadcast(isCt, "^yCan't open file: %s.\n", str.c_str()); // nothing to put into (%m)?
            }
            return;
        }
        line = 0;
        while(1) {
            n = read(ff, buf, FBUF_L);
            l = 0;
            for(i=0; i<n; i++) {
                if(buf[i] == '\n') {
                    buf[i] = 0;
                    if(i != 0 && buf[i-1] == '\r')
                        buf[i-1] = 0;
                    line++;
                    sock->printColor("%s\n", &buf[l]);
                    offset += (i - l + 1);
                    l = i + 1;
                }
            }
            if(l != n) {
                sock->printColor("%s", &buf[l]);
                offset += (i - l);
            }
            if(n < FBUF_L) {
                close(ff);
                return;
            }
        }
        // Never makes it out of the while loop to get here
//      close(ff);
    }
}

//*********************************************************************
//                      viewFileReverseReal
//*********************************************************************
// displays a file, line by line starting with the last
// similar to unix 'tac' command

void viewFileReverseReal(Socket* sock, bstring str) {
    off_t oldpos;
    off_t newpos;
    off_t temppos;
    int i,more_file=1,count,amount=1621;
    char string[1622];
    char search[80];
    long offset;
    FILE *ff;
    int TACBUF = ( (81 * 20 * sizeof(char)) + 1 );

    if(strlen(sock->tempstr[3]) > 0)
        strcpy(search, sock->tempstr[3]);
    else
        strcpy(search, "\0");

    switch(sock->getParam()) {
    case 1:

        strcpy(sock->tempstr[1], str.c_str());
        if((ff = fopen(str.c_str(), "r")) == nullptr) {
            sock->print("error opening file\n");
            sock->restoreState();
            return;
        }



        fseek(ff, 0L, SEEK_END);
        oldpos = ftell(ff);
        if(oldpos < 1) {
            sock->print("Error opening file\n");
            sock->restoreState();
            return;
        }
        break;

    case 2:

        if(str[0] != 0) {
            sock->print("Aborted.\n");
            sock->getPlayer()->clearFlag(P_READING_FILE);
            sock->restoreState();
            return;
        }

        if((ff = fopen(sock->tempstr[1], "r")) == nullptr) {
            sock->print("error opening file\n");
            sock->getPlayer()->clearFlag(P_READING_FILE);
            sock->restoreState();
            return;
        }

        offset = atol(sock->tempstr[2]);
        fseek(ff, offset, SEEK_SET);
        oldpos = ftell(ff);
        if(oldpos < 1) {
            sock->print("Error opening file\n");
            sock->restoreState();
            return;
        }

    }

nomatch:
    temppos = oldpos - TACBUF;
    if(temppos > 0)
        fseek(ff, temppos, SEEK_SET);
    else {
        fseek(ff, 0L, SEEK_SET);
        amount = oldpos;
    }

    newpos = ftell(ff);


    fread(string, amount,1, ff);
    string[amount] = '\0';
    i = strlen(string);
    i--;

    count = 0;
    while(count < 21 && i > 0) {
        if(string[i] == '\n') {
            if( (   strlen(search) > 0 &&
                    strstr(&string[i], search)
                ) ||
                search[0] == '\0'
            ) {
                sock->printColor("%s", &string[i]);
                count++;
            }
            string[i]='\0';
            if(string[i-1] == '\r')
                string[i-1]='\0';
        }
        i--;
    }

    oldpos = newpos + i + 2;
    if(oldpos < 3)
        more_file = 0;

    sprintf(sock->tempstr[2], "%ld", (long) oldpos);


    if(more_file && count == 0)
        goto nomatch;       // didnt find a match within a screenful
    else if(more_file) {
        sock->print("\n[Hit Return, Q to Quit]: ");
        gServer->processOutput();
        sock->intrpt &= ~1;

        fclose(ff);
        sock->getPlayer()->setFlag(P_READING_FILE);
        sock->setState(CON_VIEWING_FILE_REVERSE, 2);
        return;
    } else {
        if((strlen(search) > 0 && strstr(string, search))
                || search[0] == '\0') {
            sock->print("\n%s\n", string);
        }
        fclose(ff);
        sock->getPlayer()->clearFlag(P_READING_FILE);
        sock->restoreState();
        return;
    }

}

// Wrapper for viewFileReverse_real that properly sets the connected state
void viewFileReverse(Socket* sock, bstring str) {
    if(sock->getState() != CON_VIEWING_FILE_REVERSE)
        sock->setState(CON_VIEWING_FILE_REVERSE);
    viewFileReverseReal(sock, str);
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
        p += mrand(1,s);

    return(p);
}

//*********************************************************************
//                      exp_to_lev
//*********************************************************************
// This function takes a given amount of experience as its first
// argument returns the level that the experience reflects.

int exp_to_lev(unsigned long exp) {
    int level = 1;

    while(exp >= gConfig->expNeeded(level) && level < MAXALVL)
        level++;
    if(level == MAXALVL) {
        level = exp/(needed_exp[MAXALVL-1]);
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
    struct tm   *tm, time1, time2;

    t = time(0);
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
    long        t = time(0);
    struct tm   *tm, time1, time2;

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

bool isdm(bstring name) {
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

bool parse_name(bstring name) {
    FILE    *fp=0;
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
        if(strcmp(dmname[i], str)) {
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
    Player* player=0;
    int     idle=0;
    long    t = time(0);

    for( std::pair<bstring, Player*> p : gServer->players) {
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
    long    t = time(0);
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
    lasttime[LT_KICK].ltime = time(0);
    lasttime[LT_SPELL].ltime = time(0);
    lasttime[LT_READ_SCROLL].ltime = time(0);
    lasttime[LT_KICK].interval = delay+1;
    lasttime[LT_SPELL].interval = delay+1;
    lasttime[LT_READ_SCROLL].interval = delay+1;
    if(isPlayer()) {
        lasttime[LT_PLAYER_STUNNED].ltime = time(0);
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
    char *name=0;

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

MudObject* Creature::findTarget(int findWhere, int findFlags, bstring str, int val) {
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
    } while(0);

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

bstring progressBar(int barLength, float percentFull, bstring text, char progressChar, bool enclosed) {
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

