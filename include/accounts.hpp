#pragma once

#include <string>
#include <optional>
#include <map>

using PlayerName = std::string;
using PlayerId = std::string;

class Account {
  protected:
    int id;
    std::string name;
    std::string password;
    std::optional<std::string> email;
    std::map<PlayerName, PlayerId> players;

  public:
    Account();

    // Getters
    int getId() const;
    std::string getName() const;
    std::string getPassword() const;
    std::optional<std::string> getEmail() const;
    std::map<PlayerName, PlayerId> getPlayers() const;

    // Setters
    void setId(int id);
    void setName(std::string name);
    void setPassword(std::string password);
    void setEmail(std::optional<std::string> email);
    void setPlayers(std::map<PlayerName, PlayerId> players);
};

// std::ostream& operator<<(std::ostream& os, const Blackjack::Hand& hand) {
//   for (Card card : hand.getCards()) {
//     os << card << " ";
//   }
//   return os;
// }