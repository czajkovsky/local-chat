#include "../include/libs.h"
#include "../include/common.h"

int chat_quit = 0;

time_t start_time, server_start_time;

CLIENT_REPO *client_repo;
int client_sem=-1;
int client_repo_id=-1;

char command[MAX_MSG_SIZE+MAX_NAME_SIZE+4];
char input[MAX_MSG_SIZE+MAX_NAME_SIZE+4];
char arg1[MAX_MSG_SIZE+MAX_NAME_SIZE+4];
char arg2[MAX_MSG_SIZE+MAX_NAME_SIZE+4];
char arg3[MAX_MSG_SIZE+MAX_NAME_SIZE+4];
char login[MAX_NAME_SIZE+1];

void semDown(int semid);
void semUp(int semid);

//basics

void hello() {
  printf("\n<LOG> Welcome in SOPchat!\n");
  printf("<LOG> Type -h for help and usage\n");
}

void clearArgs() {
  int i;
  for(i=0; i<MAX_MSG_SIZE+MAX_NAME_SIZE+2; i++) {
    arg1[i]='\0';
    arg2[i]='\0';
    arg3[i]='\0';
    command[i]='\0';
  }
}

int updateServer(int server_id) {
  semDown(client_sem);
  client_repo->my_server=server_id;
  semUp(client_sem);
  return 0;
}

int myServer() {
  semDown(client_sem);
  int status=client_repo->my_server;
  semUp(client_sem);
  return status;
}

void updateLogin(char *login) {
  semDown(client_sem);
  strcpy(client_repo->login,login);
  semUp(client_sem);
}

char *myLogin() {
  semDown(client_sem);
  strcpy(login,client_repo->login);
  semUp(client_sem);
  return login;
}

void updateRoom(char *room) {
  semDown(client_sem);
  strcpy(client_repo->room,room);
  semUp(client_sem);
}

char *myRoom() {
  semDown(client_sem);
  strcpy(login,client_repo->room);
  semUp(client_sem);
  return login;
}

int myQueue() {
  semDown(client_sem);
  int queue=client_repo->my_queue;
  semUp(client_sem);
  return queue;
}

int createQueue() {
  semDown(client_sem);
  client_repo->my_queue=msgget(IPC_PRIVATE, 0666 | IPC_CREAT);
  printf("<LOG> Your Queue has been created (id: %d)\n",client_repo->my_queue);
  semUp(client_sem);
  return 0;
}

void semDown(int semid) {
  static struct sembuf buf = {0, -1, 0};
  if(semop(semid, &buf, 1)==0);
}

void semUp(int semid) {
  static struct sembuf buf = {0, 1, 0};
  semop(semid, &buf, 1);
}

int compare_str(char *str1, char *str2) {
  if(strcmp(str1,str2)==0) return 1;
  else return 0;
}

void exit_loop(int signo) {
  printf("<LOG> Caught signal. Type -q to exit\n");
}

long long int toNumber(char mystring[]) {
  long long int wynik=0;
  int i;
  for(i=0; i<(int)(strlen(mystring)); i++) {
    if((int)mystring[i]>57||(int)mystring[i]<48) return -1;
    wynik*=10;
    wynik+=(int)mystring[i]-'0';
  }
  return wynik;
}

int commandClue (char *str1, char *str2) {
  if(str1 == strstr(str1, str2)) return 1;
  else return 0;
}

// features

void helpAndUsage() {
  printf("<HEL> commands:\n");
  printf("<HEL> -q to close chat:\n");
  printf("<HEL> -l <login> <server> to log in\n");
  printf("<HEL> -s to show list of servers\n");
  printf("<HEL> -o to log out\n");
  printf("<HEL> -R to return to main room\n");
  printf("<HEL> -R <room> to join/create room\n");
  printf("<HEL> -r to show list of rooms\n");
  printf("<HEL> -m <message> to send public message\n");
  printf("<HEL> -p <message> to send private message\n");
  printf("<HEL> -c to show list of clients inside your room\n");
  printf("<HEL> -C to show global list of clients\n");
}

void destroyQueue(int client_msg_queue) {
  msgctl(client_msg_queue, IPC_RMID);
  printf("<LOG> Your Queue has been destroyed (id: %d)\n",client_msg_queue);
}

void serversList() {

  int servers_queue;
  int result;
  int client_msg_queue=myQueue();
  servers_queue = msgget(SERVER_LIST_MSG_KEY, 0666);
  if(servers_queue!=-1) {

    printf("geting work done...\n");

    SERVER_LIST_REQUEST req;
    req.client_msgid = client_msg_queue;
    req.type = SERVER_LIST;
    msgsnd(servers_queue, &req, sizeof(SERVER_LIST_REQUEST)-sizeof(long), 0);

    SERVER_LIST_RESPONSE res;

    if (msgrcv(myQueue(), &res, sizeof(SERVER_LIST_RESPONSE)-sizeof(long), SERVER_LIST, 0) == -1) {
      perror("error");
    }


    printf("<LOG> Available servers [%d]\n", res.active_servers);
    printf("<LOG> Iter\t\tServer\t\tClients\n");

    int i;
    for(i=0; i<res.active_servers; i++) {
      printf("<LOG> %d\t\t%d\t\t%d\n",i,res.servers[i], res.clients_on_servers[i]);
    }
  }
  else {
    printf("<LOG> No servers available. Try later.\n");
  }
}

int loginLogic(char* server_id, char *login) {
  if(myServer()>0) {
    printf("<LOG> You are already logged in as %s\n",myLogin());
    return -1;
  }
  int client_msg_queue=myQueue();
  int server_msg_queue;
  if((server_msg_queue=toNumber(server_id))>0) {
    printf("<LOG> Connecting to %d as %s\n", server_msg_queue, login);

    if(strlen(login)>MAX_NAME_SIZE) {
      printf("<LOG> Your login is to long. Max length equals %d\n", MAX_NAME_SIZE);
      return -1;
    }

    CLIENT_REQUEST request;
    request.type = LOGIN;
    request.client_msgid = client_msg_queue;
    strcpy(request.client_name, login);
    if((msgsnd(server_msg_queue, &request, sizeof(CLIENT_REQUEST)-sizeof(long), 0))==-1) {
      printf ("<LOG> Unable to connect with server\n");
      return -1;
    }
    else {
      printf("<LOG> Loging...\n");

      STATUS_RESPONSE resp;
      msgrcv(client_msg_queue, &resp, sizeof(STATUS_RESPONSE)-sizeof(long), STATUS, 0);
      if(resp.status == RESP_SERVER_FULL) {
        printf("<LOG> This server is full\n");
        return -1;
      }
      else if(resp.status == RESP_CLIENT_EXISTS) {
        printf("<LOG> Client with this login <%s> has logged in earlier...\n", login);
        return -1;
      }
      else if(resp.status == CLIENT_LOGED_IN) printf("<LOG> Logged in successfully!\n");
      updateLogin(login);
      updateServer(server_msg_queue);
      updateRoom("");
      printf("<LOG> You are now in room <>\n");
      return 0;
    }
    return -1;
  }
  else printf("<LOG> Server must be a valid number in correct range.\n");
  return -1;
}

int logoutLogic() {
  if(myServer()<0) {
    printf("<LOG> You're not loged in!\n");
    return -1;
  }
  CLIENT_REQUEST req;
  req.type = LOGOUT;
  req.client_msgid = myQueue();
  strcpy(req.client_name, myLogin());
  msgsnd(myServer(), &req, sizeof(CLIENT_REQUEST)-sizeof(long), 0);
  printf("<LOG> %s, you're logged out\n",login);
  updateServer(-1);
  return 0;
}

int roomsList() {

  if(myServer()<0) {
    printf("<LOG> You need to log in first!\n");
    return -1;
  }
  CLIENT_REQUEST req;
  req.type = ROOM_LIST;
  req.client_msgid = myQueue();
  strcpy(req.client_name, myLogin());
  msgsnd(myServer(), &req, sizeof(CLIENT_REQUEST)-sizeof(long), 0);

  ROOM_LIST_RESPONSE resp;
  msgrcv(myQueue(), &resp, sizeof(ROOM_LIST_RESPONSE)-sizeof(long), ROOM_LIST, 0);

  printf("<LOG> Available rooms [%d]\n", resp.active_rooms);
  printf("<LOG> Iter\t\tRoom\t\tClients\n");
  int i;
  for(i=0; i<resp.active_rooms; i++) {
    printf("<LOG> %d\t\t%s\t\t%d\n",i,resp.rooms[i].name, resp.rooms[i].clients);
  }

  return 0;
}

int globalClientsList() {

  if(myServer()<0) {
    printf("<LOG> You're not loged in!\n");
    return -1;
  }

  CLIENT_REQUEST req;
  req.type = GLOBAL_CLIENT_LIST;
  req.client_msgid = myQueue();
  strcpy(req.client_name, myLogin());
  msgsnd(myServer(), &req, sizeof(CLIENT_REQUEST)-sizeof(long), 0);

  CLIENT_LIST_RESPONSE resp;
  msgrcv(myQueue(), &resp, sizeof(CLIENT_LIST_RESPONSE)-sizeof(long), GLOBAL_CLIENT_LIST, 0);

  printf("<LOG> Clients globally [%d]\n", resp.active_clients);
  int i;
  for(i=0; i<resp.active_clients; i++) {
    printf("<LOG> %d\t%s\n",i,resp.names[i]);
  }
  return 0;
}

int roomClientsList() {

  if(myServer()<0) {
    printf("<LOG> You're not loged in!\n");
    return -1;
  }

  CLIENT_REQUEST req;
  req.type = ROOM_CLIENT_LIST;
  req.client_msgid = myQueue();
  strcpy(req.client_name, myLogin());
  msgsnd(myServer(), &req, sizeof(CLIENT_REQUEST)-sizeof(long), 0);

  CLIENT_LIST_RESPONSE resp;
  msgrcv(myQueue(), &resp, sizeof(CLIENT_LIST_RESPONSE)-sizeof(long), ROOM_CLIENT_LIST, 0);

  printf("<LOG> Clients in <%s> [%d]\n", myRoom(), resp.active_clients);
  int i;
  for(i=0; i<resp.active_clients; i++) {
    printf("<LOG> %d\t%s\n",i,resp.names[i]);
  }
  return 0;
}

int changeRoom(char *room) {

  if(myServer()<0) {
    printf("<LOG> You're not loged in!\n");
    return -1;
  }

  CHANGE_ROOM_REQUEST req;
  req.type = CHANGE_ROOM;
  req.client_msgid = myQueue();
  strcpy(req.client_name, myLogin());
  strcpy(req.room_name, room);

  msgsnd(myServer(), &req, sizeof(CHANGE_ROOM_REQUEST)-sizeof(long), 0);

  STATUS_RESPONSE resp;
  if(msgrcv(myQueue(), &resp, sizeof(STATUS_RESPONSE)-sizeof(long), CHANGE_ROOM, 0)==-1) {
    perror("error");
  }

  if(resp.status == 202) {
    printf("<LOG> You are chatting now in <%s>\n",room);
    updateRoom(room);
    return 0;
  }
  return -1;
}

int commandParser(char *command, char *arg1, char *arg2, char *arg3) {
  int i,length=strlen(command),j;

  if(length<2) return -1;
  if(command[0]!='-') return -1;
  i=1; j=0;
  arg1[0]=command[1];
  if(compare_str("q",arg1)==1) {
    if(length==2) return 1;
    else return -1;
  }
  else if(compare_str("m",arg1)==1) {
    j=0; i=2;
    if((int)command[i]==32) {
      i=3;
      while(i<length) {
        arg2[j]=command[i]; i++; j++;
      }
      return 2;
    }
  }
  else if(compare_str("R",arg1)==1) {
    j=0; i=2;
    if((int)command[i]==32) {
      i=3;
      j=0; i=3;
      while((int)command[i]!=32&&i<length) {
        arg2[j]=command[i]; i++; j++;
      }
      if(i==length) return 9;
    }
    else {
      if(length==2) return 10;
    }
  }
  else if(compare_str("h",arg1)==1) {
    if(length==2) return 3;
    else return -1;
  }
  else if(compare_str("c",arg1)==1) {
    if(length==2) return 7;
    else return -1;
  }
  else if(compare_str("C",arg1)==1) {
    if(length==2) return 11;
    else return -1;
  }
  else if(compare_str("r",arg1)==1) {
    if(length==2) return 8;
    else return -1;
  }
  else if(compare_str("s",arg1)==1) {
    if(length==2) return 4;
    else return -1;
  }
  else if(compare_str("o",arg1)==1) {
    if(length==2) return 6;
  }
  else if(compare_str("l",arg1)==1) {
    j=0; i=2;
    if((int)command[i]==32) {
      j=0; i=3;
      while((int)command[i]!=32&&i<length) {
        arg2[j]=command[i]; i++; j++;
      }
      if((int)command[i]==32) {
        j=0;
        i++;
        while((int)command[i]!=32&&i<length) {
          arg3[j]=command[i]; i++; j++;
        }
        if(i==length) return 5;
      }
    }
  }
  else if(compare_str("p",arg1)==1) {
    j=0; i=2;
    if((int)command[i]==32) {
      j=0; i=3;
      while((int)command[i]!=32&&i<length) {
        arg2[j]=command[i]; i++; j++;
      }
      if((int)command[i]==32) {
        j=0;
        i++;
        while(i<length) {
          arg3[j]=command[i]; i++; j++;
        }
        if(i==length) return 12;
      }
    }
  }
  return -1;
}

int setClientValues() {
  createQueue();
  updateServer(-1);
}

int initClientRepo() {
  if((client_sem=semget(IPC_PRIVATE, 1, 0666 | IPC_CREAT))!=-1) {
    semctl(client_sem, 0, SETVAL, 0);
    printf("<LOG> Client Semaphore created (%d)\n",client_sem);

    //repo
    if((client_repo_id=shmget(IPC_PRIVATE, sizeof(CLIENT_REPO), 0666 | IPC_CREAT))!=-1) {
      printf("<LOG> Client Repo created (id: %d)\n",client_repo_id);
      client_repo = (CLIENT_REPO*)shmat(client_repo_id, NULL, 0);
      printf("<LOG> Client Repo connected - ready to work\n");
    }
    else printf("<LOG> Error while trying to create Repo");
    semUp(client_sem);
  }
  else printf("<LOG> Unable to create Client Semaphore\n");
  return 0;
}

int destroyClient() {
  if(client_sem!=-1) {

    semDown(client_sem);

    if(client_repo_id!=-1) {

      destroyQueue(client_repo->my_queue);

      shmdt(client_repo);
      shmctl(client_repo_id, 0, IPC_RMID);
      printf("<LOG> Client Repo removed (id: %d)\n",client_repo_id);
    }
    else printf("<LOG> Client repo doesn't exist\n");

    semUp(client_sem);

    semctl(client_sem, 0, IPC_RMID);
    printf("<LOG> Client Semaphore destroyed (%d)\n",client_sem);
    printf("<LOG> Shutting down...\n");
  }
  else printf("<LOG> There is no Client Semaphore\n");
  return 0;
}

void heartbeat() {

  STATUS_RESPONSE res;

  while(1) {
    CLIENT_REQUEST req;
    req.type = HEARTBEAT;
    req.client_msgid = myQueue();
    strcpy(req.client_name, myLogin());

    if (msgrcv(myQueue(), &res, sizeof(STATUS_RESPONSE)-sizeof(long), HEARTBEAT, 0) == -1) perror("error");
    else {
      if(msgsnd(res.status, &req, sizeof(CLIENT_REQUEST)-sizeof(long), 0)==-1) perror("error");
    }
  }
}

void checkingForPrivateMessages() {
  while(1) {
    TEXT_MESSAGE msg;
    int result = msgrcv(myQueue(), &msg, sizeof(TEXT_MESSAGE)-sizeof(long), PRIVATE, IPC_NOWAIT);
    if(result != -1) {
      char *s = ctime(&msg.time);
      *(s+24) = 0;
      printf("<PRI> (%s) %s says '%s'\n", s, msg.from_name, msg.text);
    }
  }
}

void checkingForPublicMessages() {
  while(1) {
    TEXT_MESSAGE msg;
    int result = msgrcv(myQueue(), &msg, sizeof(TEXT_MESSAGE)-sizeof(long), PUBLIC, 0);
    if(result != -1) {
      char *s = ctime(&msg.time);
      *(s+24) = 0;
      printf("<PUB> (%s) %s says '%s'\n", s, msg.from_name, msg.text);
    }
  }
}

int sendPrivate(char *toWho, char *message) {
  if(myServer() < 0) {
    printf("<LOG> unable to connect to server\n");
    return -1;
  }
  TEXT_MESSAGE req;
  req.type = PRIVATE;
  req.from_id = myQueue();
  req.time = time(0);
  strcpy(req.to,toWho);
  strcpy(req.from_name, myLogin());
  strcpy(req.text, message);

  msgsnd(myServer(), &req, sizeof(TEXT_MESSAGE)-sizeof(long), 0);
  return 0;
}

int sendPublic(char *message) {
  if(myServer() < 0) {
    printf("<LOG> unable to connect to server\n");
    return -1;
  }
  TEXT_MESSAGE req;
  req.type = PUBLIC;
  req.from_id = myQueue();
  req.time = time(0);
  strcpy(req.from_name, myLogin());
  strcpy(req.text, message);

  msgsnd(myServer(), &req, sizeof(TEXT_MESSAGE)-sizeof(long), 0);

  return 0;
}

int main() {

  //signal(2,exit_loop);

  initClientRepo();
  setClientValues();

  pid_t pids[MAX_FORKS];
  int p=0;

  if ((pids[p++] = fork()) == 0) checkingForPrivateMessages();
  else if ((pids[p++] = fork()) == 0) checkingForPublicMessages();
  else if ((pids[p++] = fork()) == 0) heartbeat();
  else  {
    hello();
    serversList(myQueue());

    while(chat_quit!=1) {

      clearArgs();
      gets(command);

      switch(commandParser(command, arg1, arg2, arg3)) {
        case 1: chat_quit=1; break;
        case 2: sendPublic(arg2); break;
        case 3: helpAndUsage(); break;
        case 4: serversList(); break;
        case 5: loginLogic(arg3, arg2); break;
        case 6: logoutLogic(); break;
        case 7: roomClientsList(); break;
        case 8: roomsList(); break;
        case 9: changeRoom(arg2); break;
        case 10: changeRoom(""); break;
        case 11: globalClientsList(); break;
        case 12: sendPrivate(arg2, arg3); break;
        default: printf("<LOG> Wrong command or too much/less/wrong argumnets. Type -h for help\n"); break;
      }
    }
  }

  if(myServer()>0) logoutLogic();
  int i;
  semDown(client_sem);
  for (i=0; i<p; i++) {
    kill(pids[i], SIGKILL);
  }
  semUp(client_sem);

  destroyClient();

  return 0;
}
