#include <sqlite_orm/sqlite_orm.h>

#include "paths.hpp"
#include "config.hpp"
#include "accounts.hpp"

using namespace sqlite_orm;

static auto Config::initDb(const std::string &path) {
  return make_storage(
    path,

    // Note that make_index calls need to come before make_table as sync_schema evaluates these in reverse order
    // Account
    make_unique_index("idx_accounts_name", &Account::getName),
    make_table(
      "accounts",
      make_column("id", &Account::setId, &Account::getId, primary_key().autoincrement()),
      make_column("name", &Account::setName, &Account::getName),
      make_column("password", &Account::setPassword, &Account::getPassword),
      make_column("email", &Account::setEmail, &Account::getEmail)
    ),
    // Player -- only used to relate players to accounts, for now
    make_index("idx_players_account_id", &AccountPlayer::accountId),
    make_index("idx_players_name", &AccountPlayer::playerName),
    make_table(
      "players",
      make_column("id", &AccountPlayer::playerId, primary_key()),
      make_column("name", &AccountPlayer::playerName),
      make_column("account_id", &AccountPlayer::accountId),
      foreign_key(&AccountPlayer::accountId).references(&Account::getId).on_delete.cascade()
    )
  );
}
