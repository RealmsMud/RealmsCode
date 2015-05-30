/*
 * spelling.cpp
 *	 Spell checker done by David Breitz
 *   ____            _
 *  |  _ \ ___  __ _| |_ __ ___  ___ 
 *  | |_) / _ \/ _` | | '_ ` _ \/ __|
 *  |  _ <  __/ (_| | | | | | | \__ \
 *  |_| \_\___|\__,_|_|_| |_| |_|___/
 *
 * Permission to use, modify and distribute is granted via the
 *  GNU Affero General Public License v3 or later
 *  
 * 	Copyright (C) 2007-2012 Jason Mitchell, Randi Mitchell
 * 	   Contributions by Tim Callahan, Jonathan Hseu
 *  Based on Mordor (C) Brooke Paul, Brett J. Vickers, John P. Freeman
 *
 */
#include <pspell/pspell.h>
#include "mud.h"

#define SP_MODE_LIST		1
#define SP_MODE_ADVISE		2
#define SP_MODE_HIGHLIGHT	4

static void		check_spelling(Player* player,cmd* cmnd);
static void		learn_spelling(Player* player,cmd* cmnd);
static void		forget_spelling(Player* player,cmd* cmnd);
static void		do_spelling_check(Player* player, int mode, bstring str);
/*static void		do_spelling_learn(Player* player,cmd *cmd);*/
static void		print_word_list(Player* player, const PspellWordList *wl);

PspellCanHaveError	*ret;
PspellManager		*manager;
PspellConfig		*config;
int			 sp_initialized = 0;
char			*sp_language = NULL;
char			*sp_spelling = NULL;
char			*sp_jargon = NULL;
char			*sp_module = NULL;
const char			*sp_delimiters = " !.,:;-\n\t";
const char			*sp_ignores = "'";

const char *spellingSyntax =
    "Syntax: *spell check [-s|-l|-a]\n"
    "        *spell learn <word>\n";

void cleanup_spelling(void) {
	if(manager)
		delete_pspell_manager(manager);
}
void init_spelling(void) {
	config = new_pspell_config();
	if(sp_language != NULL)
		pspell_config_replace(config,"language-tag",sp_language);
	if(sp_spelling != NULL)
		pspell_config_replace(config,"spelling",sp_spelling);
	if(sp_jargon != NULL)
		pspell_config_replace(config,"jargon",sp_jargon);
	if(sp_module != NULL)
		pspell_config_replace(config,"module",sp_module);
	ret = new_pspell_manager(config);
	delete_pspell_config(config);
	if(pspell_error_number(ret) != 0) {
		fprintf(stderr,"%s\n",pspell_error_message(ret));
		return;
	}
	sp_initialized = 1;

	manager = to_pspell_manager(ret);
	config = pspell_manager_config(manager);
}

int dmSpelling(Player* player, cmd* cmnd) {

	if(sp_initialized) {
		if((strcmp(cmnd->str[1],"check") == 0) || (cmnd->num < 2)) {
			check_spelling(player,cmnd);
		} else if(strcmp(cmnd->str[1],"learn") == 0) {
			if(!player->isCt()) {
				player->print("Only CTs and DMs may do that.\n");
				return(0);
			} else
				learn_spelling(player,cmnd);
		} else if(strcmp(cmnd->str[1],"forget") == 0) {
			forget_spelling(player,cmnd);
		} else {
			player->print(spellingSyntax);
			return(0);
		}
	} else {
		player->print("spelling not initialized\n");
	}

	return(0);
}

static void check_spelling(Player* player, cmd* cmnd) {
	int	i=0;
	int	exit_desc_seen=0;
	int	long_desc_seen=0;
	int	short_desc_seen=0;
	int	list_seen=0;
	int	exit_desc=1;
	int	long_desc=1;
	int	short_desc=1;
	int	mode=SP_MODE_LIST;

	if(!needUniqueRoom(player))
		return;
	
	for(i = 2; i < COMMANDMAX; i++) {
		if(strcmp(cmnd->str[i],"-a") == 0 && cmnd->num > 2) {
			mode |= SP_MODE_ADVISE;
		} else if(strcmp(cmnd->str[i],"-h") == 0 && cmnd->num > 2) {
			mode |= SP_MODE_HIGHLIGHT;
			if(!list_seen)
				mode &= ~SP_MODE_LIST;
		} else if(strcmp(cmnd->str[i],"-l") == 0 && cmnd->num > 2) {
			mode |= SP_MODE_LIST;
			list_seen = 1;
		} else if(strcmp(cmnd->str[i],"-L") == 0 && cmnd->num > 2 ) {
			long_desc = 1;
			long_desc_seen = 1;
			if(!short_desc_seen)
				short_desc = 0;
			if(!exit_desc_seen)
				exit_desc = 0;
		} else if(strcmp(cmnd->str[i],"-S" ) == 0 && cmnd->num > 2) {
			short_desc = 1;
			short_desc_seen = 1;
			if(!long_desc_seen)
				long_desc = 0;
			if(!exit_desc_seen)
				exit_desc = 0;
		} else if(strcmp(cmnd->str[i],"-X" ) == 0 && cmnd->num > 2) {
			exit_desc = 1;
			exit_desc_seen = 1;
			if(!long_desc_seen)
				long_desc = 0;
			if(!short_desc_seen)
				short_desc = 0;
		}
	}

	if(short_desc) {
		player->print("Checking for misspelled words in short description...\n");
		do_spelling_check(player, mode, player->getUniqueRoomParent()->getShortDescription());
	}

	if(long_desc) {
		player->print("Checking for misspelled words in long description...\n");
		do_spelling_check(player, mode, player->getUniqueRoomParent()->getLongDescription());
	}

	if(exit_desc && !player->getUniqueRoomParent()->exits.empty()) {
		player->print("Checking for misspelled words in exit descriptions...\n");
		for(Exit* ext : player->getUniqueRoomParent()->exits) {
			player->print("   %s:\n", ext->getCName());
			do_spelling_check(player, mode, ext->getDescription());
		}
	}
}

static void learn_spelling(Player* player,cmd* cmnd) {
	int	i;

	for(i = 2; i < COMMANDMAX && cmnd->str[i][0] != '\0'; i++) {
		pspell_manager_add_to_personal(manager,cmnd->str[i], strlen(cmnd->str[i]));
		player->print("%s is now added to the RoH dictionary.\n", cmnd->str[i], strlen(cmnd->str[i]));
	}

	pspell_manager_save_all_word_lists(manager);
}

static void forget_spelling(Player* player,cmd* cmnd) {
	int	i=0;

	/* Well, pspell doesn't seem to have a function for removing
	   a word from a personal word list, so we'll need to get
	   the path of the personal wordlist file, open the file, 
	   lock the file for reading and writing, remove each line
	   containing one of the words we're forgetting, save the
	   file, unlock the file, and force pspell to reload the
	   wordlist. Somehow... Does pspell cache these wordlists,
	   or does it read them each time it accesses them? Can
	   we iterate over the personal wordlist, removing items
	   as we go, without hosing the entire spellchecker?
	   Are commands in Realms re-entrant?  (Can multiple
	   instances of the spelling function be running at once?)
	   If not, pspell will never be reading a wordlist while
	   we're trying to modify it, so we shouldn't have a 
	   problem so long as we can iterate over the personal
	   wordlist. */

	for(i = 2; i < COMMANDMAX && cmnd->str[i][0] != '\0'; i++) {
		/* This should check if the word appears in any of our
		   wordlists.  If it doesn't, we should tell the player
		   that we never knew how to spell that in the first place. */
		player->print("Oh, no!  I've just forgotten how to spell ");
		player->print(cmnd->str[i]);
		player->print("!\n");

		/* But for now, we use the crude method of obtaining the
		   wordlist pathname, locking the file, and removing the
		   word. */
	}

	/*pspell_manager_save_all_word_lists(manager);*/
}

static void do_spelling_check(Player* player, int mode, bstring str) {
	char	*tmp;
	char	*tok;
	char	*word;
	char	*buf = 0;
	int	 wordlen;
	int	 i;

	if(str.c_str() != NULL) {
		if((tmp = strdup(str.c_str())) == NULL)
			return;

		tok = strtok_r(tmp, sp_delimiters, &buf);

		while(tok != NULL) {
			wordlen = strcspn(tok, sp_ignores);
			word = (char *)malloc((wordlen+1) * sizeof(char));

			if(word != NULL) {
				for(i = 0; i <= wordlen; i++)
					word[i] = '\0';
				strncpy(word, tok, wordlen);

				if(pspell_manager_check(manager,word,strlen(word)) == 0) {
					if(mode & SP_MODE_HIGHLIGHT) {
						player->print("Sorry, I don't know how to highlight your spelling errors yet.\n");
					}

					if(mode & SP_MODE_LIST) {
						player->print("\t");
						player->print(word);

						if(mode & SP_MODE_ADVISE) {
							player->print(" -> ");
							print_word_list(player, pspell_manager_suggest(manager, word, strlen(word)));
						}

						player->print("\n");
					}
				}

				free(word);
			}

			tok = strtok_r(NULL,sp_delimiters,&buf);
		}

		free(tmp);
	}
}

/*
static void do_spelling_learn(Player* player,cmd *cmd)
{
}
*/

static void print_word_list(Player* player,const PspellWordList *wl) {
	if(wl != 0) {
		PspellStringEmulation	*emu = pspell_word_list_elements(wl);
		const char		*word;

		while((word = pspell_string_emulation_next(emu)) != 0) {
			player->print((char *) word);
			player->print(" ");
		}
	} else {
		fprintf(stderr,"%s\n",pspell_manager_error_message(manager));
	}
}
