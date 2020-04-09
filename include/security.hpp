//
// Created by jason on 4/8/20.
//

#ifndef REALMSCODE_SECURITY_HPP
#define REALMSCODE_SECURITY_HPP
class Socket;
class bstring;

void changePassword(Socket* sock, const bstring& str);

#endif //REALMSCODE_SECURITY_HPP
