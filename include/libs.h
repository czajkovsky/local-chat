#ifndef LIBS_H

#define LIBS_H

#include <sys/ipc.h>
#include <string.h>
#include <sys/sem.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/shm.h>
#include <signal.h>

#define UNABLE_TO_START_REPO -100099
#define UNABLE_TO_REGISTER_SERVER -100098

#define RESP_SERVER_FULL 503
#define RESP_CLIENT_EXISTS 409
#define CLIENT_LOGED_IN 201
#define ROOM_CHANGED 202

#define MAX_FORKS 32

typedef const unsigned int Flag;

#endif
