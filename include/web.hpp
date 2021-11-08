/*
 * web.h
 *   web-based functions
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

#ifndef WEB_H_
#define WEB_H_


std::string webwho();
void updateRecentActivity();
void callWebserver(std::string url, bool questionMark=true, bool silent=false);
void webUnassociate(const std::string &user);
void webCrash(const std::string &msg);


// Ascii characters we'll need
//#define   ETX     3
//#define   EOT     4
//#define   ETB     23

class WebInterface {
public:
    static const char* fifoIn;
    static const char* fifoOut;
    static WebInterface* getInstance();
    static void destroyInstance();

private:
    // Singleton, only one instance allowed of this class
    static WebInterface* myInstance;
    WebInterface();
    ~WebInterface();
    void openFifos();
    void closeFifos();
    bool checkFifo(const char* fifoFile);

    bool checkInput();
    bool handleInput();
    bool sendOutput();
    bool messagePlayer(std::string command, std::string tempBuf);
    bool wiki(std::string command, std::string tempBuf);

private:
    int inFd{};
    int outFd{};
    std::string inBuf;  // Input Buffer
    std::string outBuf; // Output Buffer


public:
    bool handleRequests();
    void recreateFifos();

};
#endif /*WEB_H_*/
