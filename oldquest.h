//
// Created by jason on 12/11/16.
//

#ifndef REALMSCODE_OLDQUEST_H
#define REALMSCODE_OLDQUEST_H

typedef struct quest {
public:
    quest() { num = exp = 0; name = 0; };
    int     num;
    int     exp;
    char    *name;
} quest, *questPtr;


#endif //REALMSCODE_OLDQUEST_H
