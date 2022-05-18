/*
 * blackjack.h
 *   Blackjack game
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
#ifndef BLACKJACK_H
#define BLACKJACK_H

#include <cards.hpp>              // Card, Deck

class Blackjack {
  public:
    struct Hand {
      std::vector<Card> cards;
      int bet;
    };

    Deck shoe;
    Hand dealerHand;
    std::vector<Hand> playerHands;

    Blackjack();
    Blackjack(Deck shoe);

    void deal(std::vector<int> bets);
    void placeBet(Hand& hand, int amount);
    void hit(Hand& hand);
    void split(Hand& hand);
    void doubleDown(Hand& hand);

    static std::string getHandStatusStr(Hand& hand);

    friend std::ostream& operator<<(std::ostream& os, const Blackjack& game);
};

#endif