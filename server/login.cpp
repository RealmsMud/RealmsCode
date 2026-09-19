/*
 * login.cpp
 *   Functions to log a person into the mud or create a new character
 *   ____            _
 *  |  _ \ ___  __ _| |_ __ ___  ___
 *  | |_) / _ \/ _` | | '_ ` _ \/ __|
 *  |  _ <  __/ (_| | | | | | | \__ \
 *  |_| \_\___|\__,_|_|_| |_| |_|___/
 *
 * Permission to use, modify and distribute is granted via the
 *  GNU Affero General Public License v3 or later
 *
 *  Copyright (C) 2007-2021 Jason Mitchell, Randi Mitchell
 *     Contributions by Tim Callahan, Jonathan Hseu
 *  Based on Mordor (C) Brooke Paul, Brett J. Vickers, John P. Freeman
 *
 */

#include <fmt/format.h>                          // for format
#include <strings.h>                             // for strncasecmp
#include <boost/algorithm/string/trim.hpp>       // for trim
#include <algorithm>                             // for find_if, transform, min
#include <array>                                 // for array
#include <cctype>                                // for isalpha, tolower
#include <cstddef>                               // for size_t
#include <ctime>                                 // for time
#include <fstream>                               // for ifstream
#include <iomanip>                               // for operator<<, setw
#include <locale>                                // for locale
#include <span>                                  // for span
#include <sstream>                               // for char_traits, operator<<
#include <string>                                // for string, allocator
#include <string_view>                           // for operator==, string_view
#include <vector>                                // for vector

#include "catRef.hpp"                            // for CatRef
#include "cmd.hpp"                               // for cmd
#include "commands.hpp"                          // for parse, cmdReconnect
#include "config.hpp"                            // for Config, gConfig, Rac...
#include "creatureStreams.hpp"                   // for Streamable, ColorOff
#include "deityData.hpp"                         // for DeityData
#include "flags.hpp"                             // for P_HARDCORE, O_STARTING
#include "global.hpp"                            // for CreatureClass, Creat...
#include "location.hpp"                          // for Location
#include "login.hpp"                             // for CON_*/LOGIN_*/CREATE_* states
#include "login/accountAuthService.hpp"          // for AccountAuth::checkName
#include "login/characterDraft.hpp"              // for CharacterDraft
#include "login/characterCreationService.hpp"    // for Create:: validators
#include "login/characterMaterializer.hpp"       // for Create::materialize
#include "login/creationFlow.hpp"                // for Create::Flow
#include "login/creationSession.hpp"             // for CreationSession
#include "login/loginSession.hpp"                // for LoginSession
#include "money.hpp"                             // for Money
#include "mudObjects/container.hpp"              // for ObjectSet
#include "mudObjects/creatures.hpp"              // for CustomCrt, Creature
#include "mudObjects/objects.hpp"                // for Object
#include "mudObjects/players.hpp"                // for Player
#include "paths.hpp"                             // for Config, CreateHelp
#include "playerClass.hpp"                       // for PlayerClass
#include "proto.hpp"                             // for low, get_class_string
#include "raceData.hpp"                          // for RaceData
#include "server.hpp"                            // for Server, gServer
#include "socket.hpp"                            // for Socket
#include "stats.hpp"                             // for Stat
#include "structs.hpp"                           // for SEX_FEMALE, SEX_MALE
#include "xml.hpp"                               // for loadPlayer, loadObject
#include "account.hpp"                           // for Account

class StartLoc;

// Forward declarations for account login functions
std::shared_ptr<Account> validateAndGetAccount(const std::shared_ptr<Socket>& sock);
void showAccountMenu(const std::shared_ptr<Socket>& sock, std::shared_ptr<Account> account);
void handleAccountMenuCommand(const std::shared_ptr<Socket>& sock, std::shared_ptr<Account> account, const std::string& str);
void handleCharacterClaim(const std::shared_ptr<Socket>& sock, std::shared_ptr<Account> account, const std::string& str);
bool loadCharacterForPlay(const std::shared_ptr<Socket>& sock, std::shared_ptr<Account> account, const std::string& charName);

namespace {
    const constexpr char* kPromptAccountName = "Please enter account name: ";
    const constexpr char* kPromptLegacyName  = "Please enter character name for legacy login: ";
    const constexpr char* kPromptEmail       = "Enter new email address (or press enter to clear): ";
    const constexpr char* kPromptConfirmName = "Did I get that right? (yes/no): ";
}

//*********************************************************************
//                      cmdReconnect
//*********************************************************************

int cmdReconnect(const std::shared_ptr<Player>& player, cmd* cmnd) {
    const std::shared_ptr<Socket> sock = player->getSock();

    player->print("\n\nReconnecting.\n\n\n");
    player->save(true);

    player->uninit();
    gServer->clearPlayer(player);
    sock->clearPlayer();
    sock->reconnect();
    return(0);
}

bool Player::checkProxyAccess(const std::shared_ptr<Player>& proxy) {
    if(!proxy)
        return(false);

    // Only a DM can proxy a DM
    if(this->isDm() && !proxy->isDm())
        return(false);

    // A DM can proxy anybody
    if(proxy->isDm())
        return(true);

    return(gConfig->hasProxyAccess(proxy, Containable::downcasted_shared_from_this<Player>()));
}
//*********************************************************************
//                      login
//*********************************************************************

static void enterLogin(const std::shared_ptr<Socket>& sock, LoginSession::Step step) {
    sock->getLoginSession().step = step;
    sock->setState(LOGIN_ACCOUNT);
}

void login(const std::shared_ptr<Socket>& sock, const std::string& inStr) {
    std::shared_ptr<Account> account=nullptr;
    if(!sock) {
        std::clog << "**** ERORR: Null socket in login.\n";
        return;
    }

    std::string str = inStr;

    if(sock->getState() == LOGIN_DNS_LOOKUP) {
        sock->print("Still performing DNS lookup, please be patient!\n");
        return;
    }

    using Step = LoginSession::Step;
    LoginSession& session = sock->getLoginSession();

    switch(session.step) {
        case Step::LockoutPassword: {
            if(str != session.lockoutPassword) {
                sock->disconnect();
                return;
            }
            sock->print("\n\nAn account can hold several characters.\nLegacy characters can be claimed by an account.\n\nLogin Options:");
            sock->print("\n  ^Wa^x) Create or Login into an account");
            sock->print("\n  ^Wb^x) Login in with a character name directly (Legacy)");
            sock->askFor("\n\nEnter choice (a/b): ");
            enterLogin(sock, Step::EntryChoice);
            return;
        }
        case Step::EntryChoice: {
            std::string choice = str;
            boost::trim(choice);
            if(choice.empty()) {
                sock->askFor("Enter choice (a/b): ");
                return;
            }
            const char c = std::tolower(choice[0]);
            if(c == 'a') {
                sock->askFor(kPromptAccountName);
                enterLogin(sock, Step::AccountName);
                return;
            } else if(c == 'b') {
                sock->askFor(kPromptLegacyName);
                enterLogin(sock, Step::LegacyName);
                return;
            } else {
                sock->askFor("Please enter A or B: ");
                return;
            }
        }
        case Step::AccountName: {
            lowercize(str, 1);
            if(!nameIsAllowed(str, sock)) {
                sock->askFor(kPromptAccountName);
                return;
            }

            session.accountName = str;

            auto resolved = AccountAuth::resolveAccount(str);
            account = resolved.account;
            if(!resolved.found) {
                if(isdm(str)) {
                    sock->print("\nYou must enter a password to create that account.\n");
                    sock->echoOff();
                    sock->askFor("Please enter password: ");
                    enterLogin(sock, Step::AccountDmPassword);
                    return;
                }
                sock->print("\n%s? ", str.c_str());
                sock->askFor(kPromptConfirmName);
                enterLogin(sock, Step::CheckCreateAccount);
                return;
            } else {
                if(resolved.banned) {
                    sock->print("Account is banned: %s\n", resolved.banReason.c_str());
                    sock->disconnect();
                    return;
                }
                sock->echoOff();
                sock->askFor("Please enter account password: ");
                enterLogin(sock, Step::AccountPassword);
                return;
            }
        }
        case Step::AccountDmPassword: {
            sock->echoOn();
            if(str != gConfig->getDmPass()) {
                sock->disconnect();
                return;
            }

            sock->print("\n%s? ", session.accountName.c_str());
            sock->askFor(kPromptConfirmName);
            enterLogin(sock, Step::CheckCreateAccount);
            return;
        }
        case Step::LegacyName: {
            std::string charName = str;
            boost::trim(charName);
            if(charName.empty()) {
                sock->askFor(kPromptLegacyName);
                return;
            }
            lowercize(charName, 1);

            auto lookup = AccountAuth::lookupLegacyCharacter(charName);
            if(!lookup.found) {
                sock->print("Character '%s' does not exist.\n", charName.c_str());
                sock->askFor(kPromptLegacyName);
                return;
            }
            if(lookup.alreadyClaimed) {
                sock->print("Character '%s' is already claimed by an account.\n", charName.c_str());
                sock->print("Please login using the account system instead of legacy login.\n");
                sock->askFor(kPromptAccountName);
                enterLogin(sock, Step::AccountName);
                return;
            }
            session.targetCharacter = charName;
            sock->print("Legacy login for character '%s'.\n", charName.c_str());
            sock->echoOff();
            sock->askFor("Please enter character password: ");
            enterLogin(sock, Step::LegacyPassword);
            return;
        }
        case Step::CheckCreateAccount: {
            if(str[0] != 'y' && str[0] != 'Y') {
                session.accountName.clear();
                sock->askFor(kPromptAccountName);
                enterLogin(sock, Step::AccountName);
                return;
            } else {
                sock->print("\nCreating new account...\n");
                sock->askFor(fmt::format("Please set a password for new account '{}': ", session.accountName).c_str());
                sock->echoOff();
                enterLogin(sock, Step::AccountCreatePassword);
                return;
            }
        }
        case Step::AccountPassword: {
            sock->echoOn();
            const std::string& accountName = session.accountName;
            account = AccountAuth::resolveAccount(accountName).account;
            if(!account || !AccountAuth::verifyPassword(*account, str)) {
                sock->write("\n\rIncorrect.\n\r");
                logn("log.incorrect", fmt::format("Invalid account password for {} from {}\n", accountName, sock->getHostname()).c_str());
                sock->disconnect();
                return;
            } else {
                AccountAuth::recordLogin(*account);
                sock->setAccount(account);
                // Show character selection
                showAccountMenu(sock, account);
                return;
            }
        }
        case Step::AccountCreatePassword: {
            sock->echoOn();

            const std::string& accountName = session.accountName;
            if(!AccountAuth::isValidPassword(str)) {
                sock->print("\nPassword must be between 5 and 35 characters.\n");
                sock->askFor(fmt::format("Please set a password for new account '{}': ", accountName).c_str());
                sock->echoOff();
                enterLogin(sock, Step::AccountCreatePassword);
                return;
            }

            account = AccountAuth::createAccount(accountName, str);
            if(!account) {
                sock->print("Error creating account. Please try again.\n");
                sock->askFor(kPromptAccountName);
                enterLogin(sock, Step::AccountName);
                return;
            }

            sock->print("\n^GAccount '%s' created successfully!^x\n", accountName.c_str());
            sock->setAccount(account);
            showAccountMenu(sock, account);
            return;
        }
        case Step::AccountMenu: {
            account = validateAndGetAccount(sock);
            if(!account) return;

            handleAccountMenuCommand(sock, account, str);
            return;
        }
        case Step::ClaimCharacter: {
            account = validateAndGetAccount(sock);
            if(!account) return;

            handleCharacterClaim(sock, account, str);
            return;
        }
        case Step::ClaimPassword: {
            sock->echoOn();
            account = validateAndGetAccount(sock);
            if(!account) return;

            const std::string charName = session.targetCharacter;

            // Load the character to verify password
            std::shared_ptr<Player> player;
            if(!loadPlayer(charName, player)) {
                sock->print("Error: Character no longer exists!\n");
                showAccountMenu(sock, account);
                return;
            }

            // Check password
            if(!AccountAuth::verifyCharacterPassword(*player, str)) {
                sock->print("\n^RIncorrect password for character '%s'.^x\n", charName.c_str());
                sock->print("Character claim failed.\n");
                showAccountMenu(sock, account);
                return;
            }

            // Success! Claim the character
            if(!AccountAuth::claimCharacter(*account, *player)) {
                sock->print("Error: Could not add character to account.\n");
                showAccountMenu(sock, account);
                return;
            }
            player->save();
            account->save();

            sock->print("\n^GCharacter '%s' has been successfully claimed!^x\n", charName.c_str());
            sock->print("The character is now linked to your account.\n");
            showAccountMenu(sock, account);
            return;
        }
        case Step::SetEmail: {
            account = validateAndGetAccount(sock);
            if(!account) return;

            std::string email = str;
            boost::trim(email);

            // Empty string clears the email
            if(email.empty()) {
                account->setEmail("");
                account->save();
                sock->print("^GEmail address cleared.^x\n");
                showAccountMenu(sock, account);
                return;
            }

            switch(AccountAuth::checkEmail(email)) {
            case AccountAuth::EmailError::BadFormat:
                sock->print("^RInvalid email format. Please enter a valid email address.^x\n");
                sock->askFor(kPromptEmail);
                return;
            case AccountAuth::EmailError::TooLong:
                sock->print("^REmail address too long (maximum 255 characters).^x\n");
                sock->askFor(kPromptEmail);
                return;
            case AccountAuth::EmailError::Ok:
                break;
            }

            session.email = email;
            sock->print("Confirm email address: ^C%s^x\n", email.c_str());
            sock->askFor("Is this correct? (y/n): ");
            enterLogin(sock, Step::SetEmailConfirm);
            return;
        }
        case Step::SetEmailConfirm: {
            account = validateAndGetAccount(sock);
            if(!account) return;

            if(str.empty() || (str[0] != 'y' && str[0] != 'Y')) {
                sock->print("Email not set.\n");
                showAccountMenu(sock, account);
                return;
            }

            // Set and save the email
            const std::string email = session.email;
            account->setEmail(email);
            account->save();

            sock->print("^GEmail address set to: ^C%s^x^G.^x\n", email.c_str());
            showAccountMenu(sock, account);
            return;
        }
        case Step::LegacyPassword: {
            sock->echoOn();
            std::string charName = session.targetCharacter;

            // Load the character
            std::shared_ptr<Player> player;
            if(!loadPlayer(charName, player)) {
                sock->print("\n^RCharacter '%s' does not exist.^x\n", charName.c_str());
                sock->askFor(kPromptLegacyName);
                enterLogin(sock, Step::LegacyName);
                return;
            }

            // Check password
            if(!AccountAuth::verifyCharacterPassword(*player, str)) {
                sock->print("\n^RIncorrect.^x\n");
                logn("log.incorrect", fmt::format("Invalid legacy password for {} from {}\n", charName, sock->getHostname()).c_str());
                sock->disconnect();
                return;
            }

            // Success! Log the player in directly
            sock->print("\n^GLegacy login successful!^x\n");

            // Update last login time
            player->setLastLogin(time(nullptr));

            // Set up the socket and player connection
            sock->setPlayer(player);
            player->fd = -1;

            // Complete the login process
            sock->finishLogin();

            return;
        }
    }
}

//*********************************************************************
//                    showAccountMenu
//*********************************************************************

void showAccountMenu(const std::shared_ptr<Socket>& sock, std::shared_ptr<Account> account) {
    sock->print("\n\n^W~~~~~~~ Account Menu ~~~~~~~^x\n\n");
    account->printInfoFields(sock);
    
    // Show command options
    sock->print("^WCommands:^x\n");
    sock->print("  ^W%-15s^x - Create a new character", "^c(c)^Wreate^x");
    if(account->canCreateCharacter()) {
        sock->print("\n");
    } else {
        sock->print(" ^R(limit reached)^x\n");
    }

    sock->print("  ^W%-15s^x - List your characters\n", "^c(l)^Wist^x");
    sock->print("  ^W%-15s^x - Claim a legacy character\n", "^c(cl)^Waim^x");
    sock->print("  ^W%-15s^x - Set email address\n", "^c(e)^Wmail^x");
    sock->print("  ^W%-15s^x - Disconnect\n", "^c(q)^Wuit^x");
    
    const auto& characters = account->getCharacterNames();
    if (characters.empty()) {
        sock->askFor("\nEnter a command: ");
    } else {
        sock->askFor("\nEnter a command, or a character name to play: ");
    }

    enterLogin(sock, LoginSession::Step::AccountMenu);
}

//*********************************************************************
//                    handleAccountMenuCommand
//*********************************************************************

void handleAccountMenuCommand(const std::shared_ptr<Socket>& sock, std::shared_ptr<Account> account, const std::string& str) {
    if(str.empty()) {
        showAccountMenu(sock, account);
        return;
    }
    
    std::string input = str;
    std::transform(input.begin(), input.end(), input.begin(), ::tolower);
    boost::trim(input);
    
    const auto& characters = account->getCharacterNames();
    
    // Extract just the command word (first word) for proper matching
    std::string command;
    const size_t spacePos = input.find(' ');
    if(spacePos != std::string::npos) {
        command = input.substr(0, spacePos);
    } else {
        command = input;
    }
    
    // Handle "create" command with partial matching
    if(command.length() >= 1 && !strncasecmp(command.c_str(), "create", std::min(command.length(), 6UL))) {
        if(!account->canCreateCharacter()) {
            sock->print("^RYou have reached your character limit (%d/%d).^x\n", 
                       account->getCharacterCount(), account->getCharacterLimit());
            sock->print("You must delete a character before creating a new one.\n\n");
            showAccountMenu(sock, account);
            return;
        }
        
        // Check if IP has reached max connections
        if(gServer->checkDouble(sock, false)) {
            sock->print("^RMaximum number of characters already connected.\n");
            sock->print("Please disconnect one of your other characters first.\n\n");
            showAccountMenu(sock, account);
            return;
        }
        sock->print("\nCreating new character...");
        sock->print("\nTo get help at any time during creation use the \"^Whelp^x\" command.\n");
        startCharacterCreation(sock);
        return;
    }
    
    // Handle "list" command with partial matching
    if(command.length() >= 1 && !strncasecmp(command.c_str(), "list", std::min(command.length(), 4UL))) {
        account->printCharacterList(sock);
        if(account->getCharacterNames().empty())
            sock->askFor("\nEnter a command: ");
        else
            sock->askFor("\nEnter a command, or a character name to play: ");
        return;
    }
    
    // Handle "claim" command with partial matching
    if(command.length() >= 2 && !strncasecmp(command.c_str(), "claim", std::min(command.length(), 5UL))) {
        if(!account->canCreateCharacter()) {
            sock->print("^RYou have reached your character limit (%d/%d).^x\n", 
                       account->getCharacterCount(), account->getCharacterLimit());
            sock->print("You cannot claim additional characters.\n");
            sock->askFor("Press ^W<Enter>^x to continue: ");
            showAccountMenu(sock, account);
            return;
        }
        sock->print("\n^WClaim Legacy Character^x\n");
        sock->print("Enter the name of the character you wish to claim.\n");
        sock->print("This character must have existed before the account system was implemented.\n");
        sock->askFor("Character name: ");
        enterLogin(sock, LoginSession::Step::ClaimCharacter);
        return;
    }
    
    // Handle "email" command with partial matching
    if(command.length() >= 1 && !strncasecmp(command.c_str(), "email", std::min(command.length(), 5UL))) {
        if(!account->getEmail().empty()) {
            sock->print("Current email: %s\n", account->getEmail().c_str());
        }
        sock->askFor(kPromptEmail);
        enterLogin(sock, LoginSession::Step::SetEmail);
        return;
    }
    
    // Handle "quit" command with partial matching
    if(command.length() >= 1 && !strncasecmp(command.c_str(), "quit", std::min(command.length(), 4UL))) {
        sock->print("Goodbye!\n");
        sock->disconnect();
        return;
    }
    
    // Handle character name login
    // First check if IP has reached max connections
    if(gServer->checkDouble(sock, false)) {
        sock->print("^RMaximum number of characters already connected.\n");
        sock->print("Please disconnect one of your other characters first.\n\n");
        showAccountMenu(sock, account);
        return;
    }

    std::string foundChar;
    for(const auto& accountChar : characters) {
        if(strcasecmp(accountChar.c_str(), input.c_str()) == 0) {
            foundChar = accountChar;
            break;
        }
    }
    
    if(!foundChar.empty()) {
        // Load and play the character from our account
        if(loadCharacterForPlay(sock, account, foundChar)) {
            sock->finishLogin();
        }
        return;
    }
    
    // Character not in our account - check if it exists and if we have proxy access
    // Format the character name properly (capitalize first letter)
    std::string formattedInput = input;
    lowercize(formattedInput, 1);
    
    std::shared_ptr<Player> player = nullptr;
    if(loadPlayer(formattedInput, player)) {
        // Check if any of our account's characters has proxy access to this character
        std::shared_ptr<Player> proxyGranter = nullptr;
        
        for(const auto& accountCharName : characters) {
            std::shared_ptr<Player> accountChar = nullptr;
            if(loadPlayer(accountCharName, accountChar)) {
                if(player->checkProxyAccess(accountChar)) {
                    proxyGranter = accountChar;
                    break;
                }
            }
        }
        
        if(proxyGranter) {
            // Set up the proxy relationship
            player->setProxy(proxyGranter);
            
            // Set up the socket for the player
            player->fd = -1;
            sock->setPlayer(player);
            
            sock->print("Loading %s (using %s as proxy)...\n", player->getName().c_str(), proxyGranter->getName().c_str());
            sock->finishLogin();
            return;
        }
    }
    
    // Invalid command
    sock->print("Invalid command or character name '%s'.\n", input.c_str());
    showAccountMenu(sock, account);
}

//*********************************************************************
//                    loadCharacterForPlay
//*********************************************************************

bool loadCharacterForPlay(const std::shared_ptr<Socket>& sock, std::shared_ptr<Account> account, const std::string& charName) {
    // Verify character still exists and belongs to this account
    std::shared_ptr<Player> player;
    if(!loadPlayer(charName, player)) {
        sock->print("Character '%s' no longer exists!\n", charName.c_str());
        // Remove from account
        account->removeCharacter(charName);
        account->save();
        showAccountMenu(sock, account);
        return false;
    }
    
    // Check if character belongs to this account (migration support)
    if(player->getAccountName() != account->getName()) {
        // Update player's account name
        player->setAccountName(account->getName());
        player->save();
    }
    
    // Load character for login
    sock->setPlayer(player);
    player->fd = -1;
    
    return true;
}

// Helper function to validate account exists and is loaded, returns account or nullptr if invalid (handles error)
std::shared_ptr<Account> validateAndGetAccount(const std::shared_ptr<Socket>& sock) {
    std::shared_ptr<Account> account = sock->getAccount();
    if(!account) {
        sock->print("Error: No account found!\n");
        sock->disconnect();
        return nullptr;
    }
    return account;
}

void Socket::finishLogin() {
    auto player = getPlayer();
    const std::string charName = player->getName();

    auto sThis = shared_from_this();
    gServer->checkDuplicateName(sThis, true);
    if(gServer->checkDouble(sThis)) {
        return;
    }


    const std::string proxyName = player->getProxyName();
    const std::string proxyId = player->getProxyId();
    player = nullptr;
    myPlayer = nullptr;

    if(!loadPlayer(charName, player)) {
        askFor("Player no longer exists!\n\nPlease enter account name: ");
        getLoginSession().step = LoginSession::Step::AccountName;
        setState(LOGIN_ACCOUNT);
        return;
    }
    player->setProxy(proxyName, proxyId);
    if(player->flagIsSet(P_HARDCORE)) {
        const StartLoc *location = gConfig->getStartLoc("highport");
        player->bind(location);
        // put them in the docks: hp.100
        player->currentLocation.room.area = "hp";
        player->currentLocation.room.id = 100;
        // remove all their stuff
        player->coins.zero();
        player->objects.clear();

    }

    setPlayer(player);
    player->fd = getFd();
    player->setSock(sThis);
    player->init();

    registerPlayer();
    setState(CON_PLAYING);

    if(player->flagIsSet(P_HARDCORE)) {
        player->clearFlag(P_HARDCORE);
        player->print("\n\n\n");
        player->print("You wake up on the shores of Derlith, completely naked...\n");
        player->print("\n\n");
        player->print("You do not know how long you have been unconscious, or how long you have\n");
        player->print("been floating at sea. The sounds of the ocean fill the air, interrupted by\n");
        player->print("the bustling noise of a city street. An occasional bell rings and the\n");
        player->print("sounds of horse-drawn carriages stumbling over cobblestone soon grow louder\n");
        player->print("and louder. You look around to find yourself at the docks of Highport.\n");
        player->print("People stare at you as they pass.\n");
        player->print("\n\n");
        player->print("You no longer feel hunted, as you did in Oceancrest. You feel the peaceful\n");
        player->print("onset of immortality that comes with living in Derlith. You are now free to\n");
        player->print("continue your adventures in this world...\n");
        player->print("\n\n\n");
    }
}

//*********************************************************************
//                      doCreateHelp
//*********************************************************************

void doCreateHelp(const std::shared_ptr<Socket>& sock, std::string_view str) {
    cmd cmnd;
    parse(str, &cmnd);

    if(cmnd.num < 2) {
        sock->viewFile((Path::CreateHelp / "helpfile.txt").string());
        return;
    }

    const std::string_view topic(cmnd.str[1]);
    if(topic.find('/') != std::string_view::npos) {
        sock->print("You may not use slashes or backslashes.\n");
        return;
    }
    sock->viewFile((Path::CreateHelp / (std::string(topic) + ".txt")).string());
}

//*********************************************************************
//                      startCharacterCreation
//*********************************************************************

static void enterCreate(const std::shared_ptr<Socket>& sock, CreationSession::Step step) {
    sock->getCreationSession().step = step;
    sock->setState(CREATE_CHARACTER);
}

void startCharacterCreation(const std::shared_ptr<Socket>& sock) {
    sock->resetCreationDraft();
    sock->resetCreationSession();
    sock->print("\n");
    // The wizard derefs entries from all four tables; refuse up front rather than
    // null-check every render step.
    if(gConfig->isCharCreationDisabled() || gConfig->races.empty() || gConfig->classes.empty() ||
       gConfig->deities.empty() || gConfig->start.empty()) {
        sock->print("Character creation is temporarily unavailable.\n");
        if(auto account = sock->getAccount())
            showAccountMenu(sock, account);
        else
            sock->reconnect();
        return;
    }
    // The DM gate is keyed on the chosen name, not the account; lockout 2 already proved
    // its password at connection entry (Socket::checkLockOut -> Step::LockoutPassword).
    if(gConfig->isLockedOut(sock) == 1) {
        sock->disconnect();
        return;
    }
    Create::renderRace(sock);
}

//*********************************************************************
//                      createPlayer
//*********************************************************************

namespace {
    using Step = CreationSession::Step;
    namespace Flow = Create::Flow;

    // Steps whose telnet input is a positional letter into Flow::options().
    constexpr bool isLetterStep(Step step) {
        return step == Step::Race || step == Step::SubRace || step == Step::Class ||
               step == Step::Deity || step == Step::Location || step == Step::Prof ||
               step == Step::SecondProf;
    }

    struct Decoded {
        std::string value;
        std::string label;
    };

    // Werewolf/monk innate skills: telnet renders them as their own block and echoes
    // nothing when they are picked.
    bool isInnateWeapon(const std::string& value) {
        return value == "claw" || value == "bare-hand";
    }

    // Non-alphabetic or out-of-range input decodes to an empty value, so advance()
    // rejects it with the step's own error rather than a second copy of the bounds.
    Decoded decodeInput(const CharacterDraft& draft, Step step, const std::string& str) {
        if(!isLetterStep(step))
            return {str, ""};
        if(str.empty() || !std::isalpha(static_cast<unsigned char>(str[0])))
            return {};
        const auto options = Flow::options(draft, step);
        const auto idx = static_cast<std::size_t>(low(str[0]) - 'a');
        if(idx >= options.size())
            return {};
        return {options[idx].value, options[idx].label};
    }

    void printStartLocation(const std::shared_ptr<Socket>& sock, const CharacterDraft& draft) {
        const auto options = Flow::options(draft, Step::Location);
        const auto it = std::find_if(options.begin(), options.end(),
                                     [&draft](const Flow::ViewOption& o) { return o.value == draft.startLocation; });
        sock->print("\n\nYour starting location is: ");
        sock->print("\n   %s\n\n", it != options.end() ? it->label.c_str() : draft.startLocation.c_str());
    }

    // Post-transition confirmation copy; reads the already-mutated draft.
    void confirmChoice(const std::shared_ptr<Socket>& sock, const CharacterDraft& draft, Step step,
                       Step next, const Decoded& picked) {
        switch(step) {
        case Step::Race:
            if(next != Step::SubRace)   // a parent race is confirmed after the subrace pick
                Create::finishRace(sock);
            break;
        case Step::SubRace:
            Create::finishRace(sock);
            break;
        case Step::Sex:
            sock->printColor(draft.sex == SEX_FEMALE ? "You are now ^Wfemale^x.\n" : "You are now ^Wmale^x.\n");
            break;
        case Step::Class:
            sock->printColor("Your chosen class: ^W%s\n", getFullClassName(draft.cClass, draft.secondClass).c_str());
            break;
        case Step::Deity: {
            const DeityData* deity = gConfig->getDeity(draft.deity);
            sock->printColor("Your chosen deity: ^W%s\n", deity ? deity->getName().c_str() : "none");
            break;
        }
        case Step::StatsChoice:
            if(next == Step::Stats)
                sock->print("You have chosen to select your own stats.\n");
            break;
        case Step::Prof:
        case Step::SecondProf:
            if(!isInnateWeapon(picked.value))
                sock->bprint(fmt::format("You have learned how to use ^W{}^x.\n", picked.label));
            break;
        default:
            break;
        }
        // The start location is settled either by an explicit pick or by the auto-resolve
        // on leaving Class/Deity without a Location menu.
        if(step == Step::Location ||
           ((step == Step::Class || step == Step::Deity) && next != Step::Deity && next != Step::Location))
            printStartLocation(sock, draft);
        if(next == Step::Prof)   // stats are final once Prof is entered, on every path
            Create::finishStats(sock);
    }

    // Every render* sets the wizard cursor (enterCreate) itself.
    void renderStep(const std::shared_ptr<Socket>& sock, Step step, const Create::InitialSkills& granted) {
        switch(step) {
        case Step::Race:        Create::renderRace(sock); break;
        case Step::SubRace:     Create::renderSubRace(sock); break;
        case Step::Sex:         Create::renderSex(sock); break;
        case Step::Class:       Create::renderClass(sock); break;
        case Step::Deity:       Create::renderDeity(sock); break;
        case Step::Location:    Create::renderLocation(sock); break;
        case Step::StatsChoice: Create::renderStatsChoice(sock); break;
        case Step::Stats:       Create::renderStats(sock); break;
        case Step::BonusStat:   Create::renderBonusStat(sock); break;
        case Step::PenaltyStat: Create::renderPenaltyStat(sock); break;
        case Step::Prof:        Create::renderProf(sock, granted); break;
        case Step::SecondProf:  Create::renderSecondProf(sock); break;
        case Step::Name:        Create::renderName(sock); break;
        case Step::DmPassword:  Create::renderDmPassword(sock); break;
        case Step::Done:        Create::renderDone(sock); break;
        }
    }

    // The Flow message is the telnet copy; a format rejection prints on its own line
    // (it may carry more than one), everything else leads the re-prompt.
    void printNameError(const std::shared_ptr<Socket>& sock, const Flow::StepError& error) {
        if(error.code == "nameBadFormat") {
            sock->print("%s\n", error.message.c_str());
            sock->askFor("Please choose a character name: ");
            return;
        }
        sock->askFor(fmt::format("\n{} Choose a character name: ", error.message).c_str());
    }

    // Race/Class/Deity redraw the whole menu; the rest re-prompt in place.
    void rejectInput(const std::shared_ptr<Socket>& sock, Step step, const std::string& str,
                     const Flow::StepError& error) {
        switch(step) {
        case Step::Race:
            renderStep(sock, step, {});
            return;
        case Step::Class:
        case Step::Deity:
            sock->printColor("Invalid selection: ^W%s\n", str.c_str());
            renderStep(sock, step, {});
            return;
        case Step::SubRace:
            sock->askFor("Choose one: ");
            return;
        case Step::Sex:
            sock->askFor("[^WM^x] Male or [^WF^x] Female: ");
            return;
        case Step::Location:
            sock->print("Invalid selection.\n");
            sock->askFor(": ");
            return;
        case Step::StatsChoice:
            sock->askFor(": ");
            return;
        case Step::Stats:
            sock->print("%s\n", error.message.c_str());
            sock->askFor(": ");
            return;
        case Step::BonusStat:
        case Step::PenaltyStat:
            sock->askFor("\nChoose one: ");
            return;
        case Step::Prof:
            sock->print("Choose a weapon skill:\n");
            sock->askFor(": ");
            return;
        case Step::SecondProf:
            sock->print("Pick a second proficiency!\n");
            sock->askFor(": ");
            return;
        case Step::Name:
            printNameError(sock, error);
            return;
        default:
            return;
        }
    }
}

void createPlayer(const std::shared_ptr<Socket>& sock, const std::string& str) {
    // Socket::restoreState calls this after the pager from any state (mail, room edit, bank).
    if(sock->getState() != CREATE_CHARACTER)
        return;
    const Step step = sock->getCreationSession().step;

    // "help" works at any step except the DM-password gate.
    if(step != Step::DmPassword && str.starts_with("help")) {
        doCreateHelp(sock, str);
        return;
    }

    // Two telnet-only steps stay outside the FSM: the DM-password gate owns the socket
    // echo and the disconnect, and Done is the commit. processDone re-checks Player::exists.
    if(step == Step::DmPassword) {
        sock->echoOn();
        if(str != gConfig->getDmPass()) {
            sock->disconnect();
            return;
        }
        Create::renderDone(sock);
        return;
    }
    if(step == Step::Done) {
        Create::processDone(sock, str);
        return;
    }

    CharacterDraft& draft = sock->getCreationDraft();
    if(str.empty()) {
        renderStep(sock, step, {});   // Socket::restoreState re-enters the step after the pager
        return;
    }

    const Decoded in = decodeInput(draft, step, str);
    const Flow::AdvanceResult result = Flow::advance(draft, step, in.value,
        {.account = sock->getAccount().get(), .allowReservedName = true});
    if(!result.ok) {
        rejectInput(sock, step, str, result.error);
        return;
    }
    confirmChoice(sock, draft, step, result.next, in);
    renderStep(sock, result.next, result.granted);
}



//*********************************************************************
//                      addStartingItem
//*********************************************************************

void Create::addStartingItem(const std::shared_ptr<Player>& player, const std::string &area, int id, bool wear, bool skipUseCheck, int num) {
    CatRef cr;
    cr.setArea(area);
    cr.id = id;

    for(int i=0; i<num; i++) {
        std::shared_ptr<Object>  object=nullptr;
        if(loadObject(cr, object)) {
            object->setFlag(O_STARTING);
            // if they can't use it, don't even give it to them
            if(!skipUseCheck && !player->canUse(object, true)) {
                continue;
            } else {
                object->setDroppedBy(player, "PlayerCreation");
                player->addObj(object);
                if(wear)
                    player->equip(object, false);
            }
        }
    }
}

//*********************************************************************
//                      addStartingWeapon
//*********************************************************************

void Create::addStartingWeapon(const std::shared_ptr<Player>& player, const std::string &weapon) {
    if(weapon == "sword")
        Create::addStartingItem(player, "tut", 28, false);
    else if(weapon == "great-sword")
        Create::addStartingItem(player, "tut", 21, false);
    else if(weapon == "polearm")
        Create::addStartingItem(player, "tut", 24, false);
    else if(weapon == "whip")
        Create::addStartingItem(player, "tut", 29, false);
    else if(weapon == "rapier")
        Create::addStartingItem(player, "tut", 25, false);
    else if(weapon == "spear")
        Create::addStartingItem(player, "tut", 26, false);
    else if(weapon == "axe")
        Create::addStartingItem(player, "tut", 14, false);
    else if(weapon == "great-axe")
        Create::addStartingItem(player, "tut", 18, false);
    else if(weapon == "dagger")
        Create::addStartingItem(player, "tut", 17, false);
    else if(weapon == "staff")
        Create::addStartingItem(player, "tut", 27, false);
    else if(weapon == "mace")
        Create::addStartingItem(player, "tut", 23, false);
    else if(weapon == "great-mace")
        Create::addStartingItem(player, "tut", 20, false);
    else if(weapon == "club")
        Create::addStartingItem(player, "tut", 16, false);
    else if(weapon == "hammer")
        Create::addStartingItem(player, "tut", 22, false);
    else if(weapon == "great-hammer")
        Create::addStartingItem(player, "tut", 19, false);
    else if(weapon == "bow")
        Create::addStartingItem(player, "tut", 15, false);
    else if(weapon == "crossbow")
        Create::addStartingItem(player, "tut", 30, false);
    else if(weapon == "thrown")
        Create::addStartingItem(player, "tut", 31, false);
    else if(weapon == "knife")
        Create::addStartingItem(player, "tut", 43, false);
    else if(weapon == "arcane-weapon")
        Create::addStartingItem(player, "tut", 44, false);
    else if(weapon == "divine-weapon")
        Create::addStartingItem(player, "tut", 45, false);
    else if(weapon == "flail")
        Create::addStartingItem(player, "tut", 46, false);
    else if(weapon == "sling")
        Create::addStartingItem(player, "tut", 47, false);

}

//*********************************************************************
//                      renderSex
//*********************************************************************

void Create::renderSex(const std::shared_ptr<Socket>& sock) {
    sock->askFor("Do you wish to be [^WM^x] Male or [^WF^x] Female? ");
    enterCreate(sock, CreationSession::Step::Sex);
}

//*********************************************************************
//                      renderRace
//*********************************************************************

void Create::renderRace(const std::shared_ptr<Socket>& sock) {
    const auto choices = Flow::options(sock->getCreationDraft(), Step::Race);

    sock->viewFile((Path::Config / "race_menu.0.txt").string());

    // race_menu.1.txt is art with one playable race interleaved after each of its lines.
    std::ifstream menu(Path::Config / "race_menu.1.txt");
    std::size_t k = 0;
    if(!menu.is_open()) {
        for(const auto& opt : choices)
            sock->printColor("[^W%1c^x] %-16s\n", static_cast<int>(k++) + 'A', opt.label.c_str());
    } else {
        std::string line;
        while(std::getline(menu, line)) {
            if(!line.empty() && line.back() == '\r')
                line.pop_back();
            sock->printColor("%s", line.c_str());
            if(k < choices.size()) {
                sock->printColor("[^W%1c^x] %-16s", static_cast<int>(k) + 'A', choices[k].label.c_str());
                k++;
            }
            sock->print("\n");
        }
    }
    sock->print("\n\n\n");

    sock->askFor("Choose one: ");
    enterCreate(sock, CreationSession::Step::Race);
}

//*********************************************************************
//                      renderSubRace
//*********************************************************************

void Create::renderSubRace(const std::shared_ptr<Socket>& sock) {
    std::ostringstream oStr;
    oStr.setf(std::ios::left, std::ios::adjustfield);
    oStr.imbue(std::locale(""));
    oStr << "\nChoose a sub race:\n";

    int k = 0;
    for(const auto& opt : Flow::options(sock->getCreationDraft(), Step::SubRace)) {
        if(k%2==0)
            oStr << "\n" << std::setw(20) << " ";
        const char c = ++k + 64;
        oStr << "[^W" << c << "^x] " << std::setw(16) << opt.label;
    }

    sock->printColor("%s\n\n", oStr.str().c_str());
    sock->askFor("Choose one: ");
    enterCreate(sock, CreationSession::Step::SubRace);
}

//*********************************************************************
//                      finishRace
//*********************************************************************
// Display only; materialize applies race size/languages/effects.

void Create::finishRace(const std::shared_ptr<Socket>& sock) {
    const RaceData* race = gConfig->getRace(sock->getCreationDraft().race);
    sock->printColor("\nYour chosen race: ^W%s^x\n\n", race->getName().c_str());
}

//*********************************************************************
//                      renderClass
//*********************************************************************

namespace {
    constexpr std::size_t kClassMenuLabelCount = 23;

    // Telnet-only abbreviations, indexed by class menu code - 1; the multiclass tail must
    // track resolveClassPair's MULTI_BASE order. data() is NUL-terminated: all literals.
    constexpr std::array<std::string_view, kClassMenuLabelCount> kClassMenuLabels{
        "Assassin", "Berserker", "Cleric", "Fighter", "Mage", "Paladin", "Ranger", "Thief",
        "Pureblood", "Monk", "Death Knight", "Druid", "Lich", "Werewolf", "Bard", "Rogue",
        "Figh/Mage", "Figh/Thief", "Cler/Ass", "Mage/Thief", "Thief/Mage", "Cler/Figh", "Mage/Ass"
    };
    static_assert(kClassMenuLabelCount == static_cast<std::size_t>(CLASS_COUNT_MULT) - 1,
                  "one telnet class label per class menu code");

    const char* classMenuLabel(const std::string& value) {
        const auto idx = static_cast<std::size_t>(std::stoi(value)) - 1;
        return idx < kClassMenuLabels.size() ? kClassMenuLabels[idx].data() : "";
    }
}

void Create::renderClass(const std::shared_ptr<Socket>& sock) {
    sock->print("%s", Create::classBannerTop(gConfig->getRace(sock->getCreationDraft().race)->getName()).c_str());

    int k=0;
    for(const auto& opt : Flow::options(sock->getCreationDraft(), Step::Class)) {
        if(k%2==0)
            sock->print("\n%20s", " ");
        sock->printColor("[^W%1c^x] %-16s", ++k + 64, classMenuLabel(opt.value));
    }

    sock->print("%s", Create::classBannerBottom().c_str());
    sock->askFor("Choose one: ");
    enterCreate(sock, CreationSession::Step::Class);
}

//*********************************************************************
//                      renderDeity
//*********************************************************************

void Create::renderDeity(const std::shared_ptr<Socket>& sock) {
    const CharacterDraft& draft = sock->getCreationDraft();

    if(draft.cClass == CreatureClass::CLERIC)
        sock->print("Note: Clerics of Linothan must be male. Clerics of Mara must be female.\n\n");
    sock->print("Please choose a deity:\n");

    int k=0;
    for(const auto& opt : Flow::options(draft, Step::Deity)) {
        if(k%2==0)
            sock->print("\n%20s", " ");
        sock->printColor("[^W%1c^x] %-16s", ++k + 64, opt.label.c_str());
    }
    sock->askFor("\n\nChoose one: ");
    enterCreate(sock, CreationSession::Step::Deity);
}

//*********************************************************************
//                      renderLocation
//*********************************************************************

void Create::renderLocation(const std::shared_ptr<Socket>& sock) {
    std::ostringstream menu;
    char opt = 'A';
    for(const auto& o : Flow::options(sock->getCreationDraft(), Step::Location)) {
        if(opt != 'A')
            menu << "     ";
        menu << "[^W" << opt++ << "^x] " << o.label;
    }
    sock->print("\n\nPlease choose a starting location:");
    sock->printColor("\n   %s\n\n", menu.str().c_str());
    sock->askFor(": ");
    enterCreate(sock, CreationSession::Step::Location);
}

//*********************************************************************
//                      renderStatsChoice
//*********************************************************************

void Create::renderStatsChoice(const std::shared_ptr<Socket>& sock) {
    sock->print("\nFor character stats, you may:\n");
    sock->printColor("\n[^WC^x]hoose your own stats");
    sock->printColor("\n[^WU^x]se predefined stats provided by the mud");
    sock->print("\n\nNote: For beginners that are unfamiliar with game mechanics, it is highly recommended to use predefined stats to reduce the learning curve.\n");
    sock->askFor(": ");
    enterCreate(sock, CreationSession::Step::StatsChoice);
}

//*********************************************************************
//                      renderStats
//*********************************************************************

void Create::renderStats(const std::shared_ptr<Socket>& sock) {
    sock->print("\nYou have %d points to distribute among your %zu stats. Please enter your %zu",
                Create::kStatTotal, Create::kStatCount, Create::kStatCount);
    sock->print("\nnumbers in the following order: Strength, Dexterity, Constitution,");
    sock->print("\nIntelligence, Piety.  No stat may be smaller than %d or larger than %d.",
                Create::kMinStat, Create::kMaxStat);
    sock->print("\nUse the following format: ## ## ## ## ##\n\n");
    sock->askFor(": ");
    enterCreate(sock, CreationSession::Step::Stats);
}

//*********************************************************************
//                      finishStats
//*********************************************************************
// Display only; computed from the draft, no Player.

void Create::finishStats(const std::shared_ptr<Socket>& sock) {
    const CharacterDraft& draft = sock->getCreationDraft();
    const std::array<int, 5> fs = Create::computeFinalStats(draft, gConfig->getRace(draft.race));
    sock->print("Your stats: %d %d %d %d %d\n", fs[0], fs[1], fs[2], fs[3], fs[4]);
}

//*********************************************************************
//                      renderBonusStat
//*********************************************************************

void Create::renderBonusStat(const std::shared_ptr<Socket>& sock) {
    sock->print("\nRaise which stat?\n[A] Strength, [B] Dexterity, [C] Constitution, [D] Intelligence, or [E] Piety.\n");
    sock->askFor(" : ");
    enterCreate(sock, CreationSession::Step::BonusStat);
}

//*********************************************************************
//                      renderPenaltyStat
//*********************************************************************

void Create::renderPenaltyStat(const std::shared_ptr<Socket>& sock) {
    sock->printColor("\nChoose your stat to lower:\n[^WA^x] Strength, [^WB^x] Dexterity, [^WC^x] Constitution, [^WD^x] Intelligence, or [^WE^x] Piety.\n");
    sock->askFor(" : ");
    enterCreate(sock, CreationSession::Step::PenaltyStat);
}

//*********************************************************************
//                      renderWeaponMenu
//*********************************************************************

namespace {
    bool isInnateWeaponMenu(std::span<const Create::Option> options) {
        return options.size() == 1 && isInnateWeapon(options.front().value);
    }

    // Print-only: the grid for an already-built option list, bucketed by group header.
    void renderWeaponMenu(const std::shared_ptr<Socket>& sock, std::span<const Create::Option> options) {
        if(isInnateWeaponMenu(options)) {
            sock->printColor("\n\n^W%s^x\n     [^WA^x] %s\n", options.front().group.c_str(),
                             options.front().label.c_str());
            return;
        }

        int k = 0, n = 0;
        std::string lastGroup;
        for(const auto& opt : options) {
            if(opt.group != lastGroup) {
                sock->bprint(fmt::format("^W\n\n{}", opt.group));
                n = 0;
                lastGroup = opt.group;
            }
            if(n++%2==0)
                sock->print("\n%5s", " ");
            sock->bprint(fmt::format("[^W{:1}^x] {:<30}", static_cast<char>(++k + 64), opt.label));
        }
    }

    // In-game weapon training: the same menu the Prof step builds, sourced from the live player.
    std::vector<Create::Option> playerWeaponOptions(const std::shared_ptr<Player>& player) {
        const auto knows = [&player](const std::string& name) { return player->knowsSkill(name); };
        return Create::weaponMenuOptions(player->getClass(), player->getSecondClass(),
                                         player->getDeity(), player->getLevel(), knows);
    }

    // Monks and werewolves train their innate skill instead; everyone else gets bare-hand
    // before the menu is built, so it never shows up as a choice.
    void ensureBareHand(const std::shared_ptr<Player>& player) {
        if(player->getClass() == CreatureClass::MONK || player->getClass() == CreatureClass::WEREWOLF)
            return;
        if(!player->knowsSkill("bare-hand"))
            player->addSkill("bare-hand", 1);
    }

    bool renderPlayerWeaponMenu(const std::shared_ptr<Socket>& sock) {
        const std::shared_ptr<Player> player = sock->getPlayer();
        ensureBareHand(player);
        const auto options = playerWeaponOptions(player);
        renderWeaponMenu(sock, options);
        return !options.empty();
    }

    bool processWeaponChoice(const std::shared_ptr<Socket>& sock, char ch) {
        const std::shared_ptr<Player> player = sock->getPlayer();
        ensureBareHand(player);
        const auto options = playerWeaponOptions(player);
        const std::size_t idx = std::isalpha(static_cast<unsigned char>(ch))
                                    ? static_cast<std::size_t>(low(ch) - 'a') : options.size();
        if(idx >= options.size()) {
            if(isInnateWeaponMenu(options))
                sock->print("Choose a weapon skill:\n");
            return false;
        }
        player->addSkill(options[idx].value, 1);
        Create::addStartingWeapon(player, options[idx].value);
        if(!isInnateWeapon(options[idx].value))
            sock->bprint(fmt::format("You have learned how to use ^W{}^x.\n", options[idx].label));
        return true;
    }
}

//*********************************************************************
//                      cmdWeapons
//*********************************************************************

int cmdWeapons(const std::shared_ptr<Player>& player, cmd* cmnd) {
    if(player->getWeaponTrains() < 1) {
        *player << "You don't have any weapon trains left!\n";
        return(0);
    }

    *player << ColorOn <<fmt::format("You have ^W{}^x weapon skills to choose.\n", player->getWeaponTrains()) << ColorOff;

    if(!renderPlayerWeaponMenu(player->getSock())) {
        *player << "\n\nSorry, couldn't find any weapon skills you don't know that you can learn.\n";
        return(0);
    }
    player->getSock()->setState(CON_CHOSING_WEAPONS);
    player->getSock()->askFor("\n(0 to exit): ");
    return(0);
}

//*********************************************************************
//                      convertNewWeaponSkills
//*********************************************************************

void convertNewWeaponSkills(const std::shared_ptr<Socket>& sock, const std::string& str) {
    if(str.empty() || !std::isalpha(static_cast<unsigned char>(str[0]))) {
        sock->setState(CON_PLAYING);
        return;
    }

    if(processWeaponChoice(sock, str[0])) {
        sock->getPlayer()->subWeaponTrains(1);
        if(sock->getPlayer()->getWeaponTrains() == 0) {
            sock->print("Exiting weapon skill selector.\n");
            sock->setState(CON_PLAYING);
            return;
        } else {
            sock->print("You have %d weapon skill(s) left to choose.", sock->getPlayer()->getWeaponTrains());
            if(!renderPlayerWeaponMenu(sock)) {
                sock->print("\n\nSorry, couldn't find any weapon skills you don't know that you can learn.\n");
                sock->setState(CON_PLAYING);
                return;
            }
            sock->askFor("\n(0 to exit): ");
        }
    } else {
        sock->print("Please pick: ");
    }
}

//*********************************************************************
//                      renderProf
//*********************************************************************

namespace {
    // Print-only half of the old addInitialClassRaceWeaponSkills: Flow::enterProf already
    // granted the skills. Returns whether the Prof menu is an "additional" pick.
    bool printGrantedSkills(const std::shared_ptr<Socket>& sock, const CharacterDraft& draft,
                            const Create::InitialSkills& skills) {
        const RaceData* rData = gConfig->getRace(draft.race);
        if(rData && skills.raceCount > 0)
            sock->printColor("^yAs race ^c%s^y, you already know the following initial weapon skill%s: ^W%s\n",
                             rData->getName().c_str(), skills.raceCount>1?"s":"", skills.raceSkills.c_str());

        if(skills.classCount > 0) {
            const std::string playerClassName = getFullClassName(draft.cClass, draft.secondClass);
            const DeityData* deity = gConfig->getDeity(draft.deity);
            if(deity && (draft.cClass==CreatureClass::CLERIC || draft.cClass==CreatureClass::PALADIN || draft.cClass==CreatureClass::DEATHKNIGHT))
                sock->printColor("^yAs a ^c%s of %s^y, you already know the following initial weapon skill%s: ^W%s\n", playerClassName.c_str(),
                                                                        deity->getName().c_str(), skills.classCount>1?"s":"", skills.classSkills.c_str());
            else
                sock->printColor("^yAs class ^c%s^y, you already know the following initial weapon skill%s: ^W%s\n",
                                                playerClassName.c_str(), skills.classCount>1?"s":"", skills.classSkills.c_str());
        }

        return skills.raceCount > 0 || skills.classCount > 0;
    }

    // The "additional" infix survives the pager's re-render, where granted is empty: at the
    // Prof step the draft holds only the auto-grants plus the implicit bare-hand.
    bool hasGrantedWeaponSkills(const CharacterDraft& draft) {
        return std::any_of(draft.weaponSkills.begin(), draft.weaponSkills.end(),
                           [](const std::string& skill) { return skill != "bare-hand"; });
    }
}

void Create::renderProf(const std::shared_ptr<Socket>& sock, const Create::InitialSkills& granted) {
    const CharacterDraft& draft = sock->getCreationDraft();
    const bool printed = printGrantedSkills(sock, draft, granted);
    const bool additionalWeapons = printed || hasGrantedWeaponSkills(draft);

    PlayerClass* pClass = Create::classForDraft(draft);
    const int profs = pClass ? pClass->numProfs() : 1;
    if(profs > 1)
        sock->print("\nPick %d %sweapon skills:", profs, additionalWeapons?"additional ":"");
    else
        sock->print("\nChoose %s weapon skill:", additionalWeapons?"an additional":"a");

    renderWeaponMenu(sock, Flow::options(draft, Step::Prof));
    sock->askFor("\n: ");
    enterCreate(sock, CreationSession::Step::Prof);
}

//*********************************************************************
//                      renderSecondProf
//*********************************************************************

void Create::renderSecondProf(const std::shared_ptr<Socket>& sock) {
    renderWeaponMenu(sock, Flow::options(sock->getCreationDraft(), Step::SecondProf));
    sock->print("\nPick a second proficiency:\n");
    sock->askFor(": ");
    enterCreate(sock, CreationSession::Step::SecondProf);
}

//*********************************************************************
//                      renderName
//*********************************************************************

void Create::renderName(const std::shared_ptr<Socket>& sock) {
    sock->print("\nYou must now choose a name for your character.\n");
    sock->print("Your character name should be appropriate for a fantasy setting.\n");
    sock->print("Names that are offensive, reference real-world people/places,\n");
    sock->print("or are otherwise inappropriate may be changed by staff.\n\n");
    sock->askFor("Please choose a character name: ");
    enterCreate(sock, CreationSession::Step::Name);
}

//*********************************************************************
//                      renderDmPassword
//*********************************************************************

void Create::renderDmPassword(const std::shared_ptr<Socket>& sock) {
    sock->print("\nYou must enter a password to create that character.\n");
    sock->echoOff();
    sock->askFor("Please enter password: ");
    enterCreate(sock, CreationSession::Step::DmPassword);
}

//*********************************************************************
//                      renderDone
//*********************************************************************

void Create::renderDone(const std::shared_ptr<Socket>& sock) {
    sock->viewFile((Path::Config / "policy_login.txt").string());

    sock->askFor("[Press Enter to Continue]");
    enterCreate(sock, CreationSession::Step::Done);
}

void Create::processDone(const std::shared_ptr<Socket>& sock, const std::string& str) {
    const std::shared_ptr<Account> account = sock->getAccount();
    if(!account) {
        sock->print("Error: No account found during character creation!\n");
        sock->reconnect();
        return;
    }

    const CharacterDraft& draft = sock->getCreationDraft();

    // The Name step already validated; guard the on-disk existence.
    if(Player::exists(draft.name)) {
        sock->printColor("\n\n^ySorry, that player already exists.^x\n\n\n");
        sock->reconnect();
        return;
    }

    // The duplicate/multi-connection checks handle their own cleanup and messaging.
    const std::shared_ptr<Player> player = Create::materialize(draft, *account);
    sock->setPlayer(player);

    if(gServer->checkDuplicateName(sock, false))
        return;
    if(gServer->checkDouble(sock))
        return;

    player->fd = sock->getFd();
    player->setSock(sock);
    player->defineColors();
    player->init();

    sock->print("\n");

    const Create::PersistResult persisted = Create::persistNewCharacter(player, *account);
    if(!persisted.ok) {
        sock->printColor("\n\n^ySorry, your character could not be saved: %s^x\n\n\n", persisted.error.c_str());
        sock->reconnect();
        return;
    }
    sock->registerPlayer();

    sock->print("Type 'welcome' at prompt to get more info on the game\nand help you get started.\n");

    sock->setState(CON_PLAYING);
}

//*********************************************************************
//                      adjustStats
//*********************************************************************

void Creature::adjustStats() {
    strength.restore();
    dexterity.restore();
    constitution.restore();
    intelligence.restore();
    piety.restore();

    strength.setInitial(strength.getCur());
    dexterity.setInitial(dexterity.getCur());
    constitution.setInitial(constitution.getCur());
    intelligence.setInitial(intelligence.getCur());
    piety.setInitial(piety.getCur());


    if(deity) {
        switch(deity) {
        case ARAMON:
        case ARACHNUS:
            alignment = (P_BOTTOM_REDDISH+P_TOP_REDDISH)/2;
            break;
        case ENOCH:
        case LINOTHAN:
        case MARA:
        case KAMIRA:
            alignment = (P_BOTTOM_BLUISH+P_TOP_BLUISH)/2;
            break;
        default:
            break;
        }
    }

    if(cClass == CreatureClass::LICH)
        alignment = (P_BOTTOM_REDDISH+P_TOP_REDDISH)/2;
}

//*********************************************************************
//                      CustomCrt
//*********************************************************************

CustomCrt::CustomCrt() {
    community = parents = brothers = sisters = social = education = height = weight = 0;
    hair = eyes = skin = "";
}

//*********************************************************************
//                      nameIsAllowed
//*********************************************************************

bool nameIsAllowed(std::string str, const std::shared_ptr<Socket>& sock) {
    const AccountAuth::NameError error = AccountAuth::checkName(str);
    if(error == AccountAuth::NameError::Ok)
        return(true);
    sock->print("%s\n", AccountAuth::nameErrorMessage(error).c_str());
    return(false);
}

//*********************************************************************
//                    handleCharacterClaim
//*********************************************************************

void handleCharacterClaim(const std::shared_ptr<Socket>& sock, std::shared_ptr<Account> account, const std::string& str) {
    std::string charName = str;
    boost::trim(charName);
    
    if(charName.empty()) {
        sock->print("You must enter a character name.\n");
        showAccountMenu(sock, account);
        return;
    }
    
    // Format the character name properly (capitalize first letter)
    lowercize(charName, 1);
    
    // Check if character exists
    std::shared_ptr<Player> player;
    if(!loadPlayer(charName, player)) {
        sock->print("Character '%s' does not exist.\n", charName.c_str());
        showAccountMenu(sock, account);
        return;
    }
    
    // Check if character is already claimed by an account
    if(!player->getAccountName().empty()) {
        sock->print("Character '%s' is already claimed by an account.\n", charName.c_str());
        sock->print("Only unclaimed legacy characters can be claimed.\n");
        showAccountMenu(sock, account);
        return;
    }
    
    // Check if we already have this character in our account somehow
    if(account->hasCharacter(charName)) {
        sock->print("Character '%s' is already in your account.\n", charName.c_str());
        showAccountMenu(sock, account);
        return;
    }
    
    // Store the character name for the password verification step
    sock->getLoginSession().targetCharacter = charName;

    sock->print("\nTo claim character '%s', you must verify ownership by entering the character's password.\n", charName.c_str());
    sock->echoOff();
    sock->askFor("Character password: ");
    enterLogin(sock, LoginSession::Step::ClaimPassword);
}
