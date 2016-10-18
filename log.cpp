/*
 * log.cpp
 *   Functions to log various items in the game.
 *   ____            _               
 *  |  _ \ ___  __ _| |_ __ ___  ___ 
 *  | |_) / _ \/ _` | | '_ ` _ \/ __|
 *  |  _ <  __/ (_| | | | | | | \__ \
 *  |_| \_\___|\__,_|_|_| |_| |_|___/
 *
 * Permission to use, modify and distribute is granted via the
 *  GNU Affero General Public License v3 or later
 *  
 *  Copyright (C) 2007-2012 Jason Mitchell, Randi Mitchell
 *     Contributions by Tim Callahan, Jonathan Hseu
 *  Based on Mordor (C) Brooke Paul, Brett J. Vickers, John P. Freeman
 *
 */
#include "mud.h"
#include <fcntl.h>

// TODO: Rework these to avoid redundant code

//*********************************************************************
//                      getTimeStr
//*********************************************************************
// gets the result of ctime but removes the trailing newline
// Returns: the a static pointer to time string

char *getTimeStr() {
    time_t  t;
    static char timestr[40];

    t = time(0);
    strcpy(timestr, ctime(&t));

    // strip the newline at the end
    timestr[strlen(timestr)-1] = 0;

    return(timestr);
}

//**********************************************************************
//                      logn
//**********************************************************************
// This function writes a formatted printf string to a logfile called
// "name" in the log directory.

void logn(const char *name, const char *fmt, ...) {
    char    filename[80];
    char    *str;
    FILE    *fp;
    va_list ap;

    va_start(ap, fmt);

    sprintf(filename, "%s/%s.txt", Path::Log, name);
    fp = fopen(filename, "a");
    
    if(fp == NULL) {
        printf("Unable to open '%s'\n", filename);
        return;
    }
    if(vasprintf(&str, fmt, ap) == -1) {
        printf("Error in logn\n");
        return; 
    }
    va_end(ap);
    // TODO: put the \n at the end of this 
    fprintf(fp, "%s: %s", getTimeStr(), str);

    free(str);  
    fclose(fp);
}

//**********************************************************************
//                      loge
//**********************************************************************
// This function writes a formatted printf string to a logfile called
// "log" in the player directory.

void loge(const char *fmt, ...) {
    char    file[80];
    char    str[4048];
    int     fd;
    time_t  t;
    va_list ap;

    va_start(ap, fmt);

    sprintf(file, "%s/log.txt", Path::Log);
    fd = open(file, O_RDWR, 0);
    if(fd < 0) {
        fd = open(file, O_RDWR | O_CREAT, ACC);
        if(fd < 0)
            return;
    }
    lseek(fd, 0L, 2);
    t = time(0);
    strcpy(str, ctime(&t));
    str[24] = ':';
    str[25] = ' ';
    vsnprintf(str + 26, 2000, fmt, ap);
    va_end(ap);

//  vsnprintf(str, 4000, fmt, ap);
//  va_end(ap);
//  snprintf(str, 4195, "%s: %s", getTimeStr(), str);
    write(fd, str, strlen(str));

    close(fd);

}

//**********************************************************************
//                      loga
//**********************************************************************
// Logs stuff in the active log

void loga(const char *fmt,...) {
    char    file[80];
    char    str[4048];
    int     fd;
    va_list ap;

    va_start(ap, fmt);

    sprintf(file, "%s/log.active.txt", Path::Log);
    fd = open(file, O_RDWR, 0);
    if(fd < 0) {
        fd = open(file, O_RDWR | O_CREAT, ACC);
        if(fd < 0)
            return;
    }
    lseek(fd, 0L, 2);
    
    vsnprintf(str, 4000, fmt, ap);
    va_end(ap);
    snprintf(str, 4195, "%s: %s", getTimeStr(), str);

    write(fd, str, strlen(str));

    close(fd);
}
