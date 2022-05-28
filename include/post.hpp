//
// Created by jason on 4/8/20.
//

#ifndef REALMSCODE_POST_HPP
#define REALMSCODE_POST_HPP
class Socket;

void postedit(std::shared_ptr<Socket> sock, const std::string& str);
void histedit(std::shared_ptr<Socket> sock, const std::string& str);

#endif //REALMSCODE_POST_HPP
