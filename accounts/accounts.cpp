#include <optional>
#include "database.hpp"
#include "accounts.hpp"

Account::Account() {}

Account::Account(std::string name_) :
  id(0),
  name(name_) {}

// Getters
int Account::getId() const { return id; }
std::string Account::getName() const { return name; }
std::string Account::getPassword() const { return password; }
std::optional<std::string> Account::getEmail() const { return email; }
std::map<PlayerName, PlayerId> Account::getPlayers() const { return players; }

// Setters
void Account::setId(int id_) { this->id = id_; }
void Account::setName(std::string name_) { this->name = name_; }
void Account::setPassword(std::string password_) { this->password = password_; }
void Account::setEmail(std::optional<std::string> email_) { this->email = email_; }
void Account::setPlayers(std::map<PlayerName, PlayerId> players_) { this->players = players_; }

// Database
int Account::save() {
  using namespace sqlite_orm;

  return database.insert(
    *this, on_conflict(
      update(
        set(
          c(&Account::name) = excluded(&Account::name),
          c(&Account::password) = excluded(&Account::password),
          c(&Account::email) = excluded(&Account::email)
        )
      )
    )
  );
}