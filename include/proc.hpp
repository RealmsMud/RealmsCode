//
// Created by jason on 12/11/16.
//

#ifndef REALMSCODE_PROC_H
#define REALMSCODE_PROC_H

#include "bstring.hpp"

enum childType {
    CHILD_START,

    CHILD_DNS_RESOLVER,
    CHILD_LISTER,
    CHILD_DEMOGRAPHICS,
    CHILD_SWAP_FIND,
    CHILD_SWAP_FINISH,
    CHILD_PRINT,

    CHILD_END
};


struct childProcess {
    int pid;
    childType type;
    int fd; // Fd if any we should watch
    bstring extra;
    childProcess() {
        pid = 0;
        fd = -1;
        extra = "";
    }
    childProcess(int p, childType t, int f = -1, bstring e = "") {
        pid = p;
        type = t;
        fd = f;
        extra = e;
    }
};

#endif //REALMSCODE_PROC_H
