#include "../include/libs.h"
#include "../include/common.h"

int inf_loop=1;

time_t start_time, server_start_time;

//server local shm
SERVER_REPO *server_repo;
int server_sem=-1;
int server_repo_id=-1;

//main repo
REPO *repo;
int repo_sem=-1;
int repo_id=-1;
int log_sem=-1;
int main_msg_queue=-1;

void semDown(int semid);
void semUp(int semid);




int compare_str(char *str1, char *str2) {
  if(strcmp(str1,str2)==0) return 1;
  else return 0;
}

int forClients() {
  semDown(server_sem);
  int queue=server_repo->clients_queue;
  semUp(server_sem);
  return queue;
}

int forServers() {
  semDown(server_sem);
  int queue=server_repo->servers_queue;
  semUp(server_sem);
  return queue;
}

void incClients() {
  semDown(server_sem);
  server_repo->clients++;
  semUp(server_sem);
}

int clientsOnMe() {
  semDown(server_sem);
  int clients=server_repo->clients;
  semUp(server_sem);
  return clients;
}

void decClients() {
  semDown(server_sem);
  server_repo->clients--;
  semUp(server_sem);
}

void semDown(int semid) {
  static struct sembuf buf = {0, -1, 0};
  if(semop(semid, &buf, 1)==0);
}

void semUp(int semid) {
  static struct sembuf buf = {0, 1, 0};
  semop(semid, &buf, 1);
}

void exit_loop(int signo) {
  printf("<LOG> Caught signal. Finishing work...\n");
  inf_loop = 0;
}

//inside critical section!!!!

int findServerIterator(int clients_queue) {
  int servers_left = repo->active_servers,iterator=-1,i;
  for(i=0; i<servers_left; i++) {
    if(repo->servers[i].client_msgid==clients_queue) iterator=i;
  }
  return iterator;
}

int findClientIterator(char *clientName) {
  int clients_left = repo->active_clients,iterator=-1,i;
  for(i=0; i<clients_left; i++) {
    if(compare_str(repo->clients[i].name,clientName)==1) iterator=i;
  }
  return iterator;
}

int findRoomIterator(char *roomName) {
  int rooms_left = repo->active_rooms,iterator=-1,i;
  for(i=0; i<rooms_left; i++) {
    if(compare_str(repo->rooms[i].name,roomName)==1) iterator=i;
  }
  return iterator;
}

void sort_servers () {
  int i,j;
  for (i=0; i<repo->active_servers; i++) {
    for (j=0; j<repo->active_servers; j++) {
      if (repo->servers[j].client_msgid > repo->servers[i].client_msgid) {

        SERVER tmp_server;
        tmp_server.client_msgid = repo->servers[i].client_msgid;
        tmp_server.server_msgid = repo->servers[i].server_msgid;
        tmp_server.clients = repo->servers[i].clients;

        repo->servers[i].client_msgid = repo->servers[j].client_msgid;
        repo->servers[i].server_msgid = repo->servers[j].server_msgid;
        repo->servers[i].clients = repo->servers[j].clients;

        repo->servers[j].client_msgid = tmp_server.client_msgid;
        repo->servers[j].server_msgid = tmp_server.server_msgid;
        repo->servers[j].clients = tmp_server.clients;
      }
    }
  }
}

void show_clients() {

  int i;
  int clients_left = repo->active_clients;
  printf("<LOG> Users list:\n<LOG> Iter\tUser\t\tServer id\t\tRoom\n");
  for(i=0; i<clients_left; i++) printf("<LOG> %d\t\t%s\t\t%d\t\t%s\n",i,repo->clients[i].name,repo->clients[i].server_id,repo->clients[i].room);
}

void show_rooms() {

  int i;
  int rooms_left = repo->active_rooms;
  printf("<LOG> Rooms list:\n<LOG> Iter\tRoom\t\tClients\n");
  for(i=0; i<rooms_left; i++) printf("<LOG> %d\t\t%s\t\t%d\n",i,repo->rooms[i].name,repo->rooms[i].clients);
}

void sort_clients () {
  int i,j;
  for (i=0; i<repo->active_clients; i++) {
    for (j=0; j<repo->active_clients; j++) {
      if (strcmp(repo->clients[j].name,repo->clients[i].name)>0) {

        CLIENT tmp_client;
        strcpy(tmp_client.name,repo->clients[i].name);
        tmp_client.server_id = repo->clients[i].server_id;
        strcpy(tmp_client.room,repo->clients[i].room);

        strcpy(repo->clients[i].name,repo->clients[j].name);
        repo->clients[i].server_id = repo->clients[j].server_id;
        strcpy(repo->clients[i].room,repo->clients[j].room);

        strcpy(repo->clients[j].name,tmp_client.name);
        repo->clients[j].server_id = tmp_client.server_id;
        strcpy(repo->clients[j].room,tmp_client.room);
      }
    }
  }
}

void sort_rooms () {
  int i,j;
  for (i=0; i<repo->active_rooms; i++) {
    for (j=0; j<repo->active_rooms; j++) {
      if (strcmp(repo->rooms[j].name,repo->rooms[i].name)>0) {

        ROOM tmp_room;
        strcpy(tmp_room.name,repo->rooms[i].name);
        tmp_room.clients = repo->rooms[i].clients;

        strcpy(repo->rooms[i].name,repo->rooms[j].name);
        repo->rooms[i].clients = repo->rooms[j].clients;

        strcpy(repo->rooms[j].name,tmp_room.name);
        repo->rooms[j].clients = tmp_room.clients;
      }
    }
  }
}

void logger(int action, int number, char client_name[MAX_NAME_SIZE]) {

  int i=0;
  char server[20],tmp[20];
  while (number) {
    tmp[i] = (char)(number%10+'0');
    number /= 10;
    i++;
  }
  int j=-1;
  while(i) {
    i--;
    j++;
    server[j]=tmp[i];
  }
  j++;
  server[j]='\0';

  semDown(log_sem);

  int fd;
  if ((fd = open("./czat.log", O_CREAT | O_RDWR | O_APPEND, 0666)) == -1) perror("error");
  else {
    if(action==1) {
      write(fd, "ALIVE: <", 8*sizeof(char));
      write(fd, server, strlen(server));
      write(fd, ">", 1*sizeof(char));
      write(fd, "\n", 1*sizeof(char));
    }
    else if(action==2) {
      write(fd, "DOWN: <", 7*sizeof(char));
      write(fd, server, strlen(server));
      write(fd, ">", 1*sizeof(char));
      write(fd, "\n", 1*sizeof(char));
    }
    else if(action==3) {
      write(fd, "LOGGED_IN@<", 11*sizeof(char));
      write(fd, server, strlen(server));
      write(fd, ">: <", 4*sizeof(char));
      write(fd, client_name, strlen(client_name));
      write(fd, ">", 1*sizeof(char));
      write(fd, "\n", 1*sizeof(char));
    }
    else if(action==3) {
      write(fd, "LOGGED_OUT@<", 12*sizeof(char));
      write(fd, server, strlen(server));
      write(fd, ">: <", 4*sizeof(char));
      write(fd, client_name, strlen(client_name));
      write(fd, ">", 1*sizeof(char));
      write(fd, "\n", 1*sizeof(char));
    }
    else if(action==4) {
      write(fd, "LOGGED_OUT@<", 12*sizeof(char));
      write(fd, server, strlen(server));
      write(fd, ">: <", 4*sizeof(char));
      write(fd, client_name, strlen(client_name));
      write(fd, ">", 1*sizeof(char));
      write(fd, "\n", 1*sizeof(char));
    }
    else if(action==5) {
      write(fd, "DEAD@<", 6*sizeof(char));
      write(fd, server, strlen(server));
      write(fd, ">: <", 4*sizeof(char));
      write(fd, client_name, strlen(client_name));
      write(fd, ">", 1*sizeof(char));
      write(fd, "\n", 1*sizeof(char));
    }
  }
  close(fd);
  semUp(log_sem);
}

//checkpoints functions

int clientOwnQueue(char *name) {
  int myClients=server_repo->clients;
  int i;
  for(i=0; i<myClients; i++) {
    if(!strcmp(name,server_repo->clients_pairs[i].name)) return server_repo->clients_pairs[i].own_queue;
  }
  return -1;
}

void showOwnClients() {
  int myClients=server_repo->clients;
  int i;
  for(i=0; i<myClients; i++) printf("c--- %s\n", server_repo->clients_pairs[i].name);
}

void clientRemove(char *name) {
  int myClients=server_repo->clients;
  int i,it;
  for(i=0; i<myClients; i++) {
    if(!strcmp(name,server_repo->clients_pairs[i].name)) it=i;
  }
  strcpy(server_repo->clients_pairs[it].name,server_repo->clients_pairs[myClients-1].name);
  server_repo->clients_pairs[it].own_queue=server_repo->clients_pairs[myClients-1].own_queue;
  server_repo->clients--;
}

void showServersData() {

  semDown(repo_sem);
  int i;
  int servers_left = repo->active_servers;
  printf("<LOG> Servers list:\n<LOG> Iter\tServer\t\tClient\t\tClients\n");
  for(i=0; i<servers_left; i++) printf("<LOG> %d\t\t%d\t\t%d\t\t%d\n",i,repo->servers[i].server_msgid,repo->servers[i].client_msgid,repo->servers[i].clients);
  semUp(repo_sem);
}

//engine functions

void publicMessage() {

  TEXT_MESSAGE msg;

  while(1) {
    if(msgrcv(forClients(), &msg, sizeof(TEXT_MESSAGE)-sizeof(long), PUBLIC, 0)==-1) {
      perror("error");
    }

    printf("<LOG> public message received\n");

    semDown(repo_sem);

    msg.from_id = forServers();

    int client_i=findClientIterator(msg.from_name);
    int room_i=findRoomIterator(repo->clients[client_i].room);

    int ids_of_servers[MAX_SERVER_NUM];
    int ids_of_clients[MAX_SERVER_NUM];

    int i;
    for(i=0; i<repo->active_servers; i++) ids_of_servers[i]=repo->servers[i].server_msgid;

    semUp(repo_sem);

    for(i=0; i<repo->active_servers; i++) msgsnd(ids_of_servers[i], &msg, sizeof(TEXT_MESSAGE)-sizeof(long), 0);
  }
}

void broadcastPubilcMessage() {

  TEXT_MESSAGE msg;

  while(1) {
    if(msgrcv(forServers(), &msg, sizeof(TEXT_MESSAGE)-sizeof(long), PUBLIC, 0)==-1) {
      perror("error");
    }
    int client_i=findClientIterator(msg.from_name);

    semDown(repo_sem);
    int i=0;
    for(i=0; i<repo->active_clients; i++) {
      if(forClients()==repo->clients[i].server_id) {
        if(!strcmp(repo->clients[client_i].room,repo->clients[i].room)&&strcmp(repo->clients[client_i].name,repo->clients[i].name)!=0) {
          semDown(server_sem);
            int ownQueue=clientOwnQueue(repo->clients[i].name);
          semUp(server_sem);
          msgsnd(ownQueue, &msg, sizeof(TEXT_MESSAGE)-sizeof(long), 0);
        }
      }
    }
    semUp(repo_sem);
  }
}

void leaveRoom(char* client_name) {
  int client_i=findClientIterator(client_name);
  printf("client iterator: %d\n",client_i);
  int prev_room_i=findRoomIterator(repo->clients[client_i].room);
  repo->rooms[prev_room_i].clients--;
  printf("<LOG> Room <%s> is about to been considered to revmove...\n", repo->rooms[prev_room_i].name);
  if(repo->rooms[prev_room_i].clients==0) {
    printf("<LOG> Room <%s> is about to be removed...\n", repo->rooms[prev_room_i].name);
    repo->active_rooms--;
    strcpy(repo->rooms[prev_room_i].name, repo->rooms[repo->active_rooms].name);
    repo->rooms[prev_room_i].clients=repo->rooms[repo->active_rooms].clients;
    sort_rooms();
  }
  printf("<LOG> There are still [%d] more clients in <%s>...\n", repo->rooms[prev_room_i].clients, repo->rooms[prev_room_i].name);
}

void sendServersList() {

  SERVER_LIST_REQUEST req;

  while(1) {
    int res = msgrcv(main_msg_queue, &req, sizeof(SERVER_LIST_REQUEST)-sizeof(long), SERVER_LIST, 0);

    if(res!=-1) {

      printf("<LOG> Server list demand\n");

      semDown(repo_sem);

      SERVER_LIST_RESPONSE resp;
      resp.active_servers = repo->active_servers;
      resp.type = SERVER_LIST;

      int i;
      for(i = 0; i < repo->active_servers; i++) {
        resp.servers[i] = repo->servers[i].client_msgid;
        resp.clients_on_servers[i] = repo->servers[i].clients;
      }

      semUp(repo_sem);

      msgsnd(req.client_msgid, &resp, sizeof(SERVER_LIST_RESPONSE)-sizeof(long), 0);
      printf("<LOG> Server list send to client (id:%d)\n",req.client_msgid);
    }
  }
}

void loginClient() {

  CLIENT_REQUEST req;
  int clients_queue=forClients();

  while(1) {

    int res = msgrcv(clients_queue, &req, sizeof(CLIENT_REQUEST)-sizeof(long), LOGIN, 0);

    if(res!=-1) {
      semDown(repo_sem);
      int server_i=findServerIterator(forClients());
      printf("<LOG> %s is trying to log in...\n",req.client_name);

      STATUS_RESPONSE resp;
      resp.type = STATUS;

      if(repo->servers[server_i].clients == SERVER_CAPACITY) {
        resp.status = RESP_SERVER_FULL;
        printf("<LOG> But server is full...\n");
      }
      else if(findClientIterator(req.client_name)!=-1) {
        resp.status = RESP_CLIENT_EXISTS;
        printf("<LOG> But this login is in use... (%s)\n",req.client_name);
      }
      else {
        int clients_left = repo->active_clients;

        repo->clients[clients_left].server_id=clients_queue;
        strcpy(repo->clients[clients_left].name, req.client_name);
        strcpy(repo->clients[clients_left].room,"");
        repo->active_clients++;

        sort_clients();
        show_clients();

        if(repo->active_clients==1) {
          //first client
          strcpy(repo->rooms[0].name,"");
          repo->rooms[0].clients=1;
          repo->active_rooms=1;
          sort_rooms();
        }
        else {
          if(strcmp(repo->rooms[0].name,"")!=0) {
            //not in main room
            strcpy(repo->rooms[repo->active_rooms].name,"");
            repo->rooms[repo->active_rooms].clients=1;
            repo->active_rooms++;
            sort_rooms();
          }
          else {
            //in main room
            repo->rooms[0].clients++;
          }
        }

        show_rooms();

        int myClients=clientsOnMe();
        semDown(server_sem);
          strcpy(server_repo->clients_pairs[myClients].name,req.client_name);
          server_repo->clients_pairs[myClients].own_queue=req.client_msgid;
        semUp(server_sem);
        incClients();

        repo->servers[server_i].clients++;

        resp.status = CLIENT_LOGED_IN;

        logger(3,forClients(),req.client_name);
      }
      int iteratorAfterSort = findClientIterator(req.client_name);
      semUp(repo_sem);
      msgsnd(req.client_msgid, &resp, sizeof(STATUS_RESPONSE)-sizeof(long), 0);
    }
  }
}

void forceToLogout(char *client_name) {

  semDown(repo_sem);

  leaveRoom(client_name);

  int server_i=findServerIterator(forClients());

  int client_i = findClientIterator(client_name);
  int numOfClients = repo->active_clients;

  repo->servers[server_i].clients--;

  repo->clients[client_i].server_id=repo->clients[numOfClients-1].server_id;
  strcpy(repo->clients[client_i].name, repo->clients[numOfClients-1].name);
  strcpy(repo->clients[client_i].room, repo->clients[numOfClients-1].room);

  repo->active_clients--;

  sort_clients(repo);
  show_clients(repo);

  semDown(server_sem);
  clientRemove(client_name);
  semUp(server_sem);

  semUp(repo_sem);
}

void privateMessage() {

  TEXT_MESSAGE msg;

  while(1) {
    if(msgrcv(forClients(), &msg, sizeof(TEXT_MESSAGE)-sizeof(long), PRIVATE, 0)==-1) {
      perror("error");
    }

    printf("<LOG> private message received\n");

    semDown(repo_sem);

    msg.from_id = forServers();

    int client_i=findClientIterator(msg.from_name);
    int room_i=findRoomIterator(repo->clients[client_i].room);

    int ids_of_servers[MAX_SERVER_NUM];
    int ids_of_clients[MAX_SERVER_NUM];

    int i;
    for(i=0; i<repo->active_servers; i++) ids_of_servers[i]=repo->servers[i].server_msgid;

    semUp(repo_sem);

    for(i=0; i<repo->active_servers; i++) msgsnd(ids_of_servers[i], &msg, sizeof(TEXT_MESSAGE)-sizeof(long), 0);
  }
}

void broadcastPrivateMessage() {

  TEXT_MESSAGE msg;

  while(1) {
    if(msgrcv(forServers(), &msg, sizeof(TEXT_MESSAGE)-sizeof(long), PRIVATE, 0)==-1) {
      perror("error");
    }
    int client_i=findClientIterator(msg.from_name);

    semDown(repo_sem);
    int i=0;
    for(i=0; i<repo->active_clients; i++) {
      if(forClients()==repo->clients[i].server_id) {
        if(!strcmp(repo->clients[i].name,msg.to)) {
          semDown(server_sem);
            int ownQueue=clientOwnQueue(repo->clients[i].name);
          semUp(server_sem);
          msgsnd(ownQueue, &msg, sizeof(TEXT_MESSAGE)-sizeof(long), 0);
        }
      }
    }
    semUp(repo_sem);
  }
}

void logoutClient() {

  CLIENT_REQUEST req;

  int clients_queue=forClients();

  while (1)
  {
    int res = msgrcv(clients_queue, &req, sizeof(CLIENT_REQUEST)-sizeof(long), LOGOUT, 0);

    if(res!=-1) {

      printf("<LOG> %s is trying to log out...\n",req.client_name);

      logger(4,forClients(),req.client_name);

      forceToLogout(req.client_name);

      printf("<LOG> %s loged out with success...\n",req.client_name);

    }
  }
}

void roomsList () {

  CLIENT_REQUEST req;

  while(1) {

    int res = msgrcv(forClients(), &req, sizeof(CLIENT_REQUEST)-sizeof(long), ROOM_LIST, 0);

    if(res!=-1) {
      semDown(repo_sem);
      printf("<LOG> %s is trying to get room list...\n",req.client_name);

      ROOM_LIST_RESPONSE resp;
      resp.type = ROOM_LIST;

      int i,rooms_count = 0;
      resp.active_rooms=repo->active_rooms;
      for(i = 0; i < repo->active_rooms; i++) {
        strcpy(resp.rooms[i].name,repo->rooms[i].name);
        resp.rooms[i].clients = repo->rooms[i].clients;
      }

      semUp(repo_sem);

      msgsnd(req.client_msgid, &resp, sizeof(ROOM_LIST_RESPONSE)-sizeof(long), 0);
    }
  }
}

void globalClientsList () {

  CLIENT_REQUEST req;

  while(1) {

    int res = msgrcv(forClients(), &req, sizeof(CLIENT_REQUEST)-sizeof(long), GLOBAL_CLIENT_LIST, 0);

    if(res!=-1) {
      semDown(repo_sem);
      printf("<LOG> %s is trying to get global client list...\n",req.client_name);

      CLIENT_LIST_RESPONSE resp;
      resp.type = GLOBAL_CLIENT_LIST;

      int i,rooms_count = 0;
      resp.active_clients=repo->active_clients;
      for(i = 0; i < repo->active_clients; i++) {
        strcpy(resp.names[i],repo->clients[i].name);
      }

      semUp(repo_sem);

      msgsnd(req.client_msgid, &resp, sizeof(CLIENT_LIST_RESPONSE)-sizeof(long), 0);
    }
  }
}

void roomClientsList () {

  CLIENT_REQUEST req;

  while(1) {

    int res = msgrcv(forClients(), &req, sizeof(CLIENT_REQUEST)-sizeof(long), ROOM_CLIENT_LIST, 0);

    if(res!=-1) {
      semDown(repo_sem);

      int client_i=findClientIterator(req.client_name);

      printf("<LOG> %s is trying to get room <%s> client list...\n",req.client_name,repo->clients[client_i].room);

      CLIENT_LIST_RESPONSE resp;
      resp.type = ROOM_CLIENT_LIST;

      int i,clients_count = 0;

      for(i = 0; i < repo->active_clients; i++) {
        if(strcmp(repo->clients[i].room,repo->clients[client_i].room)==0) {
          strcpy(resp.names[clients_count],repo->clients[i].name);
          clients_count++;
        }
      }

      resp.active_clients=clients_count;

      semUp(repo_sem);

      msgsnd(req.client_msgid, &resp, sizeof(CLIENT_LIST_RESPONSE)-sizeof(long), 0);
    }
  }
}

void changeRoom () {

  CHANGE_ROOM_REQUEST req;

  int res;

  while(1) {

    if((res=msgrcv(forClients(), &req, sizeof(CHANGE_ROOM_REQUEST)-sizeof(long), CHANGE_ROOM, 0))==-1) {
      perror("error");
    }
    if(res!=-1) {
      semDown(repo_sem);
      int server_i=findServerIterator(forClients());
      printf("<LOG> %s is trying to change room for %s...\n",req.client_name,req.room_name);
      int i;
      //to do if user room != request room
      int client_i=findClientIterator(req.client_name);
      int room_i=findRoomIterator(req.room_name);
      int prev_room_i=findRoomIterator(repo->clients[client_i].room);
      if(room_i==prev_room_i) printf("<LOG> User have been in this room already\n");
      else {
        leaveRoom(req.client_name);
        room_i=findRoomIterator(req.room_name);
        prev_room_i=findRoomIterator(repo->clients[client_i].room);
        strcpy(repo->clients[client_i].room,req.room_name);
        if(room_i==-1) {
          strcpy(repo->rooms[repo->active_rooms].name,req.room_name);
          repo->rooms[repo->active_rooms].clients=1;
          sort_rooms();
          repo->active_rooms++;
        }
        else repo->rooms[room_i].clients++;
        show_clients();
        show_rooms();
      }

      semUp(repo_sem);

      STATUS_RESPONSE resp;
      resp.type = CHANGE_ROOM;

      resp.status = ROOM_CHANGED;

      msgsnd(req.client_msgid, &resp, sizeof(STATUS_RESPONSE)-sizeof(long), 0);
    }
  }
}

//init & end functions

int initLocalRepo() {
  if((server_sem=semget(IPC_PRIVATE, 1, 0666 | IPC_CREAT))!=-1) {
    semctl(server_sem, 0, SETVAL, 0);
    printf("> <LOG> Server Semaphore created (%d)\n",server_sem);

    //repo
    if((server_repo_id=shmget(IPC_PRIVATE, sizeof(SERVER_REPO), 0666 | IPC_CREAT))!=-1) {
      printf("> <LOG> Server Repo created (id: %d)\n",server_repo_id);
      server_repo = (SERVER_REPO*)shmat(server_repo_id, NULL, 0);
      printf("> <LOG> Server Repo connected - ready to work\n");

      server_repo->servers_queue=msgget(IPC_PRIVATE, 0666 | IPC_CREAT);
      printf("> <LOG> Your Servers Queue has been created (id: %d)\n",server_repo->servers_queue);

      server_repo->clients_queue=msgget(IPC_PRIVATE, 0666 | IPC_CREAT);
      printf("> <LOG> Your Clients Queue has been created (id: %d)\n",server_repo->clients_queue);
      server_repo->clients=0;
    }
    else printf("> <LOG> Error while trying to create Repo");
    semUp(server_sem);


  }
  else printf("> <LOG> Unable to create Server Semaphore\n");

  return 0;
}

void destroyQueue(int client_msg_queue) {
  msgctl(client_msg_queue, IPC_RMID);
  printf("> <LOG> Your Queue has been destroyed (id: %d)\n",client_msg_queue);
}

int destroyServer() {
  if(server_sem!=-1) {

    semDown(server_sem);

    if(server_repo_id!=-1) {

      destroyQueue(server_repo->clients_queue);
      destroyQueue(server_repo->servers_queue);

      shmdt(server_repo);
      shmctl(server_repo_id, 0, IPC_RMID);
      printf("> <LOG> Servers Repo removed (id: %d)\n",server_repo_id);
    }
    else printf("> <LOG> Server repo doesn't exist\n");

    semUp(server_sem);

    semctl(server_sem, 0, IPC_RMID);
    printf("> <LOG> Server Semaphore destroyed (%d)\n",server_sem);
    printf("> <LOG> Shutting down...\n");
  }
  else printf("> <LOG> There is no Server Semaphore\n");
  return 0;
}

int initRepo() {
  if((repo_sem=semget(SEM_REPO, 1, 0666 | IPC_CREAT | IPC_EXCL))!=-1) {
    //first one
    printf("<LOG> Repo Semaphore created (id: %d)\n",repo_sem);
    semctl(repo_sem, 0, SETVAL, 0);

    //repo
    if((repo_id=shmget(ID_REPO, sizeof(REPO), 0666 | IPC_CREAT))==-1) {
      printf("<LOG> Error while trying to create Repo");
      semUp(repo_sem);
      return UNABLE_TO_START_REPO;
    }
    repo = (REPO*)shmat(repo_id, NULL, 0);
    printf("<LOG> Connected to repo (id: %d)\n",repo_id);

    if((log_sem=semget(SEM_LOG, 1, 0666 | IPC_CREAT | IPC_EXCL))==-1) {
      printf("<LOG> Error while trying to create Log File Sempahore\n");
      semUp(repo_sem);
      return UNABLE_TO_START_REPO;
    }
    semctl(log_sem, 0, SETVAL, 1);
    printf("<LOG> Log File Semaphore created (id: %d)\n",log_sem);

    main_msg_queue = msgget(SERVER_LIST_MSG_KEY, 0666 | IPC_CREAT);
    printf("<LOG> Main servers queue created (id: %d)\n",main_msg_queue);

    printf("<LOG> Ready to work - repo created successfully\n");

    repo->active_clients = 0;
    repo->active_rooms = 0;
    repo->active_servers = 0;

    semUp(repo_sem);
    return 0;
  }
  else {
    //not first one
    repo_sem=semget(SEM_REPO,0,0666);
    printf("<LOG> Repo has been created earlier (id: %d)\n",repo_sem);

    semDown(repo_sem);

    if((repo_id=shmget(ID_REPO, sizeof(REPO), 0))==-1) {
      printf("<LOG> Error while trying to create Repo");
      semUp(repo_sem);
      return UNABLE_TO_START_REPO;
    }
    repo = (REPO*)shmat(repo_id, NULL, 0);
    printf("<LOG> Connected to repo (id: %d)\n",repo_id);

    log_sem=semget(SEM_LOG, 1, 0);
    printf("<LOG> Log File Semaphore (id: %d)\n",log_sem);

    main_msg_queue = msgget(SERVER_LIST_MSG_KEY, 0666 | IPC_CREAT);
    printf("<LOG> Main servers queue created (id: %d)\n",main_msg_queue);

    semUp(repo_sem);
  }
  return 0;
}

int destroyRepo() {
  repo_sem=semget(SEM_REPO, 1, 0666),repo_id;
  if(repo_sem==-1) {
    printf("<LOG> Repo semaphore has been removed earlier\n");

    shmctl(repo_id, 0, IPC_RMID);
    shmctl(repo_id, 0, IPC_RMID);
    msgctl(main_msg_queue, 0, IPC_RMID);

    return -1;
  }
  printf("<LOG> Time to destroy repo...\n");
  semDown(repo_sem);

  //repo
  shmctl(repo_id, 0, IPC_RMID);
  printf("<LOG> Repo Removed (id: %d)\n",repo_id);

  if(main_msg_queue!=-1) {
    msgctl(main_msg_queue, IPC_RMID);
    printf("<LOG> Main servers queue removed (id: %d)\n",main_msg_queue);
  }

  semUp(repo_sem);
  printf("<LOG> Deleting Repo Semaphore (id: %d)\n",repo_sem);
  semctl(repo_sem, 0, IPC_RMID);

  if(log_sem==-1) printf("<LOG> Log file semaphore has been removed earlier\n");
  else {
    semDown(log_sem);
    printf("<LOG> Deleting Log Semaphore (id: %d)\n",log_sem);
    semctl(log_sem, 0, IPC_RMID);
  }
  return 0;
}

int unregisterServer() {

  semDown(repo_sem);

  int servers_left = repo->active_servers-1;

  if(forClients()!=-1) {
    int server_iterator=findServerIterator(forClients());
    printf("<LOG> Server iterator [%d]\n",server_iterator);
    int clients_left=repo->servers[server_iterator].clients;
    repo->active_servers--;
    printf("<LOG> There are [%d] more clients still on this server\n",clients_left);
    if(clients_left!=0) {
      int i;
      for(i=0; i<repo->active_clients; i++) {
        if(repo->clients[i].server_id==forClients()) forceToLogout(repo->clients[i].name);
      }
    }
    repo->servers[server_iterator].clients=repo->servers[servers_left].clients;
    repo->servers[server_iterator].client_msgid=repo->servers[servers_left].client_msgid;
    repo->servers[server_iterator].server_msgid=repo->servers[servers_left].server_msgid;

    sort_servers();
  }

  printf("bug3\n");


  semUp(repo_sem);

  logger(2,forClients(),"");

  return servers_left;
}

void registerServer() {

  semDown(repo_sem);
  repo->active_servers++;
  repo->servers[repo->active_servers-1].clients=0;
  repo->servers[repo->active_servers-1].client_msgid=forClients();
  repo->servers[repo->active_servers-1].server_msgid=forServers();

  sort_servers();
  semUp(repo_sem);

  logger(1,forClients(),"");
}

void clientHearbeat() {
  int i,res,time_break=0;

  STATUS_RESPONSE resp;
  resp.type = HEARTBEAT;
  resp.status=forClients();

  CLIENT_REQUEST req;

  int current_client=0,ownQueue,active_clients,myClients;
  char client_name[MAX_NAME_SIZE+1];

  while (1) {

    resp.status=forClients();

    if((myClients=clientsOnMe())>0) {
      semDown(server_sem);
        int clients=server_repo->clients;
      semUp(server_sem);
      if (clients > 0) {
        current_client++;
        current_client %= server_repo->clients;

      }
      else current_client=-1;
      time_break++;
      semDown(server_sem);
        ownQueue = server_repo->clients_pairs[current_client].own_queue;
        strcpy(client_name, server_repo->clients_pairs[current_client].name);
      semUp(server_sem);

      msgsnd(ownQueue, &resp, sizeof(res), 0);

      sleep(TIMEOUT);

      res=msgrcv(resp.status, &req, sizeof(req), HEARTBEAT, IPC_NOWAIT);

      if(res==-1) {
        forceToLogout(client_name);
        printf("<LOG> <%s> forced to logout\n",client_name);

        logger(5,forClients(),client_name);
      }
    }
  }
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
  return -1;
}

int main() {

  pid_t pids[MAX_FORKS];
  int p=0;

  initLocalRepo();
  if(initRepo()<0) destroyRepo();
  else {
    registerServer();
    showServersData();
    if ((pids[p++] = fork()) == 0) clientHearbeat();
    else if ((pids[p++] = fork()) == 0) sendServersList();
    else if ((pids[p++] = fork()) == 0) loginClient();
    else if ((pids[p++] = fork()) == 0) logoutClient();
    else if ((pids[p++] = fork()) == 0) globalClientsList();
    else if ((pids[p++] = fork()) == 0) roomClientsList();
    else if ((pids[p++] = fork()) == 0) changeRoom();
    else if ((pids[p++] = fork()) == 0) roomsList();
    else if ((pids[p++] = fork()) == 0) publicMessage();
    else if ((pids[p++] = fork()) == 0) broadcastPubilcMessage();
    else if ((pids[p++] = fork()) == 0) privateMessage();
    else if ((pids[p++] = fork()) == 0) broadcastPrivateMessage();
    else  {
      char input[MAX_MSG_SIZE];
      do {
        scanf("%s", input);
      } while (strcmp(input, "-q") != 0);

      int i;
      semDown(server_sem);
      for (i=0; i<p; i++) {
        kill(pids[i], SIGKILL);
      }
      semUp(server_sem);
    }
    if(unregisterServer()==0) destroyRepo(); //unregisterServer() return number of servers left
    destroyServer();
  }
  return 0;
}





