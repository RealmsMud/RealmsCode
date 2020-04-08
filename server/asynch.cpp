/*
 * asynch.cpp
 *   Asynchronous communication
 *   ____            _
 *  |  _ \ ___  __ _| |_ __ ___  ___
 *  | |_) / _ \/ _` | | '_ ` _ \/ __|
 *  |  _ <  __/ (_| | | | | | | \__ \
 *  |_| \_\___|\__,_|_|_| |_| |_|___/
 *
 * Permission to use, modify and distribute is granted via the
 *  GNU Affero General Public License v3 or later
 *
 *  Copyright (C) 2007-2018 Jason Mitchell, Randi Mitchell
 *     Contributions by Tim Callahan, Jonathan Hseu
 *  Based on Mordor (C) Brooke Paul, Brett J. Vickers, John P. Freeman
 *
 */
#include "asynch.hpp"
#include "creatures.hpp"
#include "mud.hpp"
#include "server.hpp"
#include "socket.hpp"

//*********************************************************************
//                      Async
//*********************************************************************
// Sample code:
//
//  Async async;
//  if(async.branch(player, CHILD_PRINT) == AsyncExternal) {
//      bstring output = somethingArduous();
//      printf("%s", output.c_str());
//      exit(0);
//  } else {
//      player->print("Doing something long and arduous.\n");
//  }
//
// Responses to asychronous communication are handled in Server::reapChildren().
// Create your own CHILD_XYZ type if you want to perform special handling. The
// CHILD_PRINT type simply prints the response back to the player.

Async::Async() {
}

//*********************************************************************
//                      branch
//*********************************************************************

AsyncResult Async::branch(const Player* player, childType type) {
    bstring user = (player ? player->getName() : "Someone");
    if(pipe(fds) == -1) {
        std::clog << "Error with pipe!\n";
        abort();
    }

    int pid = fork();
    if(!pid) {
        // this is the child process
        // Close the reading end, we'll only be writing
        close(fds[0]);
        // Remap stdout to fds[1] so cout will print to fds[1] and we can
        // read it in from the mud
        if(dup2(fds[1], STDOUT_FILENO) != STDOUT_FILENO) {
            std::clog << "Error with dup2.\n";
            abort();
        }

        return(AsyncExternal);
    } else {
        // this is the parent process
        // Close the writing end, we'll only be reading
        close(fds[1]);
        nonBlock(fds[0]);

        std::clog << "Watching Child " << (int)type << " for (" << user << ") running with pid " << pid << " reading from fd " << fds[0] << ".";

        // Let the server know we're monitoring this child process
        gServer->addChild(pid, type, fds[0], user);

        return(AsyncLocal);
    }
}


//********************************************************************
//                      runList
//********************************************************************

int Server::runList(Socket* sock, cmd* cmnd) {
    Async async;
    if(async.branch(sock->getPlayer(), CHILD_LISTER) == AsyncExternal) {
        bstring lister = Path::UniqueRoom;
        lister += "List";
        std::clog << "Running <" << lister << ">\n";

        execl(lister.c_str(), lister.c_str(), cmnd->str[1], cmnd->str[2], cmnd->str[3], cmnd->str[4], nullptr);
        std::clog << "Error!";

        exit(0);
    }
    return(0);
}
