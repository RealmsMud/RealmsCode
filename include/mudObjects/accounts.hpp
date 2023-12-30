#pragma once

#include <filesystem>
#include <string>
#include <boost/dynamic_bitset.hpp>

using json = nlohmann::json;

class Account {
  private:
    std::string id;
    std::string name;
    std::string password;
    std::string email;

  public:
    Account();

    friend void to_json(json &j, const Account &account);
    friend void from_json(const json &j, Account &account);
    // static bool loadFromFile(std::string_view name, std::shared_ptr<Account>& account);
    // static std::string hashPassword(const std::string &pass);

    // Getters
    std::string getId() const;
    std::string getName() const;
    std::string getPassword() const;
    std::string getEmail() const;

    // Setters
    void setId(std::string id);
    void setName(std::string name);
    void setPassword(std::string password);
    void setEmail(std::string email);
};
