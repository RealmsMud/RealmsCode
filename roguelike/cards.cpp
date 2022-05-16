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

#include <ostream>                // for operator<<, basic_ostream, ostring...
#include <cards.hpp>              // Card, Deck
#include <algorithm>              // for shuffle
#include <random>                 // for random_engine
#include <chrono>                 // std::chrono::system_clock

// template <typename CharT, typename Traits>
// auto operator<<(std::basic_ostream<CharT, Traits>& out, CardSuit s) -> std::basic_ostream<CharT, Traits>& {
//   switch (s) {
//     case CardSuit::Spades:
//       out << "spades";
//       break;
//     case CardSuit::Clubs:
//       out << "clubs";
//       break;
//     case CardSuit::Hearts:
//       out << "hearts";
//       break;
//     case CardSuit::Diamonds:
//       out << "diamonds";
//       break;
//   }
//   return out;
// }

// constexpr auto toValue(CardRank r) noexcept -> int {
//   switch (r) {
//     case CardRank::Jack:
//       [[fallthrough]];
//     case CardRank::Queen:
//       [[fallthrough]];
//     case CardRank::King:
//       return 10;
//     default:
//       return static_cast<int>(r);
//   }
// }

// template <typename CharT, typename Traits>
// auto operator<<(std::basic_ostream<CharT, Traits>& out, Card c) -> std::basic_ostream<CharT, Traits>& {
//   if (out) {
//     auto oss = std::basic_ostringstream<CharT, Traits>{};
//     oss << c.rank << " of " << c.suit;

//     out << oss.view();
//   }
//   return out;
// }

std::string cardToString(Card card) {
  std::string out;
  
  if (card.rank == CardRank::Ace) out = "Ace ";
  else if (card.rank == CardRank::Two) out = "Two ";
  else if (card.rank == CardRank::Three) out = "Three ";
  else if (card.rank == CardRank::Four) out = "Four ";
  else if (card.rank == CardRank::Five) out = "Five ";
  else if (card.rank == CardRank::Six) out = "Six ";
  else if (card.rank == CardRank::Seven) out = "Seven ";
  else if (card.rank == CardRank::Eight) out = "Eight ";
  else if (card.rank == CardRank::Nine) out = "Nine ";
  else if (card.rank == CardRank::Ten) out = "Ten ";
  else if (card.rank == CardRank::Jack) out = "Jack ";
  else if (card.rank == CardRank::Queen) out = "Queen ";
  else if (card.rank == CardRank::King) out = "King ";

  if (card.suit == CardSuit::Spades) out += "Spades";
  else if (card.suit == CardSuit::Clubs) out += "Clubs";
  else if (card.suit == CardSuit::Hearts) out += "Hearts";
  else if (card.suit == CardSuit::Diamonds) out += "Diamonds";

  return out;
}

Deck::Deck(int decks) {
  Card card;
  // a deck may also be a shoe of n decks
  for (int i = 0; i < decks; i++) {
    for (int suit = 0; suit < 4; suit++) {
      if (suit==0) card.suit = CardSuit::Spades;
      else if (suit==1) card.suit = CardSuit::Clubs;
      else if (suit==2) card.suit = CardSuit::Hearts;
      else if (suit==3) card.suit = CardSuit::Diamonds;

      for (int rank = 0; rank < 13; rank++) {
        if (rank==0) card.rank = CardRank::Ace;
        else if (rank==1) card.rank = CardRank::Two;
        else if (rank==2) card.rank = CardRank::Three;
        else if (rank==3) card.rank = CardRank::Four;
        else if (rank==4) card.rank = CardRank::Five;
        else if (rank==5) card.rank = CardRank::Six;
        else if (rank==6) card.rank = CardRank::Seven;
        else if (rank==7) card.rank = CardRank::Eight;
        else if (rank==8) card.rank = CardRank::Nine;
        else if (rank==9) card.rank = CardRank::Ten;
        else if (rank==10) card.rank = CardRank::Jack;
        else if (rank==11) card.rank = CardRank::Queen;
        else if (rank==12) card.rank = CardRank::King;

        cards.push_back(card);
      }
    }
  }
}

void Deck::shuffle() {
  unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
  std::shuffle(std::begin(cards), std::end(cards), std::default_random_engine(seed));
}

Card Deck::takeCard() {
  Card card = cards.back();
  cards.pop_back();
  return card;
}