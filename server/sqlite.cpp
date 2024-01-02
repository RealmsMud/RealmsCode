#include <sqlite_orm/sqlite_orm.h>

#include "paths.hpp"
#include "server.hpp"
#include "accounts.hpp"

using namespace sqlite_orm;

bool Server::initSqlite() {
  auto db = make_storage(
    (Path::SQL / "realms.sqlite"),
    make_table(
      "accounts",
      make_column("id", &Account::setId, &Account::getId, primary_key().autoincrement()),
      make_column("name", &Account::setName, &Account::getName),
      make_column("password", &Account::setPassword, &Account::getPassword),
      make_column("email", &Account::setEmail, &Account::getEmail)
    )
  );

  db.sync_schema(true);

  return(true);
}
