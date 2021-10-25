//
// Created by jason on 12/11/16.
//

#ifndef REALMSCODE_PROC_H
#define REALMSCODE_PROC_H

#include <string>

enum class ChildType {
    DNS_RESOLVER,
    LISTER,
    DEMOGRAPHICS,
    SWAP_FIND,
    SWAP_FINISH,
    PRINT,
    UNKNOWN,

};


struct childProcess {
    int pid;
    ChildType type;
    int fd; // Fd if any we should watch
    std::string extra;
    childProcess() {
        pid = 0;
        fd = -1;
        extra = "";
        type = ChildType::UNKNOWN;
    }
    childProcess(int p, ChildType t, int f = -1, std::string_view e = "") {
        pid = p;
        type = t;
        fd = f;
        extra = e;
    }
};

#endif //REALMSCODE_PROC_H
