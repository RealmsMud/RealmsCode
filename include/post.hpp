//
// Created by jason on 4/8/20.
//

#ifndef REALMSCODE_POST_HPP
#define REALMSCODE_POST_HPP
class Socket;
class bstring;

void postedit(Socket* sock, const bstring& str);
void histedit(Socket* sock, const bstring& str);

#endif //REALMSCODE_POST_HPP
