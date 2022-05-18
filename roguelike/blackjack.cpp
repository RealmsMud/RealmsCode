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

#include <blackjack.hpp>              // Blackjack
#include <ostream>                    // for operator<<, basic_ostream, ostring...
#include <iomanip>                    // for stream formatting

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
    placeBet(hand, bets[i]);
    hands.push_back(hand);
  }

  // dealer hand at the end
  hand = Hand();
  hands.push_back(hand);

  // everyone gets 2 cards, one at a time, in rotation
  short cardsToDeal = (bets.size() * 2) + 2;
  while (cardsToDeal > 0) {
    for (auto it = std::begin(hands); it != std::end(hands); it++) {
      it->cards.push_back(shoe.takeCard());
      cardsToDeal--;
    }
  }

  // put the hands where they belong
  dealerHand = hands.back();
  hands.pop_back();
  playerHands = hands;
}

void Blackjack::placeBet(Hand& hand, int amount) {
  hand.bet = amount;
}

void Blackjack::hit(Hand& hand) {
  hand.cards.push_back(shoe.takeCard());
}

void Blackjack::split(Hand& hand) {
  Hand newHand = Hand();
  newHand.bet = hand.bet;
  newHand.cards.push_back(hand.cards.back());
  hand.cards.pop_back();
  playerHands.push_back(newHand);
}

void Blackjack::doubleDown(Hand& hand) {

}

std::string Blackjack::getHandStatusStr(Hand& hand) {
  int sum = 0;
  int aces = 0;

  for (Card card : hand.cards) {
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
    else if (card.rank == CardRank::Ace) aces += 1;
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

  std::string statusStr;

  if (sum > 21) {
    statusStr = std::to_string(sum) + " BUST";
  } else if (aces > 0) {
    statusStr = std::to_string(sum) + " or " + std::to_string(sum - 10);
  } else {
    statusStr = std::to_string(sum);
  }

  return statusStr;
}

std::ostream& operator<<(std::ostream& os, const Blackjack::Hand& hand) {
  for (Card card : hand.cards) {
    os << card << " ";
  }
  return os;
}

std::ostream& operator<<(std::ostream& os, const Blackjack& game) {
  os << "\nDealer:\n";
  for (int i = 0; i < game.dealerHand.cards.size(); i++) {
    // dealer's 2nd card is face down
    if (i == 1) {
      os << " [] ";
    } else {
      os << game.dealerHand.cards[i] << " "; 
    }
  }

  os << "\n\nPlayer:\n";
  for (Blackjack::Hand hand : game.playerHands) {
    int handPadding = 25 - (hand.cards.size() * 4);
    os << hand << std::setw(handPadding) << " ";
    os << std::left << std::setw(16) << "Status: " + Blackjack::getHandStatusStr(hand);
    os << " Bet: " << std::right << std::setw(11) << hand.bet << "\n";
  }
  os << "\n";
  return os;
}