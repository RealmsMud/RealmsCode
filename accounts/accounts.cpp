#include <optional>
#include "accounts.hpp"

Account::Account() {}

// Getters
int Account::getId() const { return id; }
std::string Account::getName() const { return name; }
std::string Account::getPassword() const { return password; }
std::optional<std::string> Account::getEmail() const { return email; }
std::map<PlayerName, AccountPlayer> Account::getPlayers() const { return players; }

// Setters
void Account::setId(int id) { this->id = id; }
void Account::setName(std::string name) { this->name = name; }
void Account::setPassword(std::string password) { this->password = password; }
void Account::setEmail(std::optional<std::string> email) { this->email = email; }
void Account::setPlayers(std::map<PlayerName, AccountPlayer> players) { this->players = players; }
