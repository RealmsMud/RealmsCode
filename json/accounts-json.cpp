#include <fstream>
#include <iostream>
#include <string>

#include "accounts.hpp"
#include "paths.hpp"

using json = nlohmann::json;

void from_json(const json &j, std::shared_ptr<Account> &acc) {
  acc->setId(j.at("id").get<std::string>());
  acc->setName(j.at("name").get<std::string>());
  acc->setPassword(j.at("password").get<std::string>());
  acc->setEmail(j.at("email").get<std::string>());
  // j.at("id").get_to(acc.id);
  // j.at("name").get_to(acc.name);
  // j.at("password").get_to(acc.password);
  // j.at("email").get_to(acc.email);
}

void to_json(json &j, const std::shared_ptr<Account> &acc) {
  j = json{
    {"id", acc->getId()},
    {"name", acc->getName()},
    {"password", acc->getPassword()},
    {"email", acc->getEmail()}
  };
}

bool loadAccount(std::string name, std::shared_ptr<Account> &acc) {
  std::clog << "\n" << "loadAccount 1\n";
  // fs::path file = (Path::Account / name).replace_extension("json");
  // if(fs::exists(file)) {

  // }
  json j;
  std::ifstream fileStream((Path::Account / name).replace_extension("json"));
  std::clog << "\n" << "loadAccount 2\n";

  fileStream >> j;
  std::clog << "\n" << "loadAccount 3\n";
  from_json(j, acc);
  std::clog << "\n" << "loadAccount 4\n";

  return true;
}

// bool loadPlayer(std::string_view name, std::shared_ptr<Player>& player, enum LoadType loadType) {
//     xmlDocPtr   xmlDoc;
//     xmlNodePtr  rootNode;
//     fs::path    filename;
//     std::string     pass, loadName;

//     if(loadType == LoadType::LS_BACKUP)
//         filename = (Path::PlayerBackup / name).replace_extension("bak.xml");
//     else if(loadType == LoadType::LS_CONVERT)
//         filename = (Path::Player / "convert" / name).replace_extension("xml");
//     else // LoadType::LS_NORMAL
//         filename = (Path::Player / name).replace_extension("xml");

//     if((xmlDoc = xml::loadFile(filename.c_str(), "Player")) == nullptr)
//         return(false);

//     rootNode = xmlDocGetRootElement(xmlDoc);
//     loadName = xml::getProp(rootNode, "Name");
//     if(loadName != name) {
//         std::clog << "Error loading " << name << ", found " << loadName << " instead!\n";
//         xmlFreeDoc(xmlDoc);
//         xmlCleanupParser();
//         return(false);
//     }

//     player = std::make_shared<Player>();

//     (player)->setName(xml::getProp(rootNode, "Name"));
//     xml::copyPropToString(pass, rootNode, "Password");
//     (player)->setPassword(pass);
//     (player)->setLastLogin(xml::getIntProp(rootNode, "LastLogin"));

//     // If we get here, we should be loading the correct player, so start reading them in
//     (player)->readFromXml(rootNode);

//     xmlFreeDoc(xmlDoc);
//     xmlCleanupParser();
//     return(true);
// }