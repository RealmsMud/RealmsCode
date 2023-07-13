/*
 * rogues.h
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
#ifndef CARDS_H
#define CARDS_H

#include <vector>
#include <string>

enum class CardSuit : unsigned char {
  Spades,
  Clubs,
  Hearts,
  Diamonds
};

enum class CardRank : unsigned char {
  Ace,
  Two,
  Three,
  Four,
  Five,
  Six,
  Seven,
  Eight,
  Nine,
  Ten,
  Jack,
  Queen,
  King
};

struct Card {
  CardSuit suit;
  CardRank rank;
};

std::ostream& operator<<(std::ostream& os, const Card& card);
std::string cardToString(Card card);

class Deck {
  public:
    std::vector<Card> cards;

    Deck();
    Deck(int decks); // a deck can also be a shoe of multiple decks

    void shuffle();
    Card takeCard();
};

#endif