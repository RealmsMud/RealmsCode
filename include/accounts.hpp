#include <string>
#include <optional>
#include <map>

using AccountId = int;
using PlayerId = std::string;
using PlayerName = std::string;

struct AccountPlayer {
  AccountId accountId;
  PlayerId playerId;
  PlayerName playerName;
};

class Account {
  protected:
    AccountId id;
    std::string name;
    std::string password;
    std::optional<std::string> email;
    std::map<PlayerName, AccountPlayer> players;

  public:
    Account();

    // Getters
    int getId() const;
    std::string getName() const;
    std::string getPassword() const;
    std::optional<std::string> getEmail() const;
    std::map<PlayerName, AccountPlayer> getPlayers() const;

    // Setters
    void setId(int id);
    void setName(std::string name);
    void setPassword(std::string password);
    void setEmail(std::optional<std::string> email);
    void setPlayers(std::map<PlayerName, AccountPlayer> players);
};

// std::ostream& operator<<(std::ostream& os, const Blackjack::Hand& hand) {
//   for (Card card : hand.getCards()) {
//     os << card << " ";
//   }
//   return os;
// }