#include <string>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

class Account {
  protected:
    std::string id;
    std::string name;
    std::string password;
    std::string email;

  public:
    Account();

    friend void to_json(json &j, const std::shared_ptr<Account> &acc);
    friend void from_json(const json &j, std::shared_ptr<Account> &acc);
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
