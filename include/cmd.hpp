//
// Created by jason on 12/11/16.
//

#pragma once

#include <cstring>  // memset
#include <string>

typedef int (*SONGFN)();


// we need this forward declaration so command list pointers can
// be stored inside the cmd class
class cmdReturn;

const int COMMANDMAX = 6;

#define CMD_NOT_FOUND   -1
#define CMD_NOT_UNIQUE  -2
#define CMD_NOT_AUTH    -3

#define MAX_TOKEN_SIZE  50

class Command;

class cmd {
public:
    cmd() {
        ret = num = 0;
        memset(str, 0, sizeof(str));
        memset(val, 0, sizeof(val));
        myCommand = nullptr;
    };
    int num;
    std::string fullstr;
    char str[COMMANDMAX][MAX_TOKEN_SIZE];
    long val[COMMANDMAX];

    int ret;
    const Command *myCommand;
};

