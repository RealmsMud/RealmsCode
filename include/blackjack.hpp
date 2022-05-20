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

class Player;

class Blackjack {
  public:
    enum HandStatus {
      Unresolved,
      Loss,
      Push,
      Win,
      NaturalPush,
      NaturalWin
    };

    class Hand {
      private:
        std::vector<Card> cards;
        int bet;
        short sum;
        std::string sumStr;
        HandStatus status;
      public:
        Hand();

        void resolve(HandStatus result, Player *player);

        bool canDoubleDown();
        bool canSplit();
        bool isResolved();

        HandStatus getStatus() const;
        void setStatus(HandStatus state);

        int getBet() const;
        void setBet(int amount);

        std::vector<Card> getCards() const;
        void addCard(Card card);

        void updateSum();
        short getSum() const;
        std::string getSumStr() const;
    };

    Deck shoe;
    Hand dealerHand;
    std::vector<Hand> playerHands;

    Blackjack();
    Blackjack(Deck shoe);

    void deal(std::vector<int> bets);

    friend std::ostream& operator<<(std::ostream& os, const Blackjack& game);
};

void playBlackjack(Socket* sock, const std::string& str);

#endif