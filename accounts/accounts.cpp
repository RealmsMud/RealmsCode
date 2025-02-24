#include <optional>
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
