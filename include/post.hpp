//
// Created by jason on 4/8/20.
//

#ifndef REALMSCODE_POST_HPP
#define REALMSCODE_POST_HPP
class Socket;

void postedit(Socket* sock, const std::string& str);
void histedit(Socket* sock, const std::string& str);

#endif //REALMSCODE_POST_HPP
