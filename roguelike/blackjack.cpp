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

void Blackjack::Hand::resolve(Blackjack::HandStatus result, Player *player) {
  switch (result) {
    case Blackjack::HandStatus::Win: {
      // win bet
      player->coins.add(bet * 2, GOLD);
    } case Blackjack::HandStatus::NaturalWin: {
      // win 1.5x bet
      player->coins.add(bet * 2.5, GOLD);
    } case Blackjack::HandStatus::Push:
      case Blackjack::HandStatus::NaturalPush: {
      // refund bet
      player->coins.add(bet, GOLD);
    } case Blackjack::HandStatus::Unresolved:
      case Blackjack::HandStatus::Loss: {
      // player gets nothing
    } 
  }
  setStatus(result);
}

bool Blackjack::Hand::canDoubleDown() {
  return cards.size() == 2;
}

bool Blackjack::Hand::canSplit() {
  return cards.size() == 2 && cards[0].rank == cards[1].rank;
}

bool Blackjack::Hand::isResolved() {
  return status != Blackjack::HandStatus::Unresolved;
}

// void Blackjack::split(Hand& hand) {
//   Hand newHand = Hand();
//   newHand.bet = hand.bet;
//   newHand.cards.push_back(hand.cards.back());
//   hand.cards.pop_back();
//   playerHands.push_back(newHand);
// }

// void Blackjack::doubleDown(Hand& hand) {

// }

void Blackjack::Hand::addCard(Card card) {
  cards.push_back(card);
  updateSum();
}

std::vector<Card> Blackjack::Hand::getCards() const {
  return cards;
}

int Blackjack::Hand::getBet() const {
  return status;
}

void Blackjack::Hand::setBet(int amount) {
  bet = amount;
}

Blackjack::HandStatus Blackjack::Hand::getStatus() const {
  return status;
}

void Blackjack::Hand::setStatus(HandStatus state) {
  status = state;
}

short Blackjack::Hand::getSum() const {
  return sum;
}

std::string Blackjack::Hand::getSumStr() const {
  return sumStr;
}

void Blackjack::Hand::updateSum() {
  int sum = 0;
  int aces = 0;

  for (Card card : cards) {
    if (card.rank == CardRank::King) sum += 10;
    else if (card.rank == CardRank::Queen) sum += 10;
    else if (card.rank == CardRank::Jack) sum += 10;
    else if (card.rank == CardRank::Ten) sum += 10;
    else if (card.rank == CardRank::Nine) sum += 9;
    else if (card.rank == CardRank::Eight) sum += 8;
    else if (card.rank == CardRank::Seven) sum += 7;
    else if (card.rank == CardRank::Six) sum += 6;
    else if (card.rank == CardRank::Five) sum += 5;
    else if (card.rank == CardRank::Four) sum += 4;
    else if (card.rank == CardRank::Three) sum += 3;
    else if (card.rank == CardRank::Two) sum += 2;
    else if (card.rank == CardRank::Ace) aces += 1; // keep track of aces
  }

  for (int i = 1; i <= aces; i++) {
    // only one ace can count as 11 before busting, all others must be 1
    // so we need only check the last ace
    if (i == aces && sum + 11 <= 21) {
      sum += 11;
    } else {
      sum += 1;
    }
  }

  if (sum > 21) {
    sumStr = std::to_string(sum) + " BUST";
  } else if (sum == 21 && cards.size() == 2) {
    sumStr = std::to_string(sum) + " NATURAL";
  } else if (aces > 0) {
    sumStr = std::to_string(sum) + " or " + std::to_string(sum - 10);
  } else {
    sumStr = std::to_string(sum);
  }
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
  for (short i = 0; i < bets.size(); i++) {
    hand = Hand();
    hand.setBet(bets[i]);
    hands.push_back(hand);
  }

  // dealer hand at the end
  hand = Hand();
  hands.push_back(hand);

  // everyone gets 2 cards, one at a time, in rotation
  short cardsToDeal = (bets.size() * 2) + 2;
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
  os << "Sum: " + game.dealerHand.getSumStr();

  os << "\n\nPlayer:\n";
  for (int i = 0; i < game.playerHands.size(); i++) {
    handPadding = 29 - (game.playerHands[i].getCards().size() * 4);
    os << std::to_string(i+1)+") " << game.playerHands[i] << std::setw(handPadding) << " ";
    os << std::left << std::setw(14) << "Sum: " + game.playerHands[i].getSumStr();
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

    switch(sock->getState()) {
        case BLACKJACK_START: {
            if (!game) {
                // set up game with new shoe
                os << "Preparing new shoe...\n\n";
                Deck shoe = Deck(decksInShoe);
                shoe.shuffle();
                player->gamblingState.bjGame = new Blackjack(shoe);
            }

            os << "Enter bet for each hand you'd like to play, separated by a space.\n";
            os << "Each bet must be at least 100 and no more than 10,000,000.\n";
            os << "You may play up to six hands at once.\n\n";
            os << "Enter Q at any time to quit.\n";
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
            

            os << "\nDealing...\n\n";
            game->deal(bets);
            os << *game;

            // check for naturals (first two cards total 21)
            bool dealerNatural = game->dealerHand.getSum() == 21;
            for (Blackjack::Hand hand : game->playerHands) {
                bool playerNatural = hand.getSum() == 21;
                
                if (playerNatural){
                    if (dealerNatural) {
                        // push, refund bet
                        hand.resolve(Blackjack::NaturalPush, player);
                    } else {
                        // player wins 1.5x
                        hand.resolve(Blackjack::NaturalWin, player);
                    }
                } else if (dealerNatural) {
                    // player loses
                    hand.resolve(Blackjack::Loss, player);
                }
            }
            sock->setState(BLACKJACK_PLAYER_TURN);
            break;
        } case BLACKJACK_PLAYER_TURN: {
            if (std::all_of(game->playerHands.begin(), game->playerHands.end(), [](Blackjack::Hand h){ return h.isResolved(); })){
                // dealer turn
                sock->setState(BLACKJACK_DEALER_TURN);
                break;
            }

            // next player hand
            for (int i = 0; i < game->playerHands.size(); i++) {
                if (game->playerHands[i].isResolved()) {
                    continue;
                }
                // found active player hand

            }
            break;
        }
    }
    sock->printColor(os.str().c_str());
    // sock->print("\n");
    // sock->print(str.c_str());
    //sock->setState(CON_PLAYING);
}
