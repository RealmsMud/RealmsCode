//
// Created by jason on 4/8/20.
//

#pragma once
#include <memory>

class Socket;

void postedit(std::shared_ptr<Socket> sock, const std::string& str);
void histedit(std::shared_ptr<Socket> sock, const std::string& str);
