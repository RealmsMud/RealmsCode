/*
 * bank.h
 *	 Header file for bank code
 *   ____            _
 *  |  _ \ ___  __ _| |_ __ ___  ___
 *  | |_) / _ \/ _` | | '_ ` _ \/ __|
 *  |  _ <  __/ (_| | | | | | | \__ \
 *  |_| \_\___|\__,_|_|_| |_| |_|___/
 *
 * Permission to use, modify and distribute is granted via the
 *  Creative Commons - Attribution - Non Commercial - Share Alike 3.0 License
 *    http://creativecommons.org/licenses/by-nc-sa/3.0/
 *
 * 	Copyright (C) 2007-2012 Jason Mitchell, Randi Mitchell
 * 	   Contributions by Tim Callahan, Jonathan Hseu
 *  Based on Mordor (C) Brooke Paul, Brett J. Vickers, John P. Freeman
 *
 */

#ifndef _BANK_H
#define	_BANK_H

class Guild;

namespace Bank {
	bool canSee(const Player* player);
	bool can(Player* player, bool isGuild);
	bool can(Player* player, bool isGuild, Guild** guild);
	bool canAnywhere(const Player* player);
	void say(const Player* player, const char* text);
	Monster* teller(const BaseRoom* room);

	void balance(Player* player, bool isGuild=false);
	void deposit(Player* player, cmd* cmnd, bool isGuild=false);
	void withdraw(Player* player, cmd* cmnd, bool isGuild=false);
	void transfer(Player* player, cmd* cmnd, bool isGuild=false);
	void statement(Player* player, bool isGuild=false);
	void deleteStatement(Player* player, bool isGuild=false);

	void doLog(const char *file, const char *str);
	void log(const char *name, const char *fmt, ...);
	void guildLog(int guild, const char *fmt,...);
};

#endif	/* _BANK_H */

