#include <string>
#include <optional>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

class Account {
  protected:
    int id;
    std::string name;
    std::string password;
    std::optional<std::string> email;

  public:
    Account();

    // Getters
    int getId() const;
    std::string getName() const;
    std::string getPassword() const;
    std::optional<std::string> getEmail() const;

    // Setters
    void setId(int id);
    void setName(std::string name);
    void setPassword(std::string password);
    void setEmail(std::optional<std::string> email);
};
