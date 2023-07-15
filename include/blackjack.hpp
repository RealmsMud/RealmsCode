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
    enum HandStatus {
      Unresolved,
      Standing,
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
        int sum;
        std::string statusStr;
        HandStatus status;
      public:
        Hand();
        Hand(int betAmount);

        bool canDoubleDown() const;
        bool canSplit() const;
        bool isResolved();

        void update();
        int getSum();
        std::string getStatusStr() const;

        std::vector<Card> getCards() const;
        void addCard(Card card);
        Card popCard();

        HandStatus getStatus() const;
        void setStatus(HandStatus state);

        int getBet() const;
        void setBet(int amount);

        std::string getOptionsMenu() const;
    };

    Deck shoe;
    Hand dealerHand;
    std::vector<Hand> playerHands;

    Blackjack();
    Blackjack(Deck shoe);

    static std::string getRules();

    void deal(std::vector<int> bets);

    const bool allPlayerHandsResolved() const;

    friend std::ostream& operator<<(std::ostream& os, const Blackjack& game);
};

void playBlackjack(std::shared_ptr<Socket> sock, const std::string& str);

#endif