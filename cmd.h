//
// Created by jason on 12/11/16.
//

#ifndef REALMSCODE_CMD_H
#define REALMSCODE_CMD_H

#include "bstring.h"

#include <string.h>  // memset

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
#ifndef PYTHON_CODE_GEN
        ret = num = 0;
        memset(str, 0, sizeof(str));
        memset(val, 0, sizeof(val));
        myCommand=0;
#endif
    };
    int         num;
    bstring     fullstr;
    char        str[COMMANDMAX][MAX_TOKEN_SIZE];
    long        val[COMMANDMAX];

    int         ret;
    Command *myCommand;
};



#endif //REALMSCODE_CMD_H
