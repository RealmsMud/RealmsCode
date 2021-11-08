/*
 * sql.cpp
 *   Code to handle SQL queries
 *   ____            _
 *  |  _ \ ___  __ _| |_ __ ___  ___
 *  | |_) / _ \/ _` | | '_ ` _ \/ __|
 *  |  _ <  __/ (_| | | | | | | \__ \
 *  |_| \_\___|\__,_|_|_| |_| |_|___/
 *
 * Permission to use, modify and distribute is granted via the
 *  GNU Affero General Public License v3 or later
 *
 *  Copyright (C) 2007-2021 Jason Mitchell, Randi Mitchell
 *     Contributions by Tim Callahan, Jonathan Hseu
 *  Based on Mordor (C) Brooke Paul, Brett J. Vickers, John P. Freeman
 *
 */

#ifdef SQL_LOGGER

#include <sstream>

#include <odbc++/drivermanager.h>
#include <odbc++/connection.h>
#include <odbc++/resultset.h>
#include <odbc++/resultsetmetadata.h>
#include <odbc++/preparedstatement.h>

#include "config.hpp"
#include "mud.hpp"
#include "server.hpp"

//################################################################################
//#    Config::getDbConnectionStriong()
//################################################################################

// Returns the connection string needed to connect to the database specified
// by the config

std::string Config::getDbConnectionString() {
    std::ostringstream connStr;
    if(logDbType == "mysql") {
       connStr << "Driver=MySQL;";
    } else if (logDbType == "postgres" || logDbType == "postgresql") {
        // Untested
        connStr << "Driver=PostgreSQL ANSI;";
    }
    connStr << "Server=localhost;";
    connStr << "Database=" << logDbDatabase << ";";

    if(logDbType == "mysql") {
        connStr << "Port=3306;";
        connStr << "USER=" << this->logDbUser << ";";
        connStr << "PASSWORD=" << logDbPass;
    } else if (logDbType == "postgres" || logDbType == "postgresql") {
        // Untested
        connStr << "Port=5432;";
        connStr << "UserName=" << this->logDbUser << ";";
        connStr << "Password=" << logDbPass;
    }
    return(connStr.str());
}

//################################################################################
//#    Server::initSql()
//################################################################################

bool Server::initSql() {
    if(connActive) {
        return(false);
    }

    try {
        odbc::DriverManager::setLoginTimeout(0);
        conn = odbc::DriverManager::getConnection(gConfig->getDbConnectionString());
        connActive = true;

    } catch(odbc::SQLException& e) {
        std::cerr << e.getMessage() << std::endl;
        return(false);
    }
    return(true);
}

void Server::cleanUpSql() {
    if(!connActive)
        return;
    std::clog << "Cleaning up SQL." << std::endl;

    delete conn;
    odbc::DriverManager::shutdown();
    connActive = false;
}

bool Server::logGoldSql(std::string& pName, std::string& pId, std::string& targetStr, std::string& source, std::string& room,
                        std::string& logType, unsigned long amt, std::string& direction)
{
    if (!connActive)
        return (false);

    try {
        odbc::PreparedStatement* stmt = conn->prepareStatement("INSERT INTO goldlog(PlayerName,PlayerID,Target,"
            "Source,Room,LogType,Gold,Direction) VALUES (?,?,?,?,?,?,?,?)");

        stmt->setString(1, pName);
        stmt->setString(2, pId);
        stmt->setString(3, targetStr);
        stmt->setString(4, source);
        stmt->setString(5, room);
        stmt->setString(6, logType);
        stmt->setLong(7, amt);
        stmt->setString(8, direction);
        stmt->executeUpdate();

        delete stmt;
        //conn->unregisterStatement(stmt);

        return(true);
    } catch (odbc::SQLException &e ) {

        std::cerr << e.getMessage() << std::endl;
        broadcast(isDm, "%s", e.getMessage().c_str());
        cleanUpSql();
        return(false);
    }
}

bool Server::getConnStatus() {
    return(connActive);
}

int Server::getConnTimeout() {
    return(odbc::DriverManager::getLoginTimeout());
}

#endif // SQL_LOGGER
