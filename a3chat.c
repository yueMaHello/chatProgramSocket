//
//  a3chat.c
//  cmput379 ass3
//
//  Created by My on 2017-11-02.
//  Copyright © 2017 My. All rights reserved.
//
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>
#include <string.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/file.h>
#include <time.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <sys/signal.h>
#include <semaphore.h>
#define MAX_BUF 256
#define NUM_THREADS 10
#define KAL_char 0x6 // A non-printable character (e.g., ACK)
#define KAL_length 5 // Number of KAL_char in one keepalive message
#define KAL_interval 1.5 // Client sends a keepalive message every 1.5 seconds
#define KAL_count 5
#define MAX_CLIENT 6
#define interval 15
void enable_keepalive(int sock);
int serverSide(char *argv[],int argc);
int clientSide(char *argv[],int argc);
char* concat(const char *s1, const char *s2);
const char* firstCharAfterSpace(const char* input);
const char* firstCharAfterSymbol(const char* input);
void removeChar(char *str, char garbage);
void insertArray(int a[10],int length,int x);
void *communicate(void *sockset);
void *keepalive(void *sockset);
void *onlineUserCheck(void *n);
int hostname_to_ip(char * hostname , char* ip);
//global variables
int nclient;
//store username
char user1[100];
char user2[100];
char user3[100];
char user4[100];
char user5[100];
char *userNameList[] = {user1,user2,user3,user4,user5};
//store user chat list
int user1ChatList[10];
int user2ChatList[10];
int user3ChatList[10];
int user4ChatList[10];
int user5ChatList[10];
int *userChatList[] = {user1ChatList,user2ChatList,user3ChatList,user4ChatList,user5ChatList};
//semaphore
sem_t socketSema;
int usersocketList[MAX_CLIENT];
//this struct is used to print period time report
typedef struct {
    char strVal[MAX_BUF]; // or char strVal[20];
    int sockfd;
    char userName[MAX_BUF];
} lastChatRecord;

lastChatRecord chatTime[MAX_BUF];
int chatTimeIndex=0;

typedef struct _thread_data_t {
    int tid;
    int index;
} thread_data_t;

pthread_t thr[NUM_THREADS];
thread_data_t thr_data[NUM_THREADS];

int threadStatus[MAX_CLIENT];
int serverOnline = 0;
int clientOnline = 0;
int main(int argc, char *argv[]){
    
    char * symble = argv[1];
    //server side
    if (strcmp(symble, "-s") == 0){
        int a = serverSide(argv, argc);
        printf("server stopped\n");
    }
    //client side
    else if(strcmp(symble, "-c") == 0){
        int b =clientSide(argv,argc);
        printf("client stopped\n");
    }
    //something else
    else{
        printf("Wrong format input!");
        return 0;
    }
    return 0;
}
int serverSide(char *argv[],int argc){
    sem_init(&socketSema, 0, 10);
    serverOnline=1;
    thread_data_t thr_data[NUM_THREADS];
    //store user names
    bzero(user1,100);
    bzero(user2,100);
    bzero(user3,100);
    bzero(user4,100);
    bzero(user5,100);
    
    int portno= atoi(argv[2]);
    nclient = atoi(argv[3]);
    int onlineClientNumber = 0;
    int sockfd, newsockfd,clilen;
    struct sockaddr_in serv_addr, cli_addr;
    int n;
    if(nclient>5){
        printf("Maximum 5 clients. Please try again!");
        return 1;
    }
    
    if (argc < 2) {
        fprintf(stderr,"ERROR, no port provided\n");
        exit(1);
    }
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("ERROR opening socket");
        exit(1);
    }
    if (sockfd < 0){
        perror("ERROR opening socket");
    }
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(portno);
    
    if (bind(sockfd, (struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0){
        perror("ERROR on binding");}
    //create thread to display online users' status every 15 seconds
    threadStatus[0] = 1;
    for(int i=0;i<MAX_BUF;i++){
        bzero(chatTime[i].strVal,MAX_BUF);
        bzero(chatTime[i].userName,MAX_BUF);
    }
    //create a thread to handle the keepalive message
    pthread_create(&thr[0], NULL, onlineUserCheck, &thr_data[0]);
    listen(sockfd,1);
    clilen = sizeof(cli_addr);
    
    char promptMessage[MAX_BUF];
    bzero(promptMessage,MAX_BUF);
    sprintf(promptMessage,"Chat server begins  [port = %d] [nclient = %d]\n",portno,nclient);
    write(STDOUT_FILENO,promptMessage,strlen(promptMessage));

    while(1){
        int selectResult;
        fd_set readSocket;
        struct timeval timeInterval;
        FD_ZERO(&readSocket);
        FD_SET(sockfd,&readSocket);
        timeInterval.tv_sec = 0;
        timeInterval.tv_usec = 600;
        
        int selectSTDIN;
        fd_set readSTDIN;
        FD_ZERO(&readSTDIN);
        FD_SET(STDIN_FILENO,&readSTDIN);
        //detect sockfd
        selectResult = select(sockfd+1, &readSocket, NULL, NULL, &timeInterval);
        if(selectResult){
            newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
            if(newsockfd<0){
                perror("accept");
                exit(EXIT_FAILURE);
            }
            if (newsockfd>=0){
                for(int i=0;i<MAX_CLIENT;i++){
                    if(threadStatus[i]!=1){
                        thr_data[i].tid = newsockfd;
                        thr_data[i].index = i;
                        //create a thread to a client
                        pthread_create(&thr[i], NULL, communicate, &thr_data[i]);
                        threadStatus[i]=1;
                        break;
                    }
                }
            }
        }
        //detect stdin
        selectSTDIN = select(1, &readSTDIN, NULL, NULL, &timeInterval);
        if(selectSTDIN){
            char stdinBuffer[MAX_BUF];
            n = read(STDIN_FILENO,stdinBuffer,sizeof(stdinBuffer));
            //exit the server
            if(strncmp("exit", stdinBuffer, strlen("exit"))==0){
                break;
            }
        }
    }
    return 1;
    
}
void *onlineUserCheck(void *n){
    while(1){
        const char msg5[] = "...\nactivity report:\n";
        write(STDOUT_FILENO,msg5,sizeof(msg5)-1);
        for(int i=0;i<MAX_BUF;i++){
            if(strlen(chatTime[i].strVal)>0){
                write(STDOUT_FILENO,chatTime[i].strVal,MAX_BUF);
            }
        }
        const char msg6[] = "...\n";
        write(STDOUT_FILENO,msg6,sizeof(msg6)-1);
        sleep(interval);
    }
    return NULL;
}

void *communicate(void *sockset){
    int thisUserIndex;
    int kal_count = 0;
    time_t lastTimeReceiveKeepalive;
    time_t lastTimeCheckKeepalive;
    time_t kal_interval = KAL_interval;
    
    struct _thread_data_t *my_para = (struct _thread_data_t*)sockset;
    int sockfd =my_para->tid;
    int index =my_para->index;
    
    while(1){

        if(kal_count>=KAL_count){
            write(STDOUT_FILENO,"some client has lost connecting \n",strlen("some client lost connecting \n "));
            for(int i=0;i<MAX_BUF;i++){
                char s[64];
                time_t t = time(NULL);
                struct tm *tmp = localtime(&t);
                strftime(s, 64, "%r, %a %b %d, %Y", tmp);
                if(strcmp(chatTime[i].userName,userNameList[thisUserIndex])==0 && chatTime[i].sockfd == sockfd){
                    sprintf(chatTime[i].strVal ,"'%s' [sockfd = %d]: loss of keepalive messages detected at %s\n",userNameList[thisUserIndex],sockfd,s);
                    break;
                }
            }
            //release all the resource of this bad client
            bzero(userNameList[thisUserIndex],100);
            bzero(userChatList[thisUserIndex],10);
            for(int i = 0;i<nclient;i++){
                int deleteInteger = thisUserIndex+1;
                for(int n=0;n<10;n++){
                    if(userChatList[i][n]==deleteInteger){
                        userChatList[i][n]=0;
                    }
                }

            }
            close(sockfd);
            threadStatus[index] = 0;
            pthread_exit(0);
        }
        
        int n;
        char buffer[MAX_BUF];
        memset(buffer, '\0', sizeof(buffer));
        n = recv(sockfd,buffer,MAX_BUF,MSG_DONTWAIT);
        if(strncmp("close",buffer,strlen("close"))==0){
            
            char s[64];
            time_t t = time(NULL);
            struct tm *tmp = localtime(&t);
            strftime(s, 64, "%r, %a %b %d, %Y", tmp);
            for(int i=0;i<MAX_BUF;i++){
                if(strcmp(chatTime[i].userName,userNameList[thisUserIndex])==0 && sockfd == chatTime[i].sockfd ){
                    sprintf(chatTime[i].strVal ,"'%s' [sockfd = %d]: %s (This user has closed the chat.)\n",userNameList[thisUserIndex],sockfd,s);
                    break;
                }
            }
            //release all the resource of this bad client
            bzero(userNameList[thisUserIndex],100);
            bzero(userChatList[thisUserIndex],10);
            for(int i = 0;i<nclient;i++){
                int deleteInteger = thisUserIndex+1;
                for(int n=0;n<10;n++){
                    if(userChatList[i][n]==deleteInteger){
                        userChatList[i][n]=0;
                    }
                }
            }
            write(STDOUT_FILENO,"[server] done ",strlen("[server] done "));
            n = write(sockfd,"[server] done",MAX_BUF);
            if (n < 0) perror("ERROR writing to socket");
            close(sockfd);
            threadStatus[index] = 0;
            pthread_exit(0);
        }
        else if(strncmp("exit",buffer, strlen("exit"))==0){
            
            char s[64];
            time_t t = time(NULL);
            struct tm *tmp = localtime(&t);
            strftime(s, 64, "%r, %a %b %d, %Y", tmp);
            for(int i=0;i<MAX_BUF;i++){
                if(strcmp(chatTime[i].userName,userNameList[thisUserIndex])==0 && sockfd == chatTime[i].sockfd ){
                    sprintf(chatTime[i].strVal ,"'%s' [sockfd = %d]: %s (This user has exited.)\n",userNameList[thisUserIndex],sockfd,s);
                    break;
                }
            }
            bzero(userNameList[thisUserIndex],100);
            bzero(userChatList[thisUserIndex],10);
            for(int i = 0;i<nclient;i++){
                int deleteInteger = thisUserIndex+1;
                for(int n=0;n<10;n++){
                    if(userChatList[i][n]==deleteInteger){
                        userChatList[i][n]=0;
                    }
                }
                printf("\n");
            }

            close(sockfd);
            threadStatus[index] = 0;
            pthread_exit(0);
        }
        else if(strncmp("open ",buffer,strlen("open "))==0){
            
            int foundEmptySlot = 0;

            for(int i=0;i<nclient;i++){
                
                const char* userName =  firstCharAfterSpace(buffer);
                removeChar((char*)userName,'\n');
                for(int i=0;i<nclient;i++){
                    if(strcmp(userNameList[i],userName)==0){
                        char message[MAX_BUF];
                        const char msg11[] = "[server] Error: Duplicated username! Please use another name!\n";
                        write(STDOUT_FILENO,msg11,sizeof(msg11)-1);
                        
                        sprintf(message,"[server] Error: Duplicated username! Please use another name!\n");
                        n = write(sockfd,message,strlen(message));
                        close(sockfd);
                        threadStatus[index] = 0;
                        pthread_exit(0);
                        
                    }
                }
                
                
                if(strlen(userNameList[i])==0){
                    foundEmptySlot =1;
                    sem_wait(&socketSema);
                    sprintf(userNameList[i],"%s",userName);
                    thisUserIndex = i;
                    usersocketList[i] = sockfd;
                    sem_post(&socketSema);
                    
                    char message[MAX_BUF];
                    const char msg12[] ="[server] connected\n[server] User '%s' logged in";
                    write(STDOUT_FILENO,msg12,sizeof(msg12)-1);
                    sprintf(message,"[server] connected\n[server] User '%s' logged in",userName);
                    lastTimeReceiveKeepalive = time(NULL);
                    lastTimeCheckKeepalive = time(NULL);
                    n = write(sockfd,message,strlen(message));
                    break;
                }
                
            }
            if(foundEmptySlot==0){
                char message[MAX_BUF];
                const char msg13[] ="[server] Error: Maximum number of clients exceeds. Please try again later!";
                write(STDOUT_FILENO,msg13,sizeof(msg13)-1);
                sprintf(message,"[server] Error: Maximum number of clients exceeds. Please try again later!");
                n = write(sockfd,message,strlen(message));
                
                close(sockfd);
                threadStatus[index] = 0;
                pthread_exit(0);
            }
            
            char s[64];
            time_t t = time(NULL);
            struct tm *tmp = localtime(&t);
            strftime(s, 64, "%r, %a %b %d, %Y", tmp);
            for(int i=0;i<MAX_BUF;i++){
                if(strlen(chatTime[i].strVal)==0){
                    sprintf(chatTime[i].strVal ,"'%s' [sockfd = %d]: %s\n",userNameList[thisUserIndex],sockfd,s);
                    chatTime[i].sockfd= sockfd;
                    sprintf( chatTime[i].userName,"%s",userNameList[i]);
                    break;
                }
            }
            
        }
        //show current online clients' names
        else if(strncmp("who", buffer, strlen("who"))==0){
            
            char outBuf[MAX_BUF];
            memset(outBuf, '\0', sizeof(outBuf));
            sprintf(outBuf,"[server]: Current user:");
            if(strlen(user1)!=0){
                sprintf(outBuf+ strlen(outBuf),"[%d] %s",1,user1);
            }
            if(strlen(user2)!=0){
                sprintf(outBuf + strlen(outBuf),", [%d] %s",2,user2);
            }
            if(strlen(user3)!=0){
                sprintf(outBuf + strlen(outBuf),", [%d] %s",3,user3);
            }
            if(strlen(user4)!=0){
                sprintf(outBuf + strlen(outBuf),", [%d] %s",4,user4);
            }
            if(strlen(user5)!=0){
                sprintf(outBuf + strlen(outBuf),", [%d] %s",5,user5);
            }
            sprintf(outBuf + strlen(outBuf),"%s","\n");
            printf("%s",outBuf);
            n = write(sockfd, outBuf, strlen(outBuf));
            
            char s[64];
            time_t t = time(NULL);
            struct tm *tmp = localtime(&t);
            strftime(s, 64, "%r, %a %b %d, %Y", tmp);
            for(int i=0;i<MAX_BUF;i++){
                if(strcmp(chatTime[i].userName,userNameList[thisUserIndex])==0&&sockfd == chatTime[i].sockfd ){
                    sprintf(chatTime[i].strVal ,"'%s' [sockfd = %d]: %s\n",userNameList[thisUserIndex],sockfd,s);
                    break;
                }
                
            }
            
        }
        //chat with some clients
        else if(strncmp("to ",buffer,strlen("to"))==0){

            char splitStrings[10][10]; //can store 10 words of 10 characters
            char thisBuffer[MAX_BUF];
            char outBuf[MAX_BUF];
            bzero(thisBuffer,MAX_BUF);
            bzero(outBuf,MAX_BUF);
            int rv = sprintf(thisBuffer, "%s",buffer);
            char *token = strtok(thisBuffer, " \t");
            int i = 0;
            sprintf(outBuf,"[server] recipients added: ");
            while(token){
                char s[100];
                memset(s,'\0',sizeof(s));
                sprintf(s,"%s", token);
                int size = strlen(s)-1;
                if(strncmp(s,user1,size)==0 && strncmp(s,user1,strlen(user1))==0 ){
                    insertArray(userChatList[thisUserIndex],9,1);
                    sprintf(outBuf + strlen(outBuf),"%s ",s);
                }
                else if(strncmp(s,user2,size)==0 && strncmp(s,user2,strlen(user2))==0 ){
                    insertArray(userChatList[thisUserIndex],9,2);
                    sprintf(outBuf + strlen(outBuf),"%s ",s);
                    
                }
                else if( strncmp(s,user3,size)==0 && strncmp(s,user3,strlen(user3))==0 ){
                    insertArray(userChatList[thisUserIndex],9,3);
                    sprintf(outBuf + strlen(outBuf),"%s ",s);
                }
                else if( strncmp(s,user4,size)==0 && strncmp(s,user4,strlen(user4))==0 ){
                    insertArray(userChatList[thisUserIndex],9,4);
                    sprintf(outBuf + strlen(outBuf),"%s ",s);
                }
                else if( strncmp(s,user5,size)==0 && strncmp(s,user5,strlen(user5))==0 ){
                    insertArray(userChatList[thisUserIndex],9,5);
                    sprintf(outBuf + strlen(outBuf),"%s ",s);
                }
                token = strtok(NULL, " \t");
            }
            sprintf(outBuf+strlen(outBuf), "%s","\n");
            write(STDOUT_FILENO, outBuf, sizeof(outBuf));
            write(sockfd, outBuf, sizeof(outBuf));
            
            char s[64];
            time_t t = time(NULL);
            struct tm *tmp = localtime(&t);
            strftime(s, 64, "%r, %a %b %d, %Y", tmp);
            for(int i=0;i<MAX_BUF;i++){
                if(strcmp(chatTime[i].userName,userNameList[thisUserIndex])==0&&sockfd == chatTime[i].sockfd ){
                    sprintf(chatTime[i].strVal ,"'%s' [sockfd = %d]: %s\n",userNameList[thisUserIndex],sockfd,s);
                    break;
                }
                
            }
            
        }
        //send chat messages
        else if(strncmp(buffer,"<",strlen("<"))==0){
            
            char someBuf[MAX_BUF];
            const char* userWords =  firstCharAfterSymbol(buffer);
            n = write(sockfd,"[server] Messsage has been sent.\n ", strlen("[server] Message has been sent.\n "));
            memset(someBuf, '\0', sizeof(someBuf));
            sprintf(someBuf,"[Dell]%s",userWords);
            write(STDOUT_FILENO,someBuf,strlen(someBuf));
            for (int n=0;n<10;n++){
                if(userChatList[thisUserIndex][n]==0){
                    continue;
                }
                int socketn;
                sem_wait(&socketSema);
                socketn = usersocketList[userChatList[thisUserIndex][n]-1];
                sem_post(&socketSema);
                printf("socket %d\n",socketn);
                write(socketn,someBuf,sizeof(someBuf));
                write(STDOUT_FILENO,someBuf,sizeof(someBuf));
            }
            
            char s[64];
            time_t t = time(NULL);
            struct tm *tmp = localtime(&t);
            strftime(s, 64, "%r, %a %b %d, %Y", tmp);
            for(int i=0;i<MAX_BUF;i++){
                if(strcmp(chatTime[i].userName,userNameList[thisUserIndex])==0&&sockfd == chatTime[i].sockfd ){
                    sprintf(chatTime[i].strVal ,"'%s' [sockfd = %d]: %s\n",userNameList[thisUserIndex],sockfd,s);
                    break;
                }
            }
        }
        //receive keepalive messages from the client
        else if(strncmp(buffer,"0x6,0x6,0x6,0x6,0x6",strlen("0x6,0x6,0x6,0x6,0x6"))==0){
            time_t nowTime = time(NULL);
            lastTimeReceiveKeepalive = time(NULL);
            lastTimeCheckKeepalive = time(NULL);
            kal_count = 0;
            continue;
        }
        //if didn't receive keepalive messages
        time_t nowTime = time(NULL);
        if(nowTime-lastTimeCheckKeepalive>kal_interval){
            kal_count+=1;
            lastTimeCheckKeepalive = time(NULL);
        }
    

    }
    return NULL;
    
}

int clientSide(char *argv[],int argc){

    int portno = atoi(argv[2]);
    char * serveraddress= argv[3];
    char ip[MAX_BUF];
    hostname_to_ip(serveraddress, ip);
    int sockfd=0;
    int n;
    int exit_client = 0;
    
    struct sockaddr_in serv_addr;
    struct hostent *server;
    
    char buffer[MAX_BUF];
    if (argc < 3) {
        fprintf(stderr,"usage %s hostname port\n", argv[0]);
        exit(0);
    }
    portno = atoi(argv[2]);
    
    char promptMessage[MAX_BUF];
    bzero(promptMessage,MAX_BUF);
    sprintf(promptMessage,"Chat client begins (server '%s' [%s], port %d)\n",serveraddress,ip,portno);
    write(STDOUT_FILENO,promptMessage,strlen(promptMessage));
    
    write(STDOUT_FILENO,"a3chat_client: ",strlen("a3chat_client: "));
    while(1){
        int selectResult;
        //set for 'select'
        fd_set readSTDIN;
        struct timeval timeInterval;
        FD_ZERO(&readSTDIN);
        FD_SET(STDIN_FILENO,&readSTDIN);
        timeInterval.tv_sec = 0;
        timeInterval.tv_usec = 500;
    
        selectResult = select(1, &readSTDIN, NULL, NULL, &timeInterval);
        
        if(selectResult){
            memset(buffer, '\0', sizeof(buffer));
            read(STDIN_FILENO,buffer,MAX_BUF);

            if(strncmp(buffer, "open ", strlen("open "))!=0 && sockfd<=0){
                const char msg5[] = "You haven't open a user! Please try again and open a user firstly!\na3chat_client: ";
                write(STDOUT_FILENO,msg5,sizeof(msg5)-1);
                continue;
            }
            else if(strncmp(buffer,"open ",strlen("open "))==0 && sockfd>0){
                const char msg5[] = "You have already open an account! Please continue your chat.\na3chat_client: ";
                write(STDOUT_FILENO,msg5,sizeof(msg5)-1);
                continue;
            }
            else if(strncmp(buffer,"open ",strlen("open "))==0){
                sockfd = socket(AF_INET, SOCK_STREAM, 0);
                if (sockfd < 0)
                    perror("ERROR opening socket");
                server = gethostbyname(argv[3]);
                if (server == NULL) {
                    fprintf(stderr,"ERROR, no such host\n");
                    exit(0);
                }
                bzero((char *) &serv_addr, sizeof(serv_addr));
                serv_addr.sin_family = AF_INET;
                bcopy((char *)server->h_addr,
                      (char *)&serv_addr.sin_addr.s_addr,
                      server->h_length);
                serv_addr.sin_port = htons(portno);
                if (connect(sockfd,(struct sockaddr *)&serv_addr,sizeof(serv_addr)) < 0)
                    perror("ERROR connecting. Server is not running.");
                //send 'open username' to the server
                n = write(sockfd,buffer,strlen(buffer));
                memset(buffer, '\0', sizeof(buffer));
                sleep(0.5);
                n = recv(sockfd,buffer,MAX_BUF,0);
                if(strncmp(buffer,"[server] Error: Duplicated username!",10)==0){
                    write(STDOUT_FILENO,buffer,strlen(buffer));
                    close(sockfd);
                    sockfd = 0;
                    const char msg5[] = "a3chat_client: ";
                    write(STDOUT_FILENO,msg5,sizeof(msg5)-1);
                    continue;
                }

                printf("%s\n",buffer);
                const char mg5[] = "a3chat_client: ";
                write(STDOUT_FILENO,mg5,sizeof(mg5)-1);
                if(sockfd>0){
                    thr_data[0].tid = sockfd;
                    pthread_create(&thr[0], NULL, keepalive, &thr_data[0]);
                }
                else{
                    continue;
                }
            }

            else if(strncmp(buffer, "close", strlen("close"))==0){
                n = write(sockfd,buffer,strlen(buffer));
                memset(buffer, '\0', sizeof(buffer));
                sleep(0.5);
                n = recv(sockfd,buffer,MAX_BUF,0);
                printf("%s\n",buffer);
                close(sockfd);
                sockfd = 0;
                const char msg5[] = "a3chat_client: ";
                write(STDOUT_FILENO,msg5,sizeof(msg5)-1);
                continue;
            }
            else if(strncmp(buffer, "exit", strlen("exit"))==0){
                n = write(sockfd,buffer,strlen(buffer));
                memset(buffer, '\0', sizeof(buffer));
                sleep(0.5);
                n = recv(sockfd,buffer,MAX_BUF,0);
                printf("%s\n",buffer);
                close(sockfd);
                sockfd = 0;
                exit_client = 1;
            }
            else{
                n = write(sockfd,buffer,strlen(buffer));
                memset(buffer, '\0', sizeof(buffer));
                sleep(0.5);
                n = recv(sockfd,buffer,MAX_BUF,0);
                printf("%s\n",buffer);
                const char msg5[] = "a3chat_client: ";
                write(STDOUT_FILENO,msg5,sizeof(msg5)-1);
            }

            if(exit_client == 1){
                return 1;
            }
            
            
        }
        else{
            int selectSocket;
            //set 'select'
            fd_set readSocket;
            FD_ZERO(&readSocket);
            FD_SET(sockfd,&readSocket);
       
            selectSocket = select(sockfd+1, &readSocket, NULL, NULL, &timeInterval);
            if(selectSocket==-1){
                perror("select error");
            }
            else if(selectSocket){
                int readContent=recv(sockfd,buffer,MAX_BUF,0);
                if(readContent>0 && strncmp("[Dell]", buffer, strlen("[Dell]"))==0 ){
                    write(STDOUT_FILENO,buffer,strlen(buffer));
                    const char msg5[] = "a3chat_client: ";
                    write(STDOUT_FILENO,msg5,sizeof(msg5)-1);
                }
            }
        }
    }
    return 1;
    
}
void *keepalive(void *sockset){
    struct _thread_data_t *my_para = (struct _thread_data_t*)sockset;
    int sockfd =my_para->tid;
    char KAL_message[KAL_length] = {KAL_char,KAL_char,KAL_char,KAL_char,KAL_char};
    while(1){
        write(sockfd,"0x6,0x6,0x6,0x6,0x6",strlen("0x6,0x6,0x6,0x6,0x6"));
        sleep(1.5);
    }
    pthread_exit(0);
    return NULL;
}
char* concat(const char *s1, const char *s2){
    const size_t len1 = strlen(s1);
    const size_t len2 = strlen(s2);
    char *result = malloc(len1+len2+1);
    memcpy(result, s1, len1);
    memcpy(result+len1, s2, len2+1);
    return result;
}
const char* firstCharAfterSpace(const char* input) {
    const char* starting = input;
    while (*starting != ' ') {
        starting++;
    }
    // first one _after_
    return starting+1;
}
const char* firstCharAfterSymbol(const char* input) {
    const char* starting = input;
    while (*starting != '<') {
        starting++;
    }
    // first one _after_
    return starting+1;
}
void removeChar(char *str, char garbage) {
    char *src, *dst;
    for (src = dst = str; *src != '\0'; src++) {
        *dst = *src;
        if (*dst != garbage) dst++;
    }
    *dst = '\0';
}
void insertArray(int a[10],int length,int x){
    int i,n;
    n=0;
    for (i=length;i>n;i--){ a[i]=a[i-1];}
    a[n]=x;
    
}


int hostname_to_ip(char * hostname , char* ip)
{
    struct hostent *he;
    struct in_addr **addr_list;
    int i;
    
    if ( (he = gethostbyname( hostname ) ) == NULL)
    {
        // get the host info
        herror("gethostbyname");
        return 1;
    }
    
    addr_list = (struct in_addr **) he->h_addr_list;
    
    for(i = 0; addr_list[i] != NULL; i++)
    {
        //Return the first one;
        strcpy(ip , inet_ntoa(*addr_list[i]) );
        return 0;
    }
    
    return 1;
}
