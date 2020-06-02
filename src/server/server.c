#define _DEBUG

#include "server.h"
#include "debug.h"
#include "protocol.h"
#include "ClientLinkList.h"
#include "RoomLinkList.h"
#include "log.h"

# include <semaphore.h>
#include <pthread.h>
#include <signal.h>

const char exit_str[] = "exit";

char buffer[BUFFER_SIZE];
pthread_mutex_t buffer_lock;

pthread_mutex_t UserList_lock;
pthread_mutex_t RoomList_lock;

//semaphores
sem_t empty, full, mutex;
// message queue
MessageData MessageQueue[QUEUE_SIZE];

// 消息队列指针
int in=0, out=0;

int total_num_msg = 0;
int listen_fd;

// room list
Room * RoomList;

//有限缓存插入--生产
int insert_item(MessageData item) {
    /* insert item into MessageQueue */
    MessageQueue[out] = item;
    out = (out + 1) % QUEUE_SIZE;
    return 0;
}

//有限缓存删除--消费
int remove_item(MessageData *item) {
    /* remove an object from MessageQueue and then place it in item */
    *item = MessageQueue[in];
    in = (in + 1) % QUEUE_SIZE;

    return 0;   
}

void sigint_handler(int sig) {
    printf("shutting down server\n");
    close(listen_fd);
    exit(0);
}

int server_init(int server_port) {
    int sockfd;
    struct sockaddr_in servaddr;

    // socket create and verification
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        printf("socket creation failed...\n");
        exit(EXIT_FAILURE);
    } else
        printf("Socket successfully created\n");

    bzero(&servaddr, sizeof(servaddr));

    // assign IP, PORT
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(server_port);

    int opt = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, (char *)&opt, sizeof(opt)) < 0) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    // Binding newly created socket to given IP and verification
    if ((bind(sockfd, (SA *)&servaddr, sizeof(servaddr))) != 0) {
        printf("socket bind failed\n");
        exit(EXIT_FAILURE);
    } else
        printf("Socket successfully binded\n");

    // Now server is ready to listen and verification
    if ((listen(sockfd, 1)) != 0) {
        printf("Listen failed\n");
        exit(EXIT_FAILURE);
    } else
        printf("Server listening on port: %d.. Waiting for connection\n", server_port);

    return sockfd;
}

int _send_msg(int clientfd, enum msg_types type, char *body){
    // send message OK

    petr_header *return_header = (petr_header *)malloc(sizeof(petr_header));
    
    return_header->msg_type = type;
    // add temiateing charactor /0
    return_header->msg_len = strlen(body);

    int ret = wr_msg(clientfd, return_header, body);

    LogWrite(INFO, "Send message [%s] to client [%d]", body, clientfd);

    if (ret < 0) {
        error("Sending failed\n");
    }else{
        debug("Send the type[%d] message[%s] back to client[%d]\n", type, body, clientfd);
    }
    free(return_header);
    return ret;
    
}

void login(MessageData * msg){
    char * username = msg->mText;
    
    if( FindClientByName(username) < 0){
        debug("no user named as %s \n", username);
        // add user into client list 
        Client * client = (Client *)malloc(sizeof(Client));
        strcpy(client->userName, username);
        client->flag = msg->clientfd;
        client->sClient = msg->clientfd;
        AddClient(UserList, client);
        LogWrite(INFO, "Add User named [%s]", username);
        _send_msg(msg->clientfd, OK, "");
        
    }else{
        debug("user [%s] has already log in\n!", username);
        _send_msg(msg->clientfd, EUSREXISTS, "LOGIN error");
        LogWrite(INFO, "User login denied ， named [%s] already in UserList", username);
    }
}

void logout(MessageData * msg){
    removeClientBySystem(msg->clientfd);
    _send_msg(msg->clientfd, OK, "");
    debug("remove user[%d]\n", msg->clientfd);
    // close socket
    close(msg->clientfd);  
}


void listUsers(MessageData *msg){
    char response[BUFFER_SIZE];
    bzero(response, BUFFER_SIZE);

    int iCount = 0;
    pClient pCur = UserList->next;

    while (pCur != NULL){
        if(strcmp(pCur->userName, FindClient(UserList , msg->clientfd)->userName)==0){
            
        }else{
            iCount++;
            strcat(response, pCur->userName);
            strcat(response, "\n");
        }
        pCur = pCur->next;
    }

    strcat(response, "\0");

    if(iCount == 0){
        _send_msg(msg->clientfd, USRLIST, "");
        return;
    }else{
        _send_msg(msg->clientfd, USRLIST, response);
    }
    
}

// send message to user
// message:
// <to_username>\r\n<message>
void userSend(MessageData * msg){

    char * msg_buff;

    char * to_username = strtok_r(msg->mText, SPETATOR, &msg_buff);
    debug("to_username:%s\n", to_username);
    char * content  = strtok_r(NULL, SPETATOR, &msg_buff);
    debug("content:%s\n", content);

    Client * client = FindClient(UserList ,msg->clientfd);
    int retClientFd = FindClientByName(to_username);
    if(retClientFd < 0){
        info("NO user named %s \n", to_username);
        _send_msg(client->sClient, ERMNOTFOUND, "");
        return;
    }else{
        char response[BUFFER_SIZE];
        bzero(response, BUFFER_SIZE);

        Client * toClient = FindClient(UserList ,retClientFd);

        // build message
        strcat(response, client->userName);
        strcat(response, SPETATOR);
        strcat(response, content);

        // send Message to username
        _send_msg(toClient->sClient, USRRECV, response);    
        info("Send msg[%s] from [%s] to [%s]\n", response, client->userName, to_username);
    }
    

}

void roomCreate(MessageData *msg){
    char * roomName = msg->mText;
    Room * room = FindRoomByName(RoomList, roomName);
    // is roomName exist in roomList
    if(NULL != room){
        info("Room [%s] already in room list\n", roomName);
        _send_msg(msg->clientfd, ERMEXISTS, "");
    }else{
        Client * creator = FindClient(UserList ,msg->clientfd);
        room = createNewRoom(creator, roomName);
        info("Room [%s] created!\n", roomName);
        _send_msg(msg->clientfd, OK, "");
    }
}

void roomDelete(MessageData *msg){
    char * roomName = msg->mText;
    Room * room = FindRoomByName(RoomList, roomName);
    Client * client = FindClient(UserList ,msg->clientfd);
    // is roomName exist in roomList
    if(NULL == room){
        _send_msg(msg->clientfd, ERMNOTFOUND, "");
        info("Room [%s] not in room list\n", roomName);
    }else if(strcmp(room->creator->userName, client->userName)==0) {
        // send closed room message to all users in this room 
        Client * usersList = room->usersList;
        pClient pCur = usersList->next;

        while (pCur != NULL){
            if(strcmp(pCur->userName, client->userName)==0){
                
            }else{
                _send_msg(pCur->sClient, RMCLOSED, roomName);
                info("send RMCLOSED to user[%s]\n", pCur->userName);
            }
            pCur = pCur->next;
        }
        // remove room in List
        RemoveRoom(RoomList, roomName);
        // send ok to client
        info("send OK to user[%s]\n", client->userName);
        _send_msg(client->sClient, OK, "");
        
    }else{
        _send_msg(msg->clientfd, ERMDENIED, "");
        info("Room [%s] \'s creator is not [%s]\n", roomName, client->userName);
    }

}

void roomsGet(MessageData * msg){
    char response[BUFFER_SIZE];
    bzero(response, BUFFER_SIZE);

    Room * pCur = RoomList->next;
    while(pCur != NULL){
        strcat(response, pCur->roomName);
        strcat(response, ":");

        Client * userList = pCur->usersList;
        Client * userPCur = userList->next;
        while(userPCur!=NULL){
            strcat(response, userPCur->userName);
            strcat(response, ",");
            userPCur = userPCur->next;
        }
        // last chara ","" inside to "\n"
        response[strlen(response)-1] = '\n';
        pCur = pCur->next;
    }
    strcat(response, "\0");
    _send_msg(msg->clientfd, RMLIST, response);
}

void joinRoom(MessageData * msg){
    char * roomName = msg->mText;
    Room * room = FindRoomByName(RoomList, roomName);
    Client * client = FindClient(UserList ,msg->clientfd);
    // is roomName exist in roomList
    if(NULL == room){
        _send_msg(msg->clientfd, ERMNOTFOUND, "");
        info("Room [%s] not found\n", roomName);
    }else if(CountConRoom(room)>=ROOMLIMIT){
        _send_msg(msg->clientfd, ERMFULL, "");
        info("Room [%s] full\n", roomName);
    }else if(NULL != FindClient(room->usersList ,msg->clientfd)){
         // TODO 不能同时进入两次room
        _send_msg(msg->clientfd, ERMDENIED, "");
        info("User [%s] already in Room [%s]\n", client->userName, roomName);
    }else{
        // add user to room usersList
        AddClientInRoom(room, client);
        _send_msg(msg->clientfd, OK, "");
        info("Room [%s] add user [%s]\n", roomName, client->userName);
    }
}

void leaveRoom(MessageData * msg){
    char * roomName = msg->mText;
    Room * room = FindRoomByName(RoomList, roomName);
    Client * client = FindClient(UserList, msg->clientfd);
    // is roomName exist in roomList
    if(NULL == room){
        _send_msg(msg->clientfd, ERMNOTFOUND, "");
        info("Room [%s] not found\n", roomName);
    }else if(NULL == FindClient(room->usersList, msg->clientfd)){
        _send_msg(msg->clientfd, OK, "");
        info("User [%s] not in Room [%s]\n", client->userName, roomName);
    }else if(strcmp(room->creator->userName, client->userName)==0){
        _send_msg(msg->clientfd, ERMDENIED, "");
        info("Room [%s] \'s creator is self [%s]\n", roomName, client->userName);
    }else{
        // remove user from room->usersList
        RemoveClient(room->usersList, client->userName);
        _send_msg(msg->clientfd, OK, "");
        info("User [%s] leaved from Room [%s]\n", client->userName, roomName);
    }
}

void roomSend(MessageData * msg){
    char * msg_buff;

    char * to_roomName = strtok_r(msg->mText, SPETATOR, &msg_buff);
    debug("to_roomName:%s\n", to_roomName);
    char * content  = strtok_r(NULL, SPETATOR, &msg_buff);
    debug("content:%s\n", content);

    Client * client = FindClient(UserList ,msg->clientfd);
    Room * room = FindRoomByName(RoomList, to_roomName);
    if(NULL == room){
        _send_msg(client->sClient, ERMNOTFOUND, "");
        info("Room [%s] not found\n", to_roomName);
    }else if(NULL == FindClient(room->usersList, client->sClient)){
        _send_msg(client->sClient, ERMDENIED, "");
        info("User [%s] not in Room [%s]\n", client->userName, to_roomName);
    }else{
        // send message to all users in the room
        Client * pCur = room->usersList->next;
        // wrapper response  
        char response[BUFFER_SIZE];
        bzero(response, BUFFER_SIZE);
        // <roomname>\r\n<from_username>\r\n<message>
        strcat(response, to_roomName);
        strcat(response, SPETATOR);
        strcat(response, client->userName);
        strcat(response, SPETATOR);
        strcat(response, content);
        while(NULL != pCur){
            if(strcmp(pCur->userName, client->userName) == 0){
               
            }else{
                _send_msg(pCur->sClient, RMRECV, content);
                info("Send Room msg[%s] from [%s] to [%s]\n", response, client->userName, pCur->userName);
            }
            pCur = pCur->next;
        } 
        _send_msg(client->sClient, OK, "");
        info("Send to user [%s] OK", client->userName);
    }
    
}

// 消费者
void* process_Agent(void *params){

    free(params);
    info("created a new thread: pid %u tid %u (0x%x)\n",
        (unsigned int) getpid(), (unsigned int)pthread_self(), (unsigned int)pthread_self());
    LogWrite(INFO, "created a new thread: pid %u tid %u (0x%x)",
        (unsigned int) getpid(), (unsigned int)pthread_self(), (unsigned int)pthread_self());

    while(1){
        sem_wait(&full);
        sem_wait(&mutex);
    
        /* critical section */
        //remove a item
        MessageData msg;
        remove_item(&msg);
        debug("Thread %d: Consumer consume %d,%s\n", msg.clientfd, msg.type, msg.mText);
        LogWrite(INFO, "Removed a job in queue by thread: pid %u tid %u (0x%x)",
            (unsigned int) getpid(), (unsigned int)pthread_self(), (unsigned int)pthread_self());

        pthread_mutex_lock(&RoomList_lock);
        switch(msg.type){
            case LOGIN:
                debug("login request: user[%s]\n", (char *)msg.mText);
                // pthread_mutex_lock(&UserList_lock);
                login(&msg);
                // pthread_mutex_unlock(&UserList_lock);
                break;
            case USRSEND:
                debug("user send request: msg[%s]\n", (char *)msg.mText);
                userSend(&msg);
                break;
            case LOGOUT:
                debug("logout request: user[%d]\n", msg.clientfd);
                logout(&msg);
                break;
            case USRLIST:
                debug("user list request: user[%d]\n", msg.clientfd);
                listUsers(&msg);
                break;
            case RMCREATE:
                debug("room create request: user[%d]\n", msg.clientfd);
                roomCreate(&msg);
                break;
            case RMDELETE:
                debug("room delete request: user[%d] roomName[%s]\n", msg.clientfd, msg.mText);
                roomDelete(&msg);
                break;
            case RMLIST:
                debug("room list request: user[%d]\n", msg.clientfd);
                roomsGet(&msg);
                break;
            case RMJOIN:
                debug("room join request: user[%d] roomName[%s]\n", msg.clientfd, msg.mText);
                joinRoom(&msg);
                break;
            case RMLEAVE:
                debug("room leave request: user[%d] roomName[%s]\n", msg.clientfd, msg.mText);
                leaveRoom(&msg);
                break;
            case RMSEND:
                debug("room send request: user[%d] msg[%s] \n", msg.clientfd, msg.mText);
                roomSend(&msg);
                break;
            default :
                break;

        }
        pthread_mutex_unlock(&RoomList_lock);
        sem_post(&mutex);
        sem_post(&empty);
    }
    LogWrite(INFO, "destroy a thread: pid %u tid %u (0x%x)",
        (unsigned int) getpid(), (unsigned int)pthread_self(), (unsigned int)pthread_self());
    pthread_exit(0);
    
}

// 生产者
//Function running in thread
void *process_client(void *clientfd_ptr) {

    info("created a new thread: pid %u tid %u (0x%x)\n",
        (unsigned int) getpid(), (unsigned int)pthread_self(), (unsigned int)pthread_self());
    LogWrite(INFO, "created a new thread: pid %u tid %u (0x%x)",
        (unsigned int) getpid(), (unsigned int)pthread_self(), (unsigned int)pthread_self());

    int client_fd = *(int *)clientfd_ptr;
    free(clientfd_ptr);
    int received_size;
    fd_set read_fds;

    int retval;

    while (1) {
        FD_ZERO(&read_fds);
        FD_SET(client_fd, &read_fds);
        retval = select(client_fd + 1, &read_fds, NULL, NULL, NULL);
        if (retval != 1 && !FD_ISSET(client_fd, &read_fds)) {
            printf("Error with select() function\n");
            break;
        }

        pthread_mutex_lock(&buffer_lock);

        bzero(buffer, BUFFER_SIZE);
        received_size = read(client_fd, buffer, sizeof(buffer));
        if (received_size < 0) {
            printf("Receiving failed\n");
            pthread_mutex_unlock(&buffer_lock);
            break;
        } else if (received_size == 0) {
            // system close this client gracefully
            Client * pclient = FindClient(UserList, client_fd);
            info("Disconnect from UserName: %s\n", pclient->userName);
            removeClientBySystem(pclient->sClient);
            pthread_mutex_unlock(&buffer_lock);
            break;
        }

        if (strncmp(exit_str, buffer, sizeof(exit_str)) == 0) {
            printf("Client exit\n");
            pthread_mutex_unlock(&buffer_lock);
            break;
        }
        total_num_msg++;
        // print buffer which contains the client contents
        printf("Receive message from client: %s\n", buffer);
        printf("Total number of received messages: %d\n", total_num_msg);

        LogWrite(INFO, "Receive message [%s] from client [%d]", buffer, client_fd);
        
        // 发送消息到消息队列
        // message in queue 
        MessageData msg;
        msg.type = ((petr_header *)buffer)->msg_type;
        msg.clientfd = client_fd;
        strcpy(msg.mText, ((petr_message *)buffer)->body);

        pthread_mutex_unlock(&buffer_lock);
        sem_wait(&empty);
        sem_wait(&mutex);

        /* critical section */
        //add a item
        insert_item(msg);

        LogWrite(INFO, "Insert a new job in queue by thread: pid %u tid %u (0x%x)",
            (unsigned int) getpid(), (unsigned int)pthread_self(), (unsigned int)pthread_self());

        debug("Thread %d: Producer produce %d,%s\n", msg.clientfd, msg.type, msg.mText);

        sem_post(&mutex);
        sem_post(&full);
        
        

    }
    // Close the socket at the end
    printf("Close current client connection\n");
    close(client_fd);

    LogWrite(INFO, "destroy a thread: pid %u tid %u (0x%x)",
        (unsigned int) getpid(), (unsigned int)pthread_self(), (unsigned int)pthread_self());

    pthread_exit(0);
    return NULL;
}


// 系统删除用户信息
void removeClientBySystem(int clientfd){
    Client * client = FindClient(UserList, clientfd);
    // remove all room created by the client
    Room * roomPCur = RoomList->next;
    while(roomPCur != NULL){
         // send closed room message to all users in this room 
        Client * usersList = roomPCur->usersList;
        pClient pCur = usersList->next;

        while (pCur != NULL){
            if(strcmp(pCur->userName, client->userName)==0){
                
            }else{
                _send_msg(pCur->sClient, RMCLOSED, roomPCur->roomName);
                info("send RMCLOSED to user[%s]\n", pCur->userName);
            }
            pCur = pCur->next;
        }
        info("Room [%s] removed which created by user [%s]\n", roomPCur->roomName, client->userName);
        // remove room in List
        RemoveRoom(RoomList, roomPCur->roomName);
        roomPCur = roomPCur->next;
    }
    // remove user in UserList
    info("User [%s] removed from UserList\n", client->userName);
    RemoveClient(UserList, client->userName);
}


void run_server(int server_port, int jobNum) {
    listen_fd = server_init(server_port); // Initiate server and start listening on specified port
    int client_fd;
    struct sockaddr_in client_addr;
    int client_addr_len = sizeof(client_addr);

    //pthread
    pthread_t tid; // the thread identifier

    pthread_attr_t attr; //set of thread attributes

    /* get the default attributes */
    pthread_attr_init(&attr);

    //initial the semaphores
    sem_init(&mutex, 0, 1);
    sem_init(&empty, 0, BUFFER_SIZE);
    sem_init(&full, 0, 0);

    in = out = 0;

    // user list head initialize
    UserList = (Client *)malloc(sizeof(Client)); 
    // room list head initialize
    RoomList = (Room *)malloc(sizeof(Room));


    // initial UserList
    Init();
    // initial RoomList
    InitRoomList(RoomList);
    for(int i = 0; i < jobNum; ++i){
        pthread_create(&tid, &attr, process_Agent, NULL);
    }

    while (1) {
        // Wait and Accept the connection from client
        printf("Wait for new client connection\n");
        int *client_fd = malloc(sizeof(int));
        *client_fd = accept(listen_fd, (SA *)&client_addr, (socklen_t*)&client_addr_len);
        if (*client_fd < 0) {
            printf("server acccept failed\n");
            exit(EXIT_FAILURE);
        } else {
            printf("Client connetion accepted\n");
            pthread_create(&tid, &attr, process_client, (void *)client_fd);
        }
    }
    bzero(buffer, BUFFER_SIZE);
    close(listen_fd);

    //释放信号量
    sem_destroy(&mutex);
    sem_destroy(&empty);
    sem_destroy(&full);

    return;
}

int main(int argc, char *argv[]) {
    int opt;

    unsigned int port = 0;
    unsigned int jobNum = 2;
    while ((opt = getopt(argc, argv, "p:j:")) != -1) {
        switch (opt) {
        case 'p':
            port = atoi(optarg);
            break;
        case 'j':
            jobNum = atoi(optarg);
            break;
        default: /* '?' */
            fprintf(stderr, "Server Application Usage: %s -p <port_number>\n",
                    argv[0]);
            exit(EXIT_FAILURE);
        }
    }
    // set log file name
    strcpy(logFileName, argv[argc-1]);
    debug("log file name: %s\n",  logFileName);

    if (port == 0) {
        fprintf(stderr, "ERROR: Port number for server to listen is not given\n");
        fprintf(stderr, "Server Application Usage: %s -p <port_number>\n",
                argv[0]);
        exit(EXIT_FAILURE);
    }

    run_server(port, jobNum);

    return 0;
}
