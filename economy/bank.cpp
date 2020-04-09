/*
 * bank.cpp
 *   Bank code
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

#include <math.h>

#include "bank.hpp"
#include "config.hpp"
#include "creatures.hpp"
#include "guilds.hpp"
#include "mud.hpp"
#include "rooms.hpp"
#include "server.hpp"
#include "socket.hpp"
#include "xml.hpp"

//*********************************************************************
//                      teller
//*********************************************************************
// we get the bankteller mob in the room, if there is one

Monster* Bank::teller(const BaseRoom* room) {
    for(Monster* mons : room->monsters ) {
        if(mons->getName() == "bank teller")
            return(mons);

    }

    return(0);
}


//*********************************************************************
//                      canSee
//*********************************************************************
// we get the bankteller mob in the room, if there is one

bool Bank::canSee(const Player* player) {
    if(player->isCt())
        return(true);

    if(player->isEffected("mist")) {
        player->print("You are in mist form and cannot handle physical objects.\n");
        return(false);
    }

    if(!player->isInvisible())
        return(true);

    // magic money machines don't need to see people
    if(player->getConstRoomParent()->flagIsSet(R_MAGIC_MONEY_MACHINE))
        return(true);

    Monster* teller = Bank::teller(player->getConstRoomParent());
    if(teller && teller->canSee(player))
        return(true);

    player->print("The bank teller says, \"I don't deal with people I can't see!\"\n");
    return(false);
}


//*********************************************************************
//                      canAnywhere
//*********************************************************************
// can this person use the bank without being in a bank room?

bool Bank::canAnywhere(const Player* player) {
    return(
        (player->getClass() == CreatureClass::CLERIC && player->getDeity() == JAKAR) ||
        player->isCt()
    );
}


//*********************************************************************
//                      can
//*********************************************************************

bool Bank::can(Player* player, bool isGuild) {
    Guild* guild=0;
    return(Bank::can(player, isGuild, &guild));
}

bool Bank::can(Player* player, bool isGuild, Guild** guild) {
    player->clearFlag(P_AFK);

    if(!player->ableToDoCommand())
        return(false);

    if(player->getClass() == CreatureClass::BUILDER) {
        player->print("You do not have a bank account.\n");
        return(false);
    }

    if( !player->getRoomParent()->flagIsSet(R_BANK) &&
        !player->getRoomParent()->flagIsSet(R_LIMBO) &&
        !player->getRoomParent()->flagIsSet(R_MAGIC_MONEY_MACHINE) &&
        !Bank::canAnywhere(player)
    ) {
        player->print("You don't see a bank teller anywhere in this room!\n");
        return(false);
    }

    player->unhide();

    if(!Bank::canSee(player))
        return(false);

    // If this is a guild using a bank, load the guild
    if(isGuild) {
        if(!player->getGuild()  || player->getGuildRank() < GUILD_PEON) {
            player->print("You hail to no guild.\n");
            return(false);
        }

        (*guild) = gConfig->getGuild(player->getGuild());
        if(!*guild) {
            if(player->getRoomParent()->flagIsSet(R_BANK))
                player->print("The bank teller says, \"Your guild bank account is currently unavaiable. Try back later.\"\n");
            else
                player->print("The magic money machine reports that your guild bank account is currently unavaiable.\n");
            return(false);
        }
    }
    
    return(true);
}

//*********************************************************************
//                      say
//*********************************************************************

void Bank::say(const Player* player, const char* text) {
    if(player->getConstRoomParent()->flagIsSet(R_BANK))
        player->print("The bank teller says, \"%s\".\n", text);
    else
        player->print("The magic money machine tells you, \"%s\".\n", text);
}

//*********************************************************************
//                      balance
//*********************************************************************
// gives you your current balance

void Bank::balance(Player* player, bool isGuild) {
    Guild* guild=0;
    bool isBank = player->getRoomParent()->flagIsSet(R_BANK);
    
    // can they use the bank?
    if(!Bank::can(player, isGuild, &guild))
        return;


    if(isGuild) {
        if(guild->bank.isZero()) {
            if(isBank)
                player->print("The bank teller says, \"Your guild hasn't got a single coin in the bank!\"\n");
            else
                player->print("The magic money machine reports your guild's current balance as 0 coins.\n");
        } else {
            if(isBank)
                player->print("The bank teller says, \"The current balance for your guild is: %s.\"\n",
                    guild->bank.str().c_str());
            else
                player->print("The magic money machine reports your guild's current balance as %s.\n",
                    guild->bank.str().c_str());
        }
    } else {
        if(player->bank.isZero()) {
            if(isBank)
                player->print("The bank teller says, \"You haven't got a single coin in the bank!\"\n");
            else
                player->print("The magic money machine reports your guild's current balance as 0 coins.\n");
        } else {
            if(isBank)
                player->print("The bank teller says, \"Your current balance is: %s.\"\n",
                    player->bank.str().c_str());
            else
                player->print("The magic money machine reports your current balance as %s.\n",
                    player->bank.str().c_str());
        }
    }
}

//*********************************************************************
//                      cmdBalance
//*********************************************************************

int cmdBalance(Player* player, cmd* cmnd) {
    Bank::balance(player);
    return(0);
}

//*********************************************************************
//                      cmdDeposit
//*********************************************************************
// lets you put money in your account

void Bank::deposit(Player* player, cmd* cmnd, bool isGuild) {
    int i = (isGuild ? 2 : 1);
    unsigned long amt=0;
    Guild* guild=0;

    // can they use the bank?
    if(!Bank::can(player, isGuild, &guild))
        return;



    if(cmnd->num < (i+1)) {
        Bank::say(player, "How much do you want to deposit?");
        return;
    }

    if(!strcmp(cmnd->str[i], "all")) {
        amt = player->coins[GOLD];
    } else if(!strcmp(cmnd->str[i], "half")) {
        amt = player->coins[GOLD]/2;
    } else if(cmnd->str[i][0] != '$') {
        Bank::say(player, "How much do you want to deposit?");
        return;
    } else {
        amt = atol(&cmnd->str[i][1]);
    }

    if(amt > player->coins[GOLD] || amt < 1) {
        Bank::say(player, "You can't deposit money you don't have!");
        return;
    }


    player->coins.sub(amt, GOLD);
    
    if(isGuild) {
        player->print("You deposit %ld gold coins into your guild's bank account.\n", amt);
        guild->bank.add(amt, GOLD);
        Bank::guildLog(player->getGuild(), "DEPOSIT: %ld by %s [Balance: %s]\n", amt, player->getCName(), guild->bank.str().c_str());
        gConfig->saveGuilds();
    } else {
        player->print("You deposit %ld gold coins into your bank account.\n", amt);
        player->bank.add(amt, GOLD);
        Bank::log(player->getCName(), "DEPOSIT: %lu [Balance: %s]\n", amt, player->bank.str().c_str());
    }

    player->save(true);

    Bank::balance(player, isGuild);
    broadcast(player->getSock(), player->getParent(), "%s deposits some gold.", player->getCName());
}

//*********************************************************************
//                      cmdDeposit
//*********************************************************************

int cmdDeposit(Player* player, cmd* cmnd) {
    Bank::deposit(player, cmnd);
    return(0);
}

//*********************************************************************
//                      withdraw
//*********************************************************************
// lets you withdraw money from your account

void Bank::withdraw(Player* player, cmd* cmnd, bool isGuild) {
    int i = (isGuild ? 2 : 1);
    unsigned long amt=0;
    Guild* guild=0;

    // can they use the bank?
    if(!Bank::can(player, isGuild, &guild))
        return;



    if(isGuild && player->getGuildRank() < GUILD_BANKER) {
        player->print("You are not authorized to withdraw funds from the guild bank account.\n");
        return;
    }

    if(cmnd->num < (i+1)) {
        Bank::say(player, "How much do you want to withdraw?");
        return;
    }

    if(!strcmp(cmnd->str[i], "all")) {
        amt = (isGuild ? guild->bank[GOLD] : player->bank[GOLD]);
    } else if(!strcmp(cmnd->str[i], "half")) {
        amt = (isGuild ? guild->bank[GOLD] : player->bank[GOLD])/2;
    } else if(cmnd->str[i][0] != '$') {
        Bank::say(player, "How much do you want to withdraw?");
        return;
    } else {
        amt = atol(&cmnd->str[i][1]);
    }

    if(isGuild) {
        if(amt > guild->bank[GOLD] || amt < 1) {
            Bank::say(player, "You can't withdraw money your guild doesn't have!");
            return;
        }
    } else {
        if(amt > player->bank[GOLD] || amt < 1) {
            Bank::say(player, "You can't withdraw money you don't have!");
            return;
        }
    }

    
    player->coins.add(amt, GOLD);
    
    if(isGuild) {
        player->print("You withdraw %ld gold coins from your guild's bank account.\n", amt);
        guild->bank.sub(amt, GOLD);
        Bank::guildLog(player->getGuild(), "WITHDRAW: %ld by %s [Balance: %s]\n", amt, player->getCName(), guild->bank.str().c_str());
        gConfig->saveGuilds();
    } else {
        player->print("You withdraw %ld gold coins from your bank account.\n", amt);
        player->bank.sub(amt, GOLD);
        Bank::log(player->getCName(), "WITHDRAW: %ld [Balance: %s]\n", amt, player->bank.str().c_str());
    }

    player->save(true);
    
    Bank::balance(player, isGuild);
    broadcast(player->getSock(), player->getParent(), "%s withdrew some gold.", player->getCName());
}

//*********************************************************************
//                      cmdWithdraw
//*********************************************************************

int cmdWithdraw(Player* player, cmd* cmnd) {
    Bank::withdraw(player, cmnd);
    return(0);
}

//*********************************************************************
//                      cmdTransfer
//*********************************************************************
// lets you move funds

void Bank::transfer(Player* player, cmd* cmnd, bool isGuild) {
    int i = (isGuild ? 2 : 1);
    Player* target=0;
    unsigned long amt=0;
    bool    online=false;
    Guild* guild=0;

    // can they use the bank?
    if(!Bank::can(player, isGuild, &guild))
        return;



    if(isGuild && player->getGuildRank() < GUILD_BANKER) {
        player->print("You are not authorized to transfer funds from the guild bank account.\n");
        return;
    }

    if(cmnd->num < (i+1)) {
        Bank::say(player, "How much do you want to transfer?");
        return;
    }

    if(!strcmp(cmnd->str[i], "all")) {
        amt = (isGuild ? guild->bank[GOLD] : player->bank[GOLD]);
    } else if(!strcmp(cmnd->str[i], "half")) {
        amt = (isGuild ? guild->bank[GOLD] : player->bank[GOLD])/2;
    } else if(cmnd->str[i][0] != '$') {
        Bank::say(player, "How much do you want to transfer?");
        return;
    } else {
        amt = atol(&cmnd->str[i][1]);
    }

    if(isGuild) {
        if(amt > guild->bank[GOLD] || amt < 1) {
            Bank::say(player, "You can't transfer money your guild doesn't have!");
            return;
        }
    } else {
        if(amt > player->bank[GOLD] || amt < 1) {
            Bank::say(player, "You can't transfer money you don't have!");
            return;
        }
    }

    if(amt < 500 && !player->isDm()) {
        Bank::say(player, "It's not worth it to transfer so few coins!");
        return;
    }
    
    if(cmnd->num < (i+2)) {
        Bank::say(player, "Who is this money going to?");
        return;
    }

    // load up the player if nessesarry
    cmnd->str[i+1][0] = up(cmnd->str[i+1][0]);

    target = gServer->findPlayer(cmnd->str[i+1]);

    if(!target) {
        if(!loadPlayer(cmnd->str[2], &target)) {
            Bank::say(player, "I don't know who that is.");
            return;
        }
    } else
        online = true;

    if(target->getClass() == CreatureClass::BUILDER) {
        Bank::say(player, "I don't know who that is.");
        if(!online)
            free_crt(target);
        return;
    }

    if(!isGuild && player->getName() == target->getName()) {
        Bank::say(player, "You can't transfer money to yourself.");
        return;
    }

    // OCEANCREST: Dom: HC: only for tournament
    //if(target->isHardcore()) {
    //  player->print("You cannot transfer money to hardcore characters for the duration of the tournament.\n");
    //  if(!online)
    //      free_crt(target);
    //  return;
    //}

    if(!online)
        target->computeInterest(0, false);



    if(isGuild) {
        guild->bank.sub(amt, GOLD);
        player->print("You transfer %ld gold coins from your guild's account to %s's.\n", amt, target->getCName());
        Bank::guildLog(player->getGuild(), "TRANSFER to %s by %s: %ld [Balance: %s]\n", target->getCName(), player->getCName(), amt, guild->bank.str().c_str());
        Bank::log(target->getCName(), "TRANSFER from %s [GUILD ACCOUNT]: %ld [Balance: %s]\n", player->getCName(), amt, target->bank.str().c_str());
        if(online)
            target->print("*** %s just transferred %ld gold to your account. [GUILD TRANSFER]\n",player->getCName(), amt);
        gConfig->saveGuilds();
    } else {
        player->bank.sub(amt, GOLD);
        player->print("You transfer %lu gold coins from your account to %s's.\n", amt, target->getCName());
        Bank::log(player->getCName(), "TRANSFER to %s: %ld [Balance: %s]\n", target->getCName(), amt, player->bank.str().c_str());
        Bank::log(target->getCName(), "TRANSFER from %s: %ld [Balance: %s]\n", player->getCName(), amt, target->bank.str().c_str());
        if(online)
            target->print("*** %s just transferred %ld gold to your account.\n", player->getCName(), amt);
    }

    if(player->getClass() == CreatureClass::CARETAKER)
        log_immort(true, player, "%s transferred %lu gold to %s. (Balance=%s)(%s)\n", player->getCName(), amt, target->getCName(), player->bank.str().c_str(), target->bank.str().c_str());

    target->bank.add(amt, GOLD);

    player->save(true);
    target->save(online);
    if(!online)
        free_crt(target);

    Bank::balance(player, isGuild);
    broadcast(player->getSock(), player->getParent(), "%s transfers some gold.", player->getCName());
}

//*********************************************************************
//                      cmdTransfer
//*********************************************************************

int cmdTransfer(Player* player, cmd* cmnd) {
    Bank::transfer(player, cmnd);
    return(0);
}

//*********************************************************************
//                      statement
//*********************************************************************
// lets someone view their transaction history

void Bank::statement(Player* player, bool isGuild) {
    char file[80];
    bool isBank = player->getRoomParent()->flagIsSet(R_BANK);

    // can they use the bank?
    if(!Bank::can(player, isGuild))
        return;


    
    if(isGuild) {
        if(isBank)
            player->print("The bank teller shows you your guild's bank statement:\n");
        else
            player->print("The magic money machine displays your guild's bank statement:\n");
    } else {
        if(isBank)
            player->print("The bank teller shows you your bank statement:\n");
        else
            player->print("The magic money machine displays your bank statement:\n");
    }


    if(isGuild)
        sprintf(file, "%s/%d.txt", Path::GuildBank, player->getGuild());
    else
        sprintf(file, "%s/%s.txt", Path::Bank, player->getCName());

    if(file_exists(file)) {
        strcpy(player->getSock()->tempstr[3], "\0");
        viewFileReverse(player->getSock(), file);
    } else {
        player->print("\n   There is no transaction history.\n");
    }
}

//*********************************************************************
//                      cmdStatement
//*********************************************************************

int cmdStatement(Player* player, cmd* cmnd) {
    Bank::statement(player);
    return(0);
}

//*********************************************************************
//                      deleteStatement
//*********************************************************************

void Bank::deleteStatement(Player* player, bool isGuild) {
    char file[80];

    // can they use the bank?
    if(!Bank::can(player, isGuild))
        return;



    if(isGuild && player->getGuildRank() < GUILD_BANKER) {
        player->print("You don't have authorization to delete the guild transaction history.\n");
        return;
    }


    if(isGuild)
        sprintf(file, "%s/%d.txt", Path::GuildBank, player->getGuild());
    else
        sprintf(file, "%s/%s.txt", Path::Bank, player->getCName());

    player->print("Statement deleted.\n");
    unlink(file);
}

//*********************************************************************
//                      cmdDeleteStatement
//*********************************************************************

int cmdDeleteStatement(Player* player, cmd* cmnd) {
    Bank::deleteStatement(player);
    return(0);
}

//*********************************************************************
//                      getInterest
//*********************************************************************

long Player::getInterest(long principal, double annualRate, long seconds) {
    // Effective interest rate, compounded continuously
    double r = log(annualRate + 1);

    double interest = (principal * exp( (r / (60.0*60.0*24.0*365.0) * seconds)) )-principal;

    return((long)interest);
}

//*********************************************************************
//                      computeInterest
//*********************************************************************
// Computes interest the player has earned. If online = false, it is the responsibility
// of the calling function to save the player.

void Player::computeInterest(long t, bool online) {
    if(isStaff())
        return;
    // people in jail don't earn interest
    if(inJail()) {
        lastInterest = time(0);
        if(online)
            save(true);
        return;
    }

    if(!t)
        t = time(0);
    if(!lastInterest)
        lastInterest = lastLogin;

    int interestDays = (t - lastInterest) / (60*60*24);
    int rateDays = (t - lastLogin) / (60*60*24);
    double rate = 0.0;

    // check interest once a day
    if(!interestDays)
        return;

    // determine what rate they earn based on how long since they last logged in
    if(rateDays < 2) {
        rate = 0.08;
    } else if(rateDays < 14) {
        rate = 0.06;
    } else if(rateDays < 30) {
        rate = 0.02;
    }

    // calculate interest based on when we last checked
    long amt = getInterest(bank[GOLD], rate, t - lastInterest);

    // if they don't get any money because they don't have enough in the bank,
    // don't record this computation.

    // BUG: But do record the last time we checked interest!
    // If we don't someone could have a really old character that has never
    // had a bank balance, deposit a million gold, have them re-log
    // and they'll earn interest on that million gold for the life of their
    // character!
    lastInterest = t;

    if(!amt)
        return;

    bank.add(amt, GOLD);
    gServer->logGold(GOLD_IN, this, Money(amt, GOLD), nullptr, "BankInterest");

    Bank::log(getCName(), "INTEREST for %d day%s at %d%%: %ld [Balance: %s]\n",
        interestDays, interestDays != 1 ? "s" : "", (int)(rate*100), amt, bank.str().c_str());

    if(online) {
        print("You earn %d gold coins in interest.\n", amt);
        save(true);
    }
}

//**********************************************************************
//                      doLog
//**********************************************************************

void Bank::doLog(const char *file, const char *str) {
    int fd = open(file, O_RDWR | O_APPEND, 0);
    
    if(fd < 0) {
        fd = open(file, O_RDWR | O_CREAT, ACC);
        if(fd < 0)
            return;
    }

    lseek(fd, 0L, 2);

    write(fd, str, strlen(str));

    close(fd);
}

//**********************************************************************
//                      log
//**********************************************************************

void Bank::log(const char *name, const char *fmt, ...) {
    char    file[80];
    char    str[2048];
    long    t = time(0);
    va_list ap;

    va_start(ap, fmt);

    sprintf(file, "%s/%s.txt", Path::Bank, name);

    strcpy(str, ctime(&t));
    str[24] = ':';
    str[25] = ' ';
    vsnprintf(str + 26, 2000, fmt, ap);
    va_end(ap);

    Bank::doLog(file, str);

    sprintf(file, "%s/%s.txt", Path::BankLog, name);
    Bank::doLog(file, str);
}

//**********************************************************************
//                      guildLog
//**********************************************************************

void Bank::guildLog(int guild, const char *fmt, ...) {
    char    file[80];
    char    str[2048];
    long    t = time(0);
    va_list ap;

    va_start(ap, fmt);

    sprintf(file, "%s/%d.txt", Path::GuildBank, guild);

    strcpy(str, ctime(&t));
    str[24] = ':';
    str[25] = ' ';
    vsnprintf(str + 26, 2000, fmt, ap);
    va_end(ap);

    Bank::doLog(file, str);

    sprintf(file, "%s/%d.txt", Path::GuildBankLog, guild);
    Bank::doLog(file, str);
}
