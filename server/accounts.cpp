#include "mudObjects/accounts.hpp"

Account::Account() {}

// Getters
std::string Account::getId() const { return id; }
std::string Account::getName() const { return name; }
std::string Account::getPassword() const { return password; }
std::string Account::getEmail() const { return email; }

// Setters
void Account::setId(std::string id) { this->id = id; }
void Account::setName(std::string name) { this->name = name; }
void Account::setPassword(std::string password) { this->password = password; }
void Account::setEmail(std::string email) { this->email = email; }
