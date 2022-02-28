/*
 * flahs.h
 *     Room, object, player, and monster flags.
 *   ____            _
 *  |  _ \ ___  __ _| |_ __ ___  ___
 *  | |_) / _ \/ _` | | '_ ` _ \/ __|
 *  |  _ <  __/ (_| | | | | | | \__ \
 *  |_| \_\___|\__,_|_|_| |_| |_|___/
 *
 * Permission to use, modify and distribute is granted via the
 *  GNU Affero General Public License v3 or later
 *
 *     Copyright (C) 2007-2021 Jason Mitchell, Randi Mitchell
 *        Contributions by Tim Callahan, Jonathan Hseu
 *  Based on Mordor (C) Brooke Paul, Brett J. Vickers, John P. Freeman
 *
 */
#ifndef FLAGS_H_
#define FLAGS_H_

// partial owner flags
#define PROP_STOR_VIEW_LOG          0      // view log
#define PROP_SHOP_VIEW_LOG          0      // view log
#define PROP_SHOP_CAN_STOCK         1      // can stock
#define PROP_SHOP_CAN_REMOVE        2      // can remove
#define PROP_GUILD_PUBLIC           0      // public
#define PROP_HOUSE_PRIVATE          0      // private

// Room flags
#define R_SHOP                      0      // Shoppe
#define R_DUMP_ROOM                 1      // Dump
// training                         2
#define R_TRAINING_ROOM             3      // Training class bits (2-6)
// training                         4
// training                         5
// training                         6
#define R_MOB_JAIL                  7      // A jail room a monster takes you to (as opposed to staff)
#define R_DARK_ALWAYS               8      // Room is dark always
#define R_DARK_AT_NIGHT             9      // Room is dark at night
#define R_POST_OFFICE               10      // Post office
#define R_SAFE_ROOM                 11      // Safe room, no playerkilling
#define R_NO_TELEPORT               12      // Cannot teleport to this room
#define R_FAST_HEAL                 13      // Heal faster in this room
#define R_ONE_PERSON_ONLY           14      // 1-player only inside
#define R_TWO_PEOPLE_ONLY           15      // 2-players only inside
#define R_THREE_PEOPLE_ONLY         16      // 3-players only inside
#define R_NO_MAGIC                  17      // No magic allowed in room
#define R_PERMENANT_TRACKS          18      // Permanent tracks in room
#define R_EARTH_BONUS               19      // Earth realm
#define R_AIR_BONUS                 20      // Wind realm
#define R_FIRE_BONUS                21      // Fire realm
#define R_WATER_BONUS               22      // Water realm
#define R_PLAYER_DEPENDENT_WANDER   23      // Player-dependent monster wanders
#define R_PLAYER_HARM               24      // player harming room
#define R_POISONS                   25      // player poison room
#define R_DRAIN_MANA                26      // player mp drain room
#define R_STUN                      27      // player befuddle room
#define R_NO_SUMMON_OUT             28      // player can not be summon out
#define R_PLEDGE                    29      // player can pledge in room
#define R_RESCIND                   30      // player can rescind in room
#define R_NO_POTION                 31      // No potion room
#define R_MAGIC_BONUS               32      // Player magic spell extend
#define R_NO_LOGIN                  33      // No player login
#define R_VAMPIRE_COVEN             34      // room is a vampire coven
#define R_INDOORS                   35      // Indoors
#define R_JAIL                      36      // Room is a Jail
#define R_NO_CAST_TELEPORT          37      // You cannot cast teleport in this room
#define R_LOTTERY_OFFICE            38      // You buy lottery tickets here, also set shop flag
#define R_DESERT_HARM               39      // Desert room harm flag
#define R_STORAGE_ROOM_SALE         40      // Can buy storage rooms here
#define R_IS_STORAGE_ROOM           41      // Room is a storage room
#define R_NO_CLAIR_ROOM             42      // Cannot clair this room
#define R_NO_FLEE                   43      // Cannot flee from this room
#define R_DEADLY_VINES              44      // Player harm - Vines
#define R_CAN_SHOPLIFT              45      // Can shoplift here
#define R_FOREST_OR_JUNGLE          46      // Room is in a forest/jungle
#define R_ETHEREAL_PLANE            47      // Room is in the ethereal plane
#define R_OPPOSITE_REALM_BONUS      48      // The opposite realm of the room is given a bonus
#define R_PAWN_SHOP                 49      // Pawn Shoppe
#define R_ICY_WATER                 50      // Ice cold water
#define R_NO_TRACK_TO               51      // Cannot track to this room.
#define R_DESTROYS_ITEMS            52      // Environment destroys dropped items.
#define R_NO_SUMMON_TO              53      // Players cannot be summoned to this room
#define R_NO_TRACK_OUT              54      // Player cannot track out of this room
#define R_ROOM_REALM_BONUS          55      // Room's realm adds to same realms spell damage
#define R_OUTLAW_SAFE               56      // Cannot kill an outlaw in this room
#define R_BANK                      57      // Room is a bank
#define R_MAGIC_MONEY_MACHINE       58      // Magic Money Machine Room - can get balance and money from
#define R_CASINO                    59      // Room is a casino - lets you gamble
#define R_GUILD_OPEN_ACCESS         60      // Guild: Open Access
#define R_NO_WORD_OF_RECALL         61      // Word of recall does not work in this room
#define R_LOG_INTO_TRAP_ROOM        62      // Players log in to room's trap exit
#define R_DIFFICULT_TO_MOVE         63      // Difficult to move in this room
#define R_ELEC_BONUS                64      // Electromagnetic room
#define R_COLD_BONUS                65      // Cold realm room
#define R_DIFFICULT_FLEE            66      // Difficult to flee from here
#define R_NO_MIST                   67      // Mist cannot enter here
#define R_DISPERSE_MIST             68      // Room disperses mist
#define R_DECLARE_BOUNTY            69      // can declare a bounty here
#define R_BUILD_SHOP                70      // can build a shop here
#define R_MAGIC_DARKNESS            71      // Room cloaked in magical darkness
#define R_UNDER_WATER_BONUS         72      // Room is underwater
#define R_NO_DODGE                  73      // No dodge/riposte allowed in room
#define R_ARAMON                    74      // Aramon faith room
#define R_CERIS                     75      // Ceris faith room
#define R_ENOCH                     76      // Enoch faith room
#define R_GRADIUS                   77      // Gradius faith room
#define R_ARES                      78      // Ares faith room
#define R_KAMIRA                    79      // Kamira faith room
#define R_JAKAR                     80      // Jakar faith room
#define R_LINOTHAN                  81      // Linothan faith room
#define R_MARA                      82      // Mara faith room
#define R_ARACHNUS                  83      // Arachnus faith room
#define R_UNDERGROUND               84      // Underground room
#define R_BUILD_GUILDHALL           85      // can build a guildhall here
#define R_WAS_BUILD_GUILDHALL       86      // used to be able to build a guildhall
#define R_ARCHERS                   87      // elven archers
#define R_DEADLY_MOSS               88      // deadly moss
#define R_WAS_BUILD_SHOP            89      // used to be able to build a shop
#define R_CONSTRUCTION              90      // This room is under construction
#define R_TYPO                      91      // A typo was reported in this room
#define R_BUILD_HOUSE               92      // can build a house here
#define R_WAS_BUILD_HOUSE           93      // used to be able to build a house
#define R_WINTER_COLD               94      // room is cold during the winter
#define R_NO_PREVIOUS               95      // player does not record this as previous room
#define R_SHOP_STORAGE              96      // must be set in storage room for room flag 1 (Shoppe) to work
#define R_ALWAYS_WINTER             97      // Treat this room as winter regardless of season
#define R_CAN_ALWAYS_MIST           98      // Player may always turn to mist
#define R_LIMBO                     99      // room is a Limbo room
#define R_NO_SPRING_TRAP_ON_ENTER   100      // Do not spring trap when player enters the room
#define MAX_ROOM_FLAGS              101      // Incriment when you add a room flag
// Player flags
// free                             0
#define P_HIDDEN                    1        // Hidden
#define P_HARDCORE                  2        // A hardcore character: death is permanent.
#define P_NO_BROADCASTS             3        // Don't show broadcasts
// free                             4
// free                             5
#define P_IGNORE_GOSSIP             6        // ignore gossip channel
#define P_IGNORE_CLAN               7        // ignore clan channel
#define P_PORTAL                    8        // portal
#define P_NO_AUTO_ATTACK            9        // no auto attack for players
#define P_DM_INVIS                  10       // DM Invisibility
#define P_SLEEPING                  11       // sleeping
#define P_MALE                      12       // Sex == male
#define P_SEXLESS                   13       // sexless
#define P_WIMPY                     14       // Wimpy mode
#define P_EAVESDROPPER              15       // Eavesdropping mode
#define P_AUTO_INVIS                16       // Automatically turn on DM Invis if idle for over 100 minutes
#define P_UNIQUE_TO_DECAY           17       // Player has a unique that must decay
#define P_PROMPT                    18       // Display status prompt
#define P_FRENZY_OLD                19       // Frenzy Flag (For Werewolfs)
#define P_MXP_ENABLED               20       // Player wants to use MXP
#define P_MXP_ACTIVE                21       // MXP is active! we need to close it
#define P_PRAYED_OLD                22       // Prayed
#define P_INVERT_AREA_COLOR         23       // Invert area color
#define P_PREPARED                  24       // Prepared for trap
#define P_T_TO_BOUND                25       // *t takes you to bound room
#define P_ANSI_COLOR                26       // Ansi Color
#define P_SPYING                    27       // Spying on someone
#define P_CHAOTIC                   28       // Chaotic/!Lawful
#define P_READING_FILE              29       // Reading a file
#define P_NO_TICK_MSG               30       // Ignore mob tick broadcasts
#define P_NO_AGGRO_MSG              31       // Ignore mob aggro broadcasts
#define P_NO_DEATH_MSG              32       // Ignore mob death broadcasts
#define P_CAN_CHOOSE_CUSTOM_TITLE   33       // player can choose custom title
#define P_NO_SUMMON                 34       // Nosummon flag
#define P_IGNORE_ALL                35       // Ignore all send
#define P_IGNORE_SMS                36       // Ignore text messages
#define P_NO_TRACK_STATS            37       // Do not track statistics
#define P_DARKNESS                  38       // Player is carrying a darkness item
#define P_PLEDGED                   39       // Player is pledged
#define P_MIRC                      40       // Player is using mirc (colors/stuff)
#define P_SEE_HOOKS                 41       // Whether or not to show this DM successful hooks as they happen
#define P_SHOW_SKILL_PROGRESS       42       // Show skill Progress
#define P_CANNOT_CHOOSE_CUSTOM_TITLE 43      // player has lost the privelege to choose custom titles
#define P_SEE_ALL_HOOKS             44       // Whether or not to show this DM all hooks as they happen
#define P_CHARMED                   45       // Player is charmed
#define P_NO_LOGIN_MESSAGES         46       // No login messages sent
#define P_VIEW_ZONES                47       // Player wants to view zones
#define P_SECURITY_CHECK_OK         48       // Player has passed security check
#define P_AUTHERIZED                49       // No-port that has been authorized
#define P_ALIASING                  50       // DM is aliasing
// free                             51
#define P_NEWLINE_AFTER_PROMPT      52       // Print NL after prompt
#define P_BERSERKED_OLD             53       // Player is berserked
#define P_LOG_WATCH                 54       // DM is watching the log channel
// free                             55
#define P_NO_AUTO_TARGET            56       // Don't Automatically target anything you attack if you don't already have a target
// free                             56       // UNUSED
#define P_IGNORE_CLASS_SEND         57       // Player is ignoring class sends
#define P_IGNORE_GROUP_BROADCAST    58       // Player is ignoring group broadcasts
#define P_ENCHANT_ONLY              59       // Player resists-hands
#define P_JUST_STAT_MOD             60       // Player just used a stat-modifying item
#define P_RESIST_STUN               61       // Player resists-stun
#define P_SHOW_ALL_PREFS            62       // Show all preferences by default
#define P_COMPACT                   63       // show compact output
#define P_SHOW_TICK                 64       // Show ticks
#define P_JUST_REFUNDED             65       // No haggling until they leave the room!
// free                             66       // +PTY
// free                             67
// free                             68
// free                             69
#define P_HIDE_FORUM_POSTS          70       // Player does not want to see forum post notifications
#define P_FULL_FORUM_POSTS          71       // Player wants to see first few lines of forum posts
#define P_OUTLAW                    72       // Player is an outlaw
#define P_JUST_TRAINED              73       // Player just trained
#define P_NO_COMPUTE                74       // DM flag only: don't compute ac
#define P_SNEAK_WHILE_MISTED        75       // sneak while misted
#define P_FOCUSED                   76       // player is focused
#define P_STUNNED                   77       // player currently stunned
// free                             78       // unused
#define P_NO_AUTO_WEAR              79       // Player won't wear all when they log on
// free                             80
// free                             81
// free                             82
#define P_NO_FOLLOW                 83       // Player cannot be followed
#define P_CAUGHT_SHOPLIFTING        84       // Player was caught shoplifting
#define P_LAG_PROTECTION_SET        85       // Lag protection set (auto-hazy / auto-disconnect)
#define P_LAG_PROTECTION_OPERATING  86       // Lag detection routines currently in operation
#define P_LAG_PROTECTION_ACTIVE     87       // Check for lag protection activated
#define P_DM_SILENCED               88       // Player silenced by DM
#define P_DM_BLINDED                89       // Player blinded by DM
#define P_NO_MSG                    90       // DM/CT will not see *msg channel
#define P_CAN_AWARD                 91       // DM only. Allows CTs to award roleplaying xp if set.
#define P_FREE_TRAIN                92       // Player doesn't need cash to level. For ptesting
#define P_GOLD_SPLIT                93       // Player has auto gold split turned on.
// free                             94
#define P_PTESTER                   95       // Player is a ptester
#define P_NO_SHOW_STATS             96       // HP/MP will not show when followed
#define P_NO_CAST_SUMMON            97       // Player cannot cast summon - for summon slaves
#define P_NO_GET_ALL                98       // Player cannot get all - for people abusing it.
#define P_CAN_OUTLAW                99       // DM only. Allows a CT to use *outlaw command.
#define P_OUTLAW_WILL_BE_ATTACKED   100      // Outlaw will be attacked by outlaw aggros
#define P_OUTLAW_WILL_LOSE_XP       101      // Outlaw will lose experience when killed
#define P_LINKDEAD                  102      // Player is linkdead (shouldn't be considered 2x-logging)
#define P_UNCONSCIOUS               103      // Player is unconscious
// free                             104
#define P_BRAINDEAD                 105      // Player is brain dead -- For jackasses...
// free                             106
#define P_CT_CAN_DM_LIST            107      // DM only. Caretaker can use *list command
// free                             108
// free                             109
#define P_CT_CAN_DM_BROAD           110      // DM only. Caretaker can use *broad
#define P_SHOW_XP_IN_PROMPT         111      // XP shows in player prompt
#define P_UNREAD_MAIL               112      // Player has unread mudmail
#define P_NO_SHOW_MAIL              113      // Player will not see new mail notification
#define P_POISONED_BY_PLAYER        114      // Poisoned by player
#define P_POISONED_BY_MONSTER       115      // Poisoned by monster
#define P_NO_LAG_HAZY               116      // Hazy looked for 2nd in lag protection
#define P_NO_EXTRA_COLOR            117      // Extra color options
#define P_NO_PKILL_PERCENT          118      // Pkill percent not shown
#define P_NO_AUCTIONS               119      // Will not see auction broadcasts
#define P_KILLS_LOGGED              120      // All player's kills logged
#define P_SUPER_EAVESDROPPER        121      // Super eavesdrop mode
#define P_CAN_FLASH_SELF            122      // Player is allowed to flash self
#define P_SAVING_THROWSPELL_LEARN   123      // Saving throw values defined. DO NOT UNSET THIS FLAG!
// free                             124
#define P_IGNORE_RACE_SEND          125      // Player ignoring race broadsend
#define P_AFK                       126      // Player is away from keyboard (afk)
// free                             127
#define P_GLOBAL_GAG                128      // Player globally gagged
#define P_DONT_SHOW_SHOP_PROFITS    129      // Don't show profits
// free                             130
// free                             131
#define P_NO_TELLS                  132      // Player ignores send/tell messages
#define P_LANG_ALIEN                133      // Player speaks alien language (unknown)
#define P_LANG_DWARVEN              134      // Player speaks dwarven
#define P_LANG_ELVEN                135      // Player speaks elven
#define P_LANG_HALFLING             136      // Player speaks halfling
#define P_LANG_COMMON               137      // Player speaks common
#define P_LANG_ORCISH               138      // Player speaks orcish
#define P_LANG_GIANTKIN             139      // Player speaks giantkin
#define P_LANG_GNOMISH              140      // Player speaks gnomish
#define P_LANG_TROLLISH             141      // Player speaks language of trolls
#define P_LANG_OGRISH               142      // Player speaks ogrish
#define P_LANG_DARK_ELVEN           143      // Player speaks dark elven
#define P_LANG_GOBLINOID            144      // Player speaks goblinoid
#define P_LANG_MINOTAUR             145      // Player speaks language of minotaurs
#define P_LANG_CELESTIAL            146      // Player speaks celestial (seraph)
#define P_LANG_KOBOLD               147      // Player speaks kobold
#define P_LANG_INFERNAL             148      // Player speaks infernal (cambion)
#define P_LANG_BARBARIAN            149      // Player speaks barbarian
#define P_LANG_KATARAN              150      // Player speaks kataran
// free                             151
// free                             152
// free                             153
#define P_LANGUAGE_COLORS           154      // Player sees language colors
#define P_LANGUAGE_INITIALIZED      155      // Player language been initialized
// free                             156
#define P_ON_PROXY                  157      // Player is allowed to multi-log
#define P_SECRET_WATCHER            158      // Player does not show up as a watcher
#define P_GROUP_EXP_ABUSE           159      // No group exp due to abuse
#define P_WATCHER                   160      // Player is a watcher
#define P_CAN_MUDMAIL_STAFF         161      // Player can mudmail CTs and DMs
#define P_NO_DUEL_MESSAGES          162      // Player will not see duel death messages
#define P_BUGGED                    163      // Player is bugged for observation
#define P_DIED_IN_DUEL              164      // Player just died in a duel - DO NOT SET
#define P_MISTBANE                  165      // Player (ranger) can hit mist (mistbane)
#define P_XP_DIVIDE                 166      // Player has xp split turned on
#define P_NO_PKILL                  167      // Player cannot be pkilled right now
#define P_PERM_DEATH                168      // Player sees perm deaths
// free                             169
// free                             170
#define P_IGNORE_NEWBIE_SEND        171      // Player ignoring newbie channel
// free                             172
// free                             173
// free                             174
#define P_SITTING                   175      // Player is sitting down resting
#define P_NO_WTS                    176      // Cannot use *wts command
#define P_CREATING_GUILD            177      // Player is currently attempting to create a new guild
// free                             178
// free                             179
#define P_FREE_ACTION               180      // Player under free-action spell
#define P_NO_FLUSHCRTOBJ            181      // Builder doesn't have to *flushcrtobj
#define P_NO_NUMBERS                182      // No Mob Numbers
#define P_CAN_REBOOT                183      // Can use *reboot command
#define P_CAN_CHANGE_STATS          184      // Can change stats
#define P_JAILED                    185      // Jailed by *jail command. DO NOT SET
// free                             186
#define P_KILL_AGGROS               187      // Player attacks aggros first
#define P_PASSWORD_CURRENT          188      // Password is current
#define P_NO_TICK_MP                189      // Player cannot tick MP
#define P_NO_TICK_HP                190      // Player cannot tick HP
// free                         191
#define P_KILLED_BY_MOB             192      // Just killed by mob
// free                             193
#define P_DOCTOR_KILLER             194      // Player is a doctor killer
#define P_CHOSEN_SURNAME            195      // Player has chosen surname
#define P_NO_SURNAME                196      // Player cannot use surname command
// free                             197
#define P_ADULT                     198      // Player can use adult channel
#define P_VETERAN                   199      // Player can use veteran channel
#define P_CANT_BROADCAST            200      // Player cannot broadcast
#define P_CHOSEN_ALIGNMENT          201      // Player has chosen alignment
#define P_NO_SUICIDE                202      // Player cannot suicide - cool off period
#define P_BUILDER_MOBS              203      // Builder can make mobs
#define P_BUILDER_OBJS              204      // Builder can make objs
#define P_DARKMETAL                 205      // Player has a darkmetal item (DONT SET)
// free                             206
// free                             207
// free                             208
// free                             209
// free                             210
// free                             211
// free                             212
// free                             213
// free                             214
// free                             215
// free                             216
// free                             217
#define MAX_PLAYER_FLAGS            218     // Incriment when u add a new player flag
// Monster flags
#define M_PERMENANT_MONSTER         0       // Permanent monster
#define M_HIDDEN                    1       // Hidden
#define M_WAS_HIDDEN                2       // Monster was hidden before and will rehide again
#define M_FACTION_NO_GUARD          3       // If the player is in good standing with the monster, the monster will not passively guard exits
// free                             4
#define M_NO_PREFIX                 5       // No prefix
#define M_AGGRESSIVE                6       // Aggressive
#define M_GUARD_TREATURE            7       // Guards treasure
#define M_BLOCK_EXIT                8       // Blocks exits
#define M_FOLLOW_ATTACKER           9       // Monster follows attacker
#define M_WILL_FLEE                 10      // Monster flees
#define M_SCAVANGER                 11      // Monster is a scavenger
#define M_MALE                      12      // Sex == male
#define M_WILL_POISON               13      // Poisoner
#define M_UNDEAD                    14      // Undead
#define M_CANT_BE_STOLEN_FROM       15      // Cannot be stolen from
#define M_NO_HARM_SPELL             16      // Immune to Harm spell
#define M_CAN_CAST                  17      // Can cast spells
#define M_HAS_SCAVANGED             18      // Has already scavenged something
// free                             19
#define M_ONLY_HARMED_BY_MAGIC      20      // Can only be harmed by magic
#define M_FACTION_ASSIST            21      // monster will assist members of primeFaction
#define M_ENCHANTED_WEAPONS_ONLY    22      // Can only be harmed by magic/ench.weapon
#define M_TALKS                     23      // Monster can talk interactively
#define M_UNKILLABLE                24      // Monster cannot be harmed
#define M_NO_RANDOM_GOLD            25      // Monster has fixed amt of gold
#define M_AGGRESSIVE_AFTER_TALK     26      // Becomes aggressive after talking
#define M_NO_FACTION_AGGRO          27      // Don't aggro based on faction
#define M_CAN_RIPOSTE               28      // Can riposte regardless of other rules
#define M_DARKNESS                  29      // Monster is carrying a darkness item
// free                             30
// free                             31
#define M_CAN_PLEDGE_TO             32      // players can pledge to monster
#define M_CAN_RESCIND_TO            33      // players can rescind to monster
#define M_DISEASES                  34      // Monster causes disease
#define M_DISOLVES_ITEMS            35      // Monster can dissolve items
#define M_CAN_PURCHASE_FROM         36      // player can purchase from monster
#define M_TRADES                    37      // monster will give items
#define M_PASSIVE_EXIT_GUARD        38      // passive exit guard
#define M_AGGRESSIVE_GOOD           39      // Monster aggro to good players
#define M_AGGRESSIVE_EVIL           40      // Monster aggro to evil players
#define M_DEATH_SCENE               41      // Monster has additon desc after death
#define M_CAST_PRECENT              42      // Monster cast magic percent flag (prof 1)
#define M_RESIST_STUN_SPELL         43      // Monster resists stun only
#define M_NO_CIRCLE                 44      // Monster cannot be circled
#define M_WILL_BLIND                45      // Monster blinds
#define M_DM_FOLLOW                 46      // Monster will follow DM
// free                             47
// free                             48
// free                             49
#define M_CHARMED                   50      // Monster is charmed
#define M_MOBILE_MONSTER            51      // Monster will wander around the area
#define M_LOGIC_MONSTER             52      // Logic Monster
#define M_TAKE_LOOT                 53      // Monster will try to take players' loot
#define M_FAST_WANDER               54      // Monster will quickly wander around the area
#define M_PET                       55      // Monster is a pet
// free                             56
// free                             57
// free                             58
// free                             59
// free                             60
// free                             61
#define M_SEXLESS                   62      // sexless
// free                             63
// free                             64
// free                             65
// free                             66
// free                             67
// free                             68
// free                             69
// free                             70
#define M_NO_WANDER                 71      // monster doesn't wander
// free                             72
// free                             73
// free                             74
// free                             75
// free                             76
// free                             77
// free                             78
// free                             79
// free                             80
// free                             81
// free                             82
// free                             83
// free                             84
// free                             85
// free                             86
// free                             87
// free                             88
// free                             89
#define M_REGENERATES               90      // monster regenerates
#define M_PLUS_TWO                  91      // monster needs +2 weapon or above to be hit
#define M_PLUS_THREE                92      // monster needs +3 weapon or above to be hit
// free                             93
#define M_STEAL_WHEN_ATTACKING      94      // monster steals if attacked
#define M_STEAL_ALWAYS              95      // monster steals on sight
// free                             96
// free                             97
// free                             98
#define M_WILL_BE_ASSISTED          99      // Will be assisted
#define M_WILL_ASSIST               100      // Will assist
#define M_NIGHT_ONLY                101      // Only comes at night
#define M_OUTLAW_AGGRO              102      // Mob aggros outlaws
// free                             103
// free                             104
// free                             105
#define M_WILL_WIELD                106      // Monster will wield weapons in their inventory
//#define M_WILL_MOVE_FOR_CASH      107      // monster will move out of the way (clear pguard for a few) when given enough money
// free                             108
#define M_DAY_ONLY                  109      // monster will only come during the day
#define M_NO_SHOW_ARRIVE            110      // Don't show when monster arrives
// free                             111
// free                             112
#define M_UN_DODGEABLE              113      // Monster cannot be dodged
// free                             114
// free                             115
#define M_ATTACKING_SHOPLIFTER      116      // Monster is attacking a shoplifter
#define M_CHASING_SOMEONE           117      // Monster chasing someone
#define M_RESIST_CIRCLE             118      // Monster resists circle - circled as if undead
#define M_WILL_YELL_FOR_HELP        119      // Monster will yell for help
#define M_YELLED_FOR_HELP           120      // Monster yelled for help - DONT SET
#define M_WILL_BE_HELPED            121      // Monster doesn't actually yell for help, but mobs arrive to help
#define M_BENEVOLENT_SPELLCASTER    122      // Monster is a benevolent spellcaster
#define M_WILL_BE_LOGGED            123      // Anything that happens to the monster will be logged until saved
// free                             124
// free                             125
#define M_LEVEL_BASED_STUN          126      // Level-based stunnable
#define M_RESIST_TOUCH              127      // Monster resists touch-of-death
#define M_GREEDY                    128      // Monster will sometimes steal cash instead of killing
#define M_POLICE                    129      // Monster will beat the person up and then throw em in jail
#define M_LEVEL_RESTRICTED          130      // Monster can only be killed/attacked by someone less than maxLevel
#define M_NO_BACKSTAB               131      // Monster cannot be backstabbed - for really small creatures and stuff like slimes
#define M_IGNORE_ROOM_DESTROY       132      // Monster drops ignore room destroy flag
// free                             133
// free                             134
// free                             135
// free                             136
// free                             137
// free                             138
// free                             139
// free                             140
// free                             141
// free                             142
// free                             143
// free                             144
// free                             145
// free                             146
// free                             147
// free                             148
// free                             149
// free                             150
// free                             151
// free                             152
// free                             153
// free                             154
// free                             155
#define M_NO_LEVEL_ONE              156      // Monster uneffected by level 1 spells
#define M_NO_LEVEL_TWO              157      // Monster uneffected by level 2 spells or less
#define M_NO_LEVEL_THREE            158      // Monster uneffected by level 3 spells or less
#define M_NO_LEVEL_FOUR             159      // Monster uneffected by level 4 spells or less
#define M_NO_KICK                   160      // Monster cannot be kicked (for things like slimes)
#define M_DEATH_SCREAM              161      // Monster screams like a banshee - death scream
#define M_WILL_SNEAK                162      // Monster will sneak around
#define M_SNEAKING                  163      // Monster is sneaking
// free                             164
#define M_WILL_BE_AGGRESSIVE        165      // Monster will randomly become aggressive
#define M_NO_POISON                 166      // Monster cannot be poisoned
#define M_NO_CHARM                  167      // Monster cannot be charmed
#define M_SPECIAL_UNDEAD            168      // Monster turned as 'special' undead -- harder to turn
// free                             169
// free                             170
// free                             171
#define M_NO_EXP_LOSS               172      // Monster does not make player lose exp when they die
// free                             173
// free                             174
// free                             175
// free                             176
// free                             177
// free                             178
#define M_CAN_STONE                 179      // Monster's touch can turn to stone
// free                             180
#define M_ALWAYS_AGGRESSIVE         181      // Monster ALWAYS aggressive, ignoring other rules that can override aggro
#define M_KILL_PERMS                182      // Monster will attack perm creatures
#define M_KILL_NON_ASSIST_MOBS      183      // Monster attacks all mobs it will not assist
#define M_KILL_MASTER_NOT_PET       184      // Monster kills master - not pet
// free                             185
#define M_FAST_TICK                 186      // Monster fast ticks when no one in room
#define M_RESTRICT_EXP_LEVEL        187      // Lower exp after max-killable level
// free                             188
// free                             189
#define M_TOLLKEEPER                190      // Monster is a tollkeeper
#define M_ALIGNMENT_VARIES          191      // Monster's alignment will vary
#define M_ASSIST_HIT_CHANCE         192      // Monsters assist each others' chance to hit
#define M_STREET_SWEEPER            193      // Monster is a streetsweeper - cleans rooms (will pick up a lot of objects)
#define M_CUSTOM                    194      // Monster is custom - has no specific mtype
#define M_NO_ADJUST                 195      // Monster will not adjust on loading
// free                             196
// free                             197
// free                             198
// free                             199
#define M_ALWAYS_ACTIVE             200      // Monster will remain on active list until killed
#define M_NO_AUTO_CRIT              201      // Monster will not be auto-critted
// free                             202
#define M_ANTI_MAGIC_AURA           203      // Monster is anti-magic - blocks magic in room
#define M_MAGE_LICH_STUN_ONLY       204      // Monster is mage/lich stun only
#define M_FIRE_AURA                 205      // Monster has fire aura attack
#define M_COLD_AURA                 206      // Monster has cold aura attack
#define M_NAUSEATING_AURA           207      // Monster has a nauseating aura
#define M_NEGATIVE_LIFE_AURA        208      // Monster has a negative life-draining aura
#define M_TURBULENT_WIND_AURA       209      // Monster has a turbulent wind aura
// free                             210
// free                             211
// free                             212
#define M_WOUNDING                  213      // Monster has wounding attack
// free                             214
#define M_WILL_BERSERK              215      // Monster will berserk
#define M_WAS_PORTED                216      // Monster was teleported (DO NOT SET)
// free                             217
// free                             218
// free                             219
#define M_RACE_AGGRO_INVERT         220      // Race aggro invert flag
#define M_CLASS_AGGRO_INVERT        221      // Class aggro invert flag
// free                             222
#define M_SAVE_FULL                 223      // Save the full mob to a roomfile instead of just a reference
#define M_DEITY_AGGRO_INVERT        224      // Deity aggro invert flag
// free                             225
// free                             226
// free                             227
// free                             228
// free                             229
#define M_RAIDING                   230      // Monster is raiding (DO NOT SET)
// free                             231
// free                             232
// free                             233
// free                             234
// free                             235
// free                             236

#define MAX_MONSTER_FLAGS           237      // Incriment when adding (max is 256)

// Object flags
#define O_PERM_ITEM                 0      // Permanent item (not yet taken)
#define O_HIDDEN                    1      // Hidden
// free                             2
#define O_OLD_INVISIBLE             2      // Invisible
#define O_SOME_PREFIX               3      // "some" prefix
#define O_PUSH_PULL_SPRINGS_TRAP    4      // Pushing or pulling this object springs the room's trap
#define O_NO_PREFIX                 5      // No prefix
#define O_FISHING                   6      // Can be used to fish
#define O_WEIGHTLESS_CONTAINER      7      // Container of weightless holding
#define O_TEMP_PERM                 8      // Temporarily permanent
#define O_PERM_INV_ITEM             9      // Permanent INVENTORY item
#define O_NO_MAGE                   10      // Mages cannot wear/use it
#define O_LIGHT_SOURCE              11      // Object serves as a light
#define O_GOOD_ALIGN_ONLY           12      // Usable only by good players
#define O_EVIL_ALIGN_ONLY           13      // Usable only by evil players
#define O_NO_ENCHANT                14      // Object cannot be enchanted
#define O_NO_FIX                    15      // Cannot be repaired
#define O_CLIMBING_GEAR             16      // Climbing gear
#define O_NO_TAKE                   17      // Cannot be taken
#define O_SCENERY                   18      // Part of room description/scenery
#define O_DARKNESS                  19      // Object creates magical darkness
#define O_HOT                       20      // Object is hot (can be used in cooking)
#define O_RANDOM_ENCHANT            21      // Random enchantment flag
#define O_CURSED                    22      // Object is cursed
#define O_WORN                      23      // Object is being worn
#define O_CAN_USE_FROM_FLOOR        24      // Can be used from the floor
#define O_DEVOURS_ITEMS             25      // Container devours items
#define O_FEMALE_ONLY               26      // Usable by only females
#define O_MALE_ONLY                 27      // Usable by only males
#define O_SEXLESS_ONLY              28      // Usable by only sexless creatures
#define O_PLEDGED_ONLY              29      // Usable by only pledged players
#define O_MAGIC                     30      // object is magical (displays (M) with detect-magic)
#define O_CSEL_INVERT               31      // invert class selective rules
#define O_SEL_ASSASSIN              32      // class selective: assassin
#define O_SEL_BERSERKER             33      // class selective: barbarian
#define O_SEL_CLERIC                34       // class selective: cleric
#define O_SEL_FIGHTER               35      // class selective: fighter
#define O_SEL_MAGE                  36      // class selective: mage
#define O_SEL_PALADIN               37      // class selective: paladin
#define O_SEL_RANGER                38      // class selective: ranger
#define O_SEL_THIEF                 39      // class selective: thief
#define O_SEL_VAMPIRE               40      // class selective: vamp
#define O_SEL_MONK                  41      // class selective: monk
#define O_SEL_DEATHKNIGHT           42      // class selective: death knight
#define O_SEL_DRUID                 43      // class selective: druid
#define O_SEL_LICH                  44      // class selective: lich
#define O_SEL_WEREWOLF              45      // class selective: werewolf
#define O_SEL_BARD                  46      // class selective: bard
#define O_SEL_ROGUE                 47      // class selective: rogue
#define O_DISPOSABLE                48      // Object will not keep area room in memory
#define O_NEVER_SHATTER             49      // Weapon will never shatter
#define O_ALWAYS_CRITICAL           50      // Weapon will always critical
#define O_LUCKY                     51      // Object is a luck charm
#define O_WEAPON_CASTS              52      // Weapon casts spells on hit (realm-offensive only)
#define O_TWO_HANDED                53      // Weapon requires two hands to wield (no double wield)
#define O_EQUIPPING_BESTOWS_EFFECT  54      // Equiping this object bestows an effect
#define O_JUST_LOADED               55      // Object was just loaded on a monster and hasn't been obtained
                                            // by a player yet so exp can be gained from stealing it
#define O_CHAOTIC_ONLY              56      // Chaotics Only
#define O_LAWFUL_ONLY               57      // Lawfuls Only
#define O_TEMP_ENCHANT              58      // Temp Enchant
#define O_STARTING                  59      // Starting Item
// free                             60
#define O_CLAN_1                    61      // Clan 1 can use
#define O_CLAN_2                    62      // Clan 2 can use
#define O_CLAN_3                    63      // Clan 3 can use
#define O_CLAN_4                    64      // Clan 4 can use
#define O_CLAN_5                    65      // Clan 5 can use
#define O_CLAN_6                    66      // Clan 6 can use
#define O_CLAN_7                    67      // Clan 7 can use
#define O_CLAN_8                    68      // Clan 8 can use
#define O_CLAN_9                    69      // Clan 9 can use
#define O_CLAN_10                   70      // Clan 10 can use
// free                             71
// free                             72
// free                             73
// free                             74
// free                             75
// free                             76
// free                             77
// free                             78
// free                             79
#define O_LOAD_ALL                  80      // When trading, load all RandomObjects, rather than a random one.
#define O_RESIST_DISOLVE            81      // Object resists dissolve
#define O_RECLAIMED                 82      // Object has been reclaimed at least once, no futher haggling allowed
// free                             83
// free                             84
// free                             85
// free                             86
// free                             87
// free                             88
// free                             89
// free                             90
#define O_DRINKABLE                 91      // You can drink the object
#define O_EATABLE                   92      // You can eat the object
#define O_ALWAYS_DROPPED            93      // Monsters always drop this object
#define O_CONSUME_HEAL              94      // Consumable object that will heal the player
// free                             95
// free                             96
// free                             97
// free                             98
#define O_CLAN_11                   99      // Clan 11 can use
#define O_CLAN_12                   100      // Clan 12 can use
// free                             101
// free                             102
// free                             103
#define O_CUSTOM_OBJ                104      // Object is a custom
#define O_NO_SHOPLIFT               105      // Object cannot be shoplifted
#define O_WAS_SHOPLIFTED            106      // Object was shoplifted (so people can't sell shoplifted items)
// free                             107
#define O_CAN_HIT_MIST              108      // Object can hit misted vampire
#define O_SILVER_OBJECT             109      // Object made of silver - extra damage to werewolf
#define O_HOLD_BONUS                110      // Object confers a bonus to hit if held
// free                             111
#define O_NO_STEAL                  112      // Object cannot be stolen
#define O_SMALL_SHIELD              113      // Object is a small shield
// free                             114
// free                             115
// free                             116
// free                             117
#define O_NO_PAWN                   118      // Object not pawnable
#define O_BREAK_ON_DROP             119      // Object breaks when drops
#define O_LORE                      120      // Lore Item
#define O_RSEL_INVERT               121      // invert race-selective rules
#define O_SEL_DWARF                 122      // race selective: dwarf
#define O_SEL_ELF                   123      // race selective: elf
#define O_SEL_HALFELF               124      // race selective: half-elf
#define O_SEL_HALFLING              125      // race selective: halfling
#define O_SEL_HUMAN                 126      // race selective: human
#define O_SEL_ORC                   127      // race selective: orc
#define O_SEL_HALFGIANT             128      // race selective: half-giant
#define O_SEL_GNOME                 129      // race selective: gnome
#define O_SEL_TROLL                 130      // race selective: troll
#define O_SEL_HALFORC               131      // race selective: half-orc
#define O_SEL_OGRE                  132      // race selective: ogre
#define O_SEL_DARKELF               133      // race selective: dark-elf
#define O_SEL_GOBLIN                134      // race selective: goblin
#define O_SEL_MINOTAUR              135      // race selective: minotaur
#define O_SEL_SERAPH                136      // race selective: seraph
#define O_SEL_KOBOLD                137      // race selective: kobold
#define O_SEL_CAMBION               138      // race selective: cambion
#define O_SEL_BARBARIAN             139      // race selective: barbarian
#define O_SEL_KATARAN               140      // race selective: kataran
#define O_ENVENOMED                 141      // Object has been envenomed
#define O_NULL_MAGIC                142      // Object does not reveal that it is magical to detect-magic
#define O_NO_BREAK                  143      // Object does not break
#define O_ALLOW_TOUCH_OF_DEATH      144      // Monks can use touch-of-death with this weapon wielded
#define O_JUST_BOUGHT               145      // Object was just bought and is refundable
#define O_NO_DROP                   146      // Can't drop or otherwise get rid of this object
#define O_UNPAGED_FILE                147      // Display file all at once
#define O_BROKEN_BY_CMD             148      // Object is broken by break command
#define O_SMALL_BOW                 149      // Objext is a small bow
#define O_COIN_OPERATED_OBJECT      150      // Coin operated object
#define O_BEING_PREPARED            151      // Object is being prepared
#define O_MISSLE_USE_STRENGTH       152      // Missile weapon uses strength bonus
#define O_UNIQUE                    153      // Object is unique or limited
#define O_BULKLESS_CONTAINER        154      // Bulkless container object
#define O_BULKLESS_OBJECT           155      // Bulkless object
// free                             156
#define O_NOT_PEEKABLE              157      // Object can not be peeked at
#define O_ENHANCE_BASH              158      // Object does extra bash damage equal to dice
#define O_KEEP                      159      // Object is being kept - cannot be dropped or sole on accident
#define O_NO_BACKSTAB               160      // Cannot backstab with this weapon
#define O_SAVE_FULL                 161      // Save the entire object to pfile/roomfile instead of just a reference.
// Mainly used when a ct/dm has changed an item, cleared when *saving
// or for custom
#define O_BODYPART                  162      // Object cannot be peeked/stolen, unset when monster dies
#define O_DARKMETAL                 163      // Object is a darkmetal item (destroyed in sunlight)
#define O_SEL_TIEFLING              164      // race selective: tiefling
#define MAX_OBJECT_FLAGS            165      // Incriment when you add a new Object flag

// Exit flags
#define X_SECRET                    0         // Secret
// Free                             1
#define X_LOCKED                    2         // Locked
#define X_CLOSED                    3         // Closed
#define X_LOCKABLE                  4         // Lockable
#define X_CLOSABLE                  5         // Closable
#define X_UNPICKABLE                6         // Un-pickable lock
#define X_NAKED                     7         // Naked exit
#define X_NEEDS_CLIMBING_GEAR       8         // Climbing gear required to go up
#define X_CLIMBING_GEAR_TO_REPEL    9         // Climbing gear require to repel
#define X_DIFFICULT_CLIMB           10        // Very difficult climb
#define X_NEEDS_FLY                 11        // Must fly to go that way
#define X_FEMALE_ONLY               12        // female only exit
#define X_MALE_ONLY                 13        // male only exit
#define X_PLEDGE_ONLY               14        // pledge player exit only
#define X_PORTAL                    15        // portal
#define X_NIGHT_ONLY                16        // only open at night
#define X_DAY_ONLY                  17        // only open during day
#define X_PASSIVE_GUARD             18        // passive guarded exit
#define X_NO_SEE                    19        // Can not use / see exit
#define X_CSEL_INVERT               20        // Class selective exit
#define X_SEL_ASSASSIN              21        // Assassin
#define X_SEL_BERSERKER             22        // Berserker
#define X_SEL_CLERIC                23        // Cleric
#define X_SEL_FIGHTER               24        // Fighter
#define X_SEL_MAGE                  25        // Mage
#define X_SEL_PALADIN               26        // Paladin
#define X_SEL_RANGER                27        // Ranger
#define X_SEL_THIEF                 28        // Thief
#define X_SEL_VAMPIRE               29        // Vampire
#define X_SEL_MONK                  30        // Monk
#define X_SEL_DEATHKNIGHT           31        // Death Knight
#define X_SEL_DRUID                 32        // Druid
#define X_SEL_LICH                  33        // Lich
#define X_SEL_WEREWOLF              34        // Werewolf
#define X_SEL_BARD                  35        // Bard
#define X_SEL_ROGUE                 36        // Rogue
#define X_NO_GOOD_ALIGN             37        // blocks good players
#define X_NO_NEUTRAL_ALIGN          38        // blocks neutral players
#define X_NO_EVIL_ALIGN             39        // blocks evil players
#define X_STAFF_ONLY                40        // staff only
#define X_CLAN_1                    41        // Clan 1 can use
#define X_CLAN_2                    42        // Clan 2 can use
#define X_CLAN_3                    43        // Clan 3 can use
#define X_CLAN_4                    44        // Clan 4 can use
#define X_CLAN_5                    45        // Clan 5 can use
#define X_CLAN_6                    46        // Clan 6 can use
#define X_CLAN_7                    47        // Clan 7 can use
#define X_CLAN_8                    48        // Clan 8 can use
#define X_CLAN_9                    49        // Clan 9 can use
#define X_CLAN_10                   50        // Clan 10 can use
#define X_CLAN_11                   51        // Clan 11 can use
#define X_CLAN_12                   52        // Clan 12 can use
#define X_NO_WANDER                 53        // monster will not wander here
#define X_TO_STORAGE_ROOM           54        // Leads to a storage room
#define X_NO_FLEE                   55        // Cannot flee to this exit
#define X_NO_SCOUT                  56        // Cannot scout this exit
#define X_CONCEALED                 57      // Concealed exit - very hard to find
#define X_DESCRIPTION_ONLY          58      // Exit shown only in room description
#define X_NO_MIST                   59      // Mist may not enter this exit
#define X_TOLL_TO_PASS              60      // Exit requires toll to enter
#define X_LEVEL_BASED_TOLL          61      // Exit's toll varies on player level
#define X_WATCHER_UNLOCK            62      // Watcher can unlock w/o key - no key
#define X_MOVING                    63      // Moving exit (do not set)
#define X_TO_BOUND_ROOM             64      // Exit to bound room
#define X_RSEL_INVERT               65      // race invert
#define X_SEL_DWARF                 66      // race selective: dwarf
#define X_SEL_ELF                   67      // race selective: elf
#define X_SEL_HALFELF               68      // race selective: half-elf
#define X_SEL_HALFLING              69      // race selective: halfling
#define X_SEL_HUMAN                 70      // race selective: human
#define X_SEL_ORC                   71      // race selective: orc
#define X_SEL_HALFGIANT             72      // race selective: half-giant
#define X_SEL_GNOME                 73      // race selective: gnome
#define X_SEL_TROLL                 74      // race selective: troll
#define X_SEL_HALFORC               75      // race selective: half-orc
#define X_SEL_OGRE                  76      // race selective: ogre
#define X_SEL_DARKELF               77      // race selective: dark-elf
#define X_SEL_GOBLIN                78      // race selective: goblin
#define X_SEL_MINOTAUR              79      // race selective: minotaur
#define X_SEL_SERAPH                80      // race selective: seraph
#define X_SEL_KOBOLD                81      // race selective: kobold
#define X_SEL_CAMBION               82      // race selective: cambion
#define X_SEL_BARBARIAN             83      // race selective: barbarian
#define X_SEL_KATARAN               84      // race selective: kataran
#define X_SEL_TIEFLING              85      // race selective: tiefling
#define X_WINTER                    86      // not in winter
#define X_SPRING                    87      // not in spring
#define X_SUMMER                    88      // not in summer
#define X_AUTUMN                    89      // not in autumn
#define X_ONOPEN_PLAYER             90      // when opening, print to player only
#define X_TO_PREVIOUS               91      // take player to room they were previously in
#define X_MIST_ONLY                 92      // only a mist can pass through
#define X_CAN_LOOK                  93      // Can look through this exit (free scout)
#define X_LOOK_ONLY                 94      // Can only look through this exit (free scout)
#define X_LOCK_AFTER_USAGE          95      // Exit re-locks after single usage
#define X_NO_KNOCK_SPELL            96      // Exit immune to knock spell
#define MAX_EXIT_FLAGS              97      // Incriment when adding... check structs.h for max
#endif /*FLAGS_H_*/

