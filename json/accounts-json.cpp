#include <filesystem>
#include <iostream>
#include <string>
#include <string_view>

#include "json.hpp"
#include "mudObjects/accounts.hpp"

using json = nlohmann::json;

void from_json(const json &j, Account &acc) {
  acc.id = j.at("id").get<std::string>();
  acc.name = j.at("name").get<std::string>();
  acc.password = j.at("password").get<std::string>();
  acc.email = j.at("email").get<std::string>();
}

void to_json(json &j, const Account &acc) {
  j = json{{
    {"id", acc.id},
    {"name", acc.name},
    {"password", acc.password},
    {"email", acc.email}
  }}
}

bool loadAccount(std::string_view name, std::shared_ptr<Account>& acc) {
  std::ifstream fileStream((Path::Account / name).replace_extension("json"));
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