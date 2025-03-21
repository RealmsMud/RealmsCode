#pragma once

#include <sqlite_orm/sqlite_orm.h>

#include "paths.hpp"
#include "accounts.hpp"

namespace SQL {
  // Connect SQL accounts to XML Player models
  struct Player {
    int id;
    int accountId;
    std::string xmlId;
    std::string name;
  };

  inline auto init(const std::string &fileName) {
    using namespace sqlite_orm;
    fs::create_directory(Path::SQL);

    return make_storage(
      Path::SQL / fileName,

      // Note that make_index calls need to come before make_table as sync_schema evaluates these in reverse order
      // Account
      make_table(
        "account",
        make_column("id", &Account::setId, &Account::getId, primary_key().autoincrement()),
        make_column("name", &Account::setName, &Account::getName, unique()),
        make_column("password", &Account::setPassword, &Account::getPassword),
        make_column("email", &Account::setEmail, &Account::getEmail)
      ),
      // Player -- only used to relate players to accounts for now
      make_index("idx_player_account_id", &Player::accountId),
      make_table(
        "player",
        make_column("id", &Player::id, primary_key().autoincrement()),
        make_column("account_id", &Player::accountId),
        make_column("xml_id", &Player::xmlId, unique()),
        make_column("name", &Player::name, unique()),
        foreign_key(&Player::accountId).references(&Account::getId).on_delete.cascade()
      )
    );
  }
}

using Database = decltype(SQL::init(""));

extern Database database;
