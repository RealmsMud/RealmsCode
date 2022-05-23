/*
 * cards.cpp
 *   Cards and decks of cards
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

#include <ostream>                    // for operator<<, basic_ostream, ostring...
#include <iomanip>                    // for stream formatting
#include <algorithm>                  // for count
#include "mudObjects/players.hpp"     // Player
#include "blackjack.hpp"              // Blackjack
#include "socket.hpp"                 // Socket
#include "login.hpp"                  // connection states
#include "proto.hpp"                  // splitString

Blackjack::Hand::Hand() {
  status = Blackjack::HandStatus::Unresolved;
}

Blackjack::Hand::Hand(int betAmount) {
  status = Blackjack::HandStatus::Unresolved;
  bet = betAmount;
}

bool Blackjack::Hand::canDoubleDown() const {
  return cards.size() == 2;
}

bool Blackjack::Hand::canSplit() const {
  return cards.size() == 2 && cards[0].rank == cards[1].rank;
}

bool Blackjack::Hand::isResolved() {
  return status != Blackjack::HandStatus::Unresolved;
}

void Blackjack::Hand::addCard(Card card) {
  cards.push_back(card);
  update();
}

Card Blackjack::Hand::popCard() {
  Card card = cards.back();
  cards.pop_back();
  return card;
}

std::vector<Card> Blackjack::Hand::getCards() const {
  return cards;
}

int Blackjack::Hand::getBet() const {
  return bet;
}

void Blackjack::Hand::setBet(int amount) {
  bet = amount;
}

Blackjack::HandStatus Blackjack::Hand::getStatus() const {
  return status;
}

void Blackjack::Hand::setStatus(HandStatus state) {
  status = state;
  update();
}

int Blackjack::Hand::getSum() {
  return sum;
}

std::string Blackjack::Hand::getStatusStr() const {
  return statusStr;
}

void Blackjack::Hand::update() {
  int newSum = 0;
  int aces = 0;

  for (Card card : cards) {
    if (card.rank == CardRank::King) newSum += 10;
    else if (card.rank == CardRank::Queen) newSum += 10;
    else if (card.rank == CardRank::Jack) newSum += 10;
    else if (card.rank == CardRank::Ten) newSum += 10;
    else if (card.rank == CardRank::Nine) newSum += 9;
    else if (card.rank == CardRank::Eight) newSum += 8;
    else if (card.rank == CardRank::Seven) newSum += 7;
    else if (card.rank == CardRank::Six) newSum += 6;
    else if (card.rank == CardRank::Five) newSum += 5;
    else if (card.rank == CardRank::Four) newSum += 4;
    else if (card.rank == CardRank::Three) newSum += 3;
    else if (card.rank == CardRank::Two) newSum += 2;
    else if (card.rank == CardRank::Ace) aces += 1; // keep track of aces
  }

  for (int i = 1; i <= aces; i++) {
    // only one ace can count as 11 before busting, all others must be 1
    // so we need only check the last ace
    if (i == aces && newSum + 11 <= 21) {
      newSum += 11;
    } else {
      newSum += 1;
    }
  }

  sum = newSum;

  statusStr = std::to_string(sum);
  if (sum > 21) {
    statusStr += " BUST";
  } else if (sum == 21 && cards.size() == 2) {
    statusStr += " NATURAL";
  } else if (status == Blackjack::HandStatus::Standing) {
    statusStr += " STANDING";
  } else if (aces > 0) {
    statusStr += " or " + std::to_string(sum - 10);
  }
}

std::string Blackjack::Hand::getOptionsMenu() const {
  std::string menu = "[V] View Table, [S] Stand, [H] Hit";
  if (canSplit()) {
    menu += ", [P] Split";
  }
  if (canDoubleDown()) {
    menu += ", [D] Double Down";
  }

  menu += "\n";
  return menu;
}

Blackjack::Blackjack() {
  
}

Blackjack::Blackjack(Deck deck) {
  shoe = deck;
}

void Blackjack::deal(std::vector<int> bets) {
  std::vector<Hand> hands;
  Hand hand;

  // create hand for each player/bet
  for (int i = 0; i < bets.size(); i++) {
    hand = Hand(bets[i]);
    hands.push_back(hand);
  }

  // dealer hand at the end
  hand = Hand();
  hands.push_back(hand);

  // everyone gets 2 cards, one at a time, in rotation
  int cardsToDeal = (bets.size() * 2) + 2;
  while (cardsToDeal > 0) {
    for (auto it = std::begin(hands); it != std::end(hands); it++) {
      it->addCard(shoe.takeCard());
      cardsToDeal--;
    }
  }

  // put the hands where they belong
  dealerHand = hands.back();
  hands.pop_back();
  playerHands = hands;
}

bool Blackjack::allPlayerHandsResolved() {
  return std::all_of(playerHands.begin(), playerHands.end(), [](Blackjack::Hand h){ return h.isResolved(); });
}

std::string Blackjack::getRules() {
  std::string rules = "";
  rules += "\n\nBlackjack\n";
  rules += "----------------------------------------------------------------------------------------------------\n";
  rules += "Each hand will attempt to beat the dealer by getting up to 21 without going over.\n";
  rules += "Aces may be worth 11 or 1. Face cards are 10 and every other card is its printed value.\n";
  rules += "Dealing:\n";
  rules += "Cards will be dealt from a shoe of six decks. The shoe will be shuffled when less than\n";
  rules += "60 cards remain. Bets are placed on each hand before dealing. Each player and the dealer\n";
  rules += "is dealt one card face up. Each player is dealt a 2nd card face up, and the dealer takes\n";
  rules += "a 2nd card face down.\n";
  rules += "Naturals:\n";
  rules += "If the first two cards total 21, the hand is a \"natural\".\n";
  rules += "If the player has a natural and the dealer does not, the player wins 1.5x their bet.\n";
  rules += "If the dealer has a natural and the player does not, the player loses their bet.\n";
  rules += "If both the dealer and player have naturals, it's a \"push\" and the bet is refunded.\n";
  rules += "Play:\n";
  rules += "Each player may choose to \"hit\" taking one card at a time, as many times as they like,\n";
  rules += "to get closer to 21. When they are done hitting, they may \"stand\" and play passes to the\n";
  rules += "next player. If the player goes over 21, the hand is a \"bust\" and they lose their bet.\n";
  rules += "Split:\n";
  rules += "When the player's first two cards are a pair, they may split them into separate hands.\n";
  rules += "A 2nd hand is created with the same bet as the original. Each hand is then played separately.\n";
  rules += "Double Down:\n";
  rules += "A player may choose to \"double down\" on their first two cards. Their bet is doubled\n";
  rules += "and the hand receives one more card, then automatically stands.\n";
  rules += "Settlement:\n";
  rules += "When all player hands are standing, the dealer takes their turn. The dealer reveals their\n";
  rules += "face-down card. If the total is 16 or under, the dealer must hit until the total is 17 or more.\n";
  rules += "If the dealer has an ace and counting it as 11 would bring the sum to 17 or over, the dealer\n";
  rules += "must stand. Any player hand with a lower total than the dealer loses, and vice versa.\n\n";
  return rules;
}

std::ostream& operator<<(std::ostream& os, const Blackjack::Hand& hand) {
  for (Card card : hand.getCards()) {
    os << card << " ";
  }
  return os;
}

std::ostream& operator<<(std::ostream& os, const Blackjack& game) {
  int handPadding = 29 - (game.dealerHand.getCards().size() * 4);

  os << "\nDealer:\n   ";
  for (int i = 0; i < game.dealerHand.getCards().size(); i++) {
    // dealer's 2nd card is face down
    if (i == 1) {
      os << " [] ";
    } else {
      os << game.dealerHand.getCards()[i] << " "; 
    }
  }
  os << std::setw(handPadding) << " ";
  os << "Sum: " + game.dealerHand.getStatusStr();

  os << "\n\nPlayer:\n";
  for (int i = 0; i < game.playerHands.size(); i++) {
    handPadding = 29 - (game.playerHands[i].getCards().size() * 4);
    os << std::to_string(i+1)+") " << game.playerHands[i] << std::setw(handPadding) << " ";
    os << std::left << std::setw(17) << "Sum: " + game.playerHands[i].getStatusStr();
    os << "Bet: " << std::right << std::setw(11) << game.playerHands[i].getBet() << "\n";
  }
  os << "\n";
  return os;
}

void playBlackjack(Socket* sock, const std::string& str) {
  std::ostringstream os;
  short decksInShoe = 6;
  Player *player = sock->getPlayer();
  Blackjack *game = sock->getPlayer()->gamblingState.bjGame;

  if (strncasecmp(str.c_str(), "Q", 1) == 0) {
    os << "Aborted.\n";
    sock->setState(CON_PLAYING);
  }
  if (strncasecmp(str.c_str(), "R", 1) == 0) {
    os << Blackjack::getRules();
    sock->printColor(os.str().c_str());
    return;
  }

  switch(sock->getState()) {
    case BLACKJACK_START: {
      if (!game) {
        // set up game with new shoe
        os << "Preparing new shoe...\n\n";
        Deck shoe = Deck(decksInShoe);
        shoe.shuffle();
        game = new Blackjack(shoe);
      } else {
        os << "Continuing with current shoe...\n\n";
      }

      if (game->shoe.cards.size() < 60) {
        os << "Too few cards left in shoe, reshuffling...\n";
        game->shoe.shuffle();
      }

      os << "Enter bet for each hand you'd like to play, separated by a space.\n";
      os << "Each bet must be at least 100 and no more than 10,000,000.\n";
      os << "You may play up to six hands at once.\n\n";
      os << "WARNING: Ending the game before hand resolution will forfeit your bet.\n";
      sock->setState(BLACKJACK_DEAL);
      break;
    } case BLACKJACK_DEAL: {
      std::vector<std::string> inputArgs = splitString(str, " ");
      std::vector<int> bets;
      std::transform(inputArgs.begin(), inputArgs.end(), std::back_inserter(bets), [](std::string s) { return std::stoi(s); });

      if (bets.size() < 1) {
        os << "Must play at least one hand.\n";
        break;
      }
      if (bets.size() > 6) {
        os << "Can play at most six hands at once.\n";
        break;
      }
      if (std::any_of(bets.begin(), bets.end(), [](int i){ return i < 100 || i > 10000000; })){
        os << "No bets < 100 or > 10,000,000.\n";
        break;
      }

      // take bets before dealing to prevent abuse
      int betSum = 0;
      for (int bet : bets) {
        betSum += bet;
      }
      if (betSum > player->coins[GOLD]) {
        os << "You don't have enough money to satisfy those bets!\n";
        break;
      }
      player->coins.sub(betSum, GOLD);

      os << "\nDealing...\n";
      game->deal(bets);
      os << *game;

      // check for naturals (first two cards total 21) and settle right away
      bool dealerNatural = game->dealerHand.getSum() == 21;
      if (dealerNatural) {
        os << "The dealer got a natural!\n";
      }

      int firstHandIdx = -1;
      for (int i = 0; i < game->playerHands.size(); i++) {
        bool playerNatural = game->playerHands[i].getSum() == 21;
        
        if (playerNatural){
          int net;
          if (dealerNatural) {
            // push, refund bet
            net = game->playerHands[i].getBet();
            game->playerHands[i].setStatus(Blackjack::NaturalPush);
            os << "Hand "+std::to_string(i+1)+" got a natural! Push.\n";
            player->coins.add(net, GOLD);
            os << "You were refunded your bet of $"+std::to_string(net)+".\n";
          } else {
            // player wins 1.5x
            net = game->playerHands[i].getBet() * 2.5;
            game->playerHands[i].setStatus(Blackjack::NaturalWin);
            os << "Hand "+std::to_string(i+1)+" got a natural!\n";
            player->coins.add(net, GOLD);
            os << "You won 1.5x your bet: $"+std::to_string(net - game->playerHands[i].getBet())+".\n";
          }
        } else if (dealerNatural) {
          // player loses
          game->playerHands[i].setStatus(Blackjack::Loss);
          os << "Hand "+std::to_string(i+1)+" loses its bet of $"+std::to_string(game->playerHands[i].getBet())+"\n";
        }

        if (firstHandIdx == -1 && !game->playerHands[i].isResolved()) {
          firstHandIdx = i;
        }
      }

      os << "All hands dealt! Press [enter] to continue.\n";

      if (dealerNatural) {
        // if dealer got a natural all hands should be resolved
        sock->setState(BLACKJACK_END);
      } else {
        os << "\nChoose action for hand "+std::to_string(firstHandIdx+1)+":\n" << game->playerHands[firstHandIdx] << " Sum: " + game->playerHands[firstHandIdx].getStatusStr() + "\n";
        os << game->playerHands[firstHandIdx].getOptionsMenu();
        sock->setState(BLACKJACK_PLAY);
      }
      break;
    } case BLACKJACK_PLAY: {
      // next player hand
      for (int i = 0; i < game->playerHands.size(); i++) {
        if (game->playerHands[i].isResolved()) {
          continue;
        }

        std::string handNumber = std::to_string(i+1);
        Blackjack::Hand &hand = game->playerHands[i];
        
        // Options: [V] View Table, [S] Stand, [H] Hit, [P] Split, [D] Double Down
        if (strncasecmp(str.c_str(), "V", 1) == 0) {
          // View table
          os << *game;
        } else if (strncasecmp(str.c_str(), "S", 1) == 0) {
          // Stand
          os << "Hand "+handNumber+" stands!\n";
          hand.setStatus(Blackjack::Standing);
        } else if (strncasecmp(str.c_str(), "H", 1) == 0) {
          // Hit
          os << "Hand "+handNumber+" hits!\n";
          Card card = game->shoe.takeCard();
          hand.addCard(card);
          if (hand.getSum() > 21) {
            hand.setStatus(Blackjack::Loss);
          } else if (hand.getSum() == 21) {
            hand.setStatus(Blackjack::Standing);
          }
        } else if (strncasecmp(str.c_str(), "P", 1) == 0) {
          // Split
          if (hand.canSplit()) {
            os << "Hand "+handNumber+" splits!\n";
            // separate the pair into two hands with equal bet
            Blackjack::Hand newHand = Blackjack::Hand(hand.getBet());
            newHand.addCard(hand.popCard());
            game->playerHands.insert(game->playerHands.begin() + i + 1, newHand);
          }
        } else if (strncasecmp(str.c_str(), "D", 1) == 0) {
          // Double Down
          if (hand.canDoubleDown()) {
            os << "Hand "+handNumber+" doubles down!\n";
            // double the bet and receive only one more card
            int newBet = hand.getBet() * 2;
            hand.setBet(newBet);
            hand.addCard(game->shoe.takeCard());
            hand.setStatus(Blackjack::Standing);
          }
        }

        // if hand is now resolved, display it once more
        if (hand.isResolved()) {
          os << handNumber+") " << game->playerHands[i] << " Sum: " + game->playerHands[i].getStatusStr() + "\n";
        }
        break;
      }

      // check for next prompt or progress to next phase
      bool isDealersTurn = true;
      for (int i = 0; i < game->playerHands.size(); i++) {
        if (game->playerHands[i].isResolved()) {
          continue;
        }
        isDealersTurn = false;
        os << "\nChoose action for hand "+std::to_string(i+1)+":\n" << game->playerHands[i] << " Sum: " + game->playerHands[i].getStatusStr() + "\n";
        os << game->playerHands[i].getOptionsMenu();
        break;
      }
      if (isDealersTurn) {
        os << "\nAll hands standing, dealer's turn...\nPress [enter] to continue.\n\n";
        sock->setState(BLACKJACK_DEALER_TURN);
      }
      break;
    } case BLACKJACK_DEALER_TURN: {
      os << game->dealerHand << " Sum: "+game->dealerHand.getStatusStr()+"\n";
      while (game->dealerHand.getSum() < 17) {
        os << "Dealer hits.\n";
        game->dealerHand.addCard(game->shoe.takeCard());
        os << game->dealerHand << " Sum: "+game->dealerHand.getStatusStr()+"\n";
      }

      int dealerSum = game->dealerHand.getSum();
      if (dealerSum > 21) {
        os << "Dealer busted!\n";
      } else {
        os << "Dealer stands.\n";
      }

      os << *game;
      os << "Settling bets...\n";

      for (int i = 0; i < game->playerHands.size(); i++) {
        // only standing hands are unsettled at this point
        if (game->playerHands[i].getStatus() != Blackjack::Standing) {
          continue;
        }

        int sum = game->playerHands[i].getSum();
        int bet = game->playerHands[i].getBet();
        if (sum > dealerSum) {
          // player win
          os << "Hand "+std::to_string(i+1)+" wins its bet of $"+std::to_string(bet)+"!\n";
          player->coins.add(bet * 2, GOLD);
        } else if (sum < dealerSum) {
          // player lose
          os << "Hand "+std::to_string(i+1)+" loses its bet of $"+std::to_string(bet)+"!\n";
        } else if (sum == dealerSum) {
          // push
          os << "Hand "+std::to_string(i+1)+" pushes! Its bet of $"+std::to_string(bet)+" has been refunded.\n";
          player->coins.add(bet, GOLD);
        }
      }

      os << "\nGood game! Would you like to play again?\n";
      os << "[Y] Yes  [N] No\n";
      sock->setState(BLACKJACK_END);
      break;
    } case BLACKJACK_END: {
      if (strncasecmp(str.c_str(), "Y", 1) == 0) {
        os << "Press [enter] to continue.\n";
        sock->setState(BLACKJACK_START);
      } else if (strncasecmp(str.c_str(), "N", 1) == 0) {
        os << "Goodbye!\n";
        sock->setState(CON_PLAYING);
      }
      break;
    }
  }
  player->gamblingState.bjGame = game;
  sock->printColor(os.str().c_str());
}
