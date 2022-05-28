//
// Created by jason on 4/8/20.
//

#ifndef REALMSCODE_SECURITY_HPP
#define REALMSCODE_SECURITY_HPP
class Socket;

void changePassword(std::shared_ptr<Socket> sock, const std::string& str);

#endif //REALMSCODE_SECURITY_HPP
