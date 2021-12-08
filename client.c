#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <string.h>

int sockfd, filefd, Game=0, myturn=0, oppofd;
char account[30], passwd[30], name[30];
char opponame[100], x[9], le1='O', le2='X';

typedef struct sockaddr SA;

int iswin(char);
int isfair();
void print();
void start();
void* recv_thread(void*);

int main(int argc, char** argv){
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(8080);
	addr.sin_addr.s_addr=htonl(INADDR_ANY);

	if (connect(sockfd, (SA*)&addr, sizeof(addr)) == -1){
		perror("Connect");
		exit(-1);
	}
	printf("Client starts.\n");
	
	if(argc != 1 && strcmp(argv[1], "signup") == 0){
		char newName[100], newPassword[100]; 
		
		printf("new user's name : ");
		scanf("%s", newName);
		printf("new user's password : ");
		scanf("%s", newPassword);
		
		FILE* fp = fopen("password.txt", "r+");
		if(fp == NULL){
			printf("password.txt not found!\n");
			exit(1);
		}
		
		fseek(fp, 0, SEEK_END);
		fprintf(fp, "%s:%s\n", newName, newPassword);
		
		send(sockfd, "new", strlen("new"), 0);
		
		fclose(fp);
		printf("sign up successfully, reopen the program.\n");
		return 0;
	}
	
	printf("Account :");
	scanf("%s", account);
	printf("Password:");
	scanf("%s", passwd);
	
	sprintf(name, "%s:%s@", account, passwd);
	
	start();
	
	return 0;
}

int iswin(char le){
	if(x[0] == le && x[1] == le && x[2] == le)
		return 1;
	if(x[3] == le && x[4] == le && x[5] == le)
		return 1;
	if(x[6] == le && x[7] == le && x[8] == le)
		return 1;
	if(x[0] == le && x[3] == le && x[6] == le)
		return 1;
	if(x[1] == le && x[4] == le && x[7] == le)
		return 1;
	if(x[2] == le && x[5] == le && x[8] == le)
		return 1;
	if(x[0] == le && x[4] == le && x[8] == le)
		return 1;
	if(x[2] == le && x[5] == le && x[6] == le)
		return 1;
	return 0;
}

void print(){
	printf("%c|%c|%c\n", x[0], x[1], x[2]);
	printf("------\n");
	printf("%c|%c|%c\n", x[3], x[4], x[5]);
	printf("------\n");
	printf("%c|%c|%c\n", x[6], x[7], x[8]);
}

int isfair(){
	for(int i = 0 ; i < 9 ; i++)
		if(x[i] == ' ')
			return 0;
	return 1;
}

void start(){
	char buf2[100] = {};
	while(1){
		recv(sockfd, buf2, sizeof(buf2), 0);
		if (strcmp(buf2, "login") == 0){
			send(sockfd, name, strlen(name), 0);
			break;
		}
	}
	
	pthread_t id;
	pthread_create(&id, 0, recv_thread, 0);
	
	char *ptr;
	ptr = strstr(name, ":");
	*ptr = '\0';
	
	int f = 1;
	while(1){
		char buf[100] = {};
		fgets(buf, sizeof(buf), stdin);
		/*Game:1 for game start.      f:1 for first time output*/
		/*myturn:1 for my turn to enter #(0~8)*/
		if(Game == 1 && myturn == 1 && f == 1){
			print();
			printf("Please enter #(0-8)\n");
			f = 0;
		}
		if(Game == 1)
			f = 0;
		
		char *ptr = strstr(buf, "\n");
		*ptr = '\0';
		
		char msg[131] = {};
		if (strcmp(buf, "bye") == 0){
			memset(buf2, 0, sizeof(buf2));
			sprintf(buf2, "%s leaves", name);
			send(sockfd, buf2, strlen(buf2), 0);
			break;
		}
		else if (strcmp(buf, "ls") == 0){
			memset(buf2, 0, sizeof(buf2));
			sprintf(buf2, "ls");
			send(sockfd, buf2, strlen(buf2), 0);
		}
		else if(strcmp(buf, "/clear") == 0){
			system("clear");
		}
		else if(buf[0] == '@'){
			sprintf(msg, "%s", buf);
			send(sockfd, msg, strlen(msg), 0);
		}
		else if(buf[0] == '#')
		{
			if(Game == 0)
				printf("Game not start.\n");
			else if(myturn == 0)
				printf("Is not your turn!\n");
			else{
				int n = atoi(&buf[1]);
				if(x[n] != ' ' || n > 9 || n < 0){
					printf("Please enter another number.#(0-8)\n");
				}
				else{
					x[n] = le1;
					printf("----------\n");
					print();
					
					if(iswin(le1)){
						printf("[You win!]\n\n");
						Game=0;
					}
					else if(isfair()){
						printf("[Fair!]\n\n");
						Game = 0;
					}
					else
						printf("\nWait for your opponent....\n");
					
					myturn = 0;
					
					sprintf(msg, "#%d %d", n, oppofd);
					send(sockfd, msg, strlen(msg), 0);
				}
			}
		}
		else if(strcmp(buf, "yes") == 0){
			sprintf(buf,"AGREE %d",oppofd);
			send(sockfd,buf,strlen(buf),0);
			
			printf("Game Start!\n");
			for(int i = 0 ; i < 9 ; i++)
				x[i]=' ';
			
			le1 = 'O';
			le2 = 'X';
			Game = 1;
			
			if(le1 == 'O')
				myturn=1;
			else
				myturn=0;
			
			if(myturn)
				printf("Press Enter to continue...\n");
			else
				printf("Wait for your opponent....\n");
		}
		else if(strcmp(buf, "print")==0){
			print();
		}
		else if(strncmp(buf, "/all ", 4) == 0){
			/*   /all hi   */
			char* p = buf;
			p = strstr(buf, "all");
			p += 4;
			sprintf(msg,"/all [%s] : %s",name,p);
			send(sockfd,msg,strlen(msg),0);
		}
		else if(strncmp(buf, "/msg ", 4) == 0){
			/*   /msg 4 hi   */
			char* p = buf;
			p = strstr(buf, "msg");
			p += 4;
			
			char char_fd[100], char_len = 0;
			while(*p != ' '){
				char_fd[char_len] = *p;
				char_len++;
				p++;
			}
			char_fd[char_len] = '\0';
			p++;
			int msgfd = atoi(char_fd);
			
			sprintf(msg,"/msg %d [%s] : %s",msgfd,name,p);
			//printf("send %s to server\n", msg);
			send(sockfd,msg,strlen(msg),0);
		}
	}
	close(sockfd);
}

void* recv_thread(void* p){
	char *ptr, *qtr;
	while(1){
		char buf[100] = {};
		if (recv(sockfd, buf, sizeof(buf), 0) <= 0){
			break;
		}
		if (strcmp(buf, "login") == 0){
			send(sockfd, name, strlen(name), 0);
		}
		else if (strcmp(buf, "fail") == 0){
			printf("wrong login!\n");
			exit(1);
		}
		else if(strncmp(buf, "CONNECT ", 8) == 0){
			ptr=strstr(buf, " ");
			ptr++;
			strcpy(opponame, ptr);
			
			qtr = strstr(ptr, " ");
			*qtr = '\0';
			
			qtr++;
			oppofd = atoi(qtr);
			printf("Do you want to start a new game with [%s] ?(enter yes or just ignore it.)\n",opponame);
		}
		else if(strncmp(buf, "AGREE ", 6) == 0){
			ptr = strstr(buf, " ");
			ptr++;
			strcpy(opponame, ptr);
			
			qtr = strstr(ptr, " ");
			*qtr = '\0';
			qtr++;
			oppofd = atoi(qtr);
			
			printf("%s agree with you.\n", opponame);
			printf("Game Start!\n");
			
			printf("creating chessboard...\n");
			for(int i = 0 ; i < 9 ; i++)
				x[i]=' ';
			printf("creating successfully!\n");
			
			le1 = 'X';
			le2 = 'O';
			Game = 1;
			
			if(le1 == 'O')
				myturn = 1;
			else
				myturn = 0;
			if(myturn)
				printf("Press Enter to continue...\n");
			else
				printf("Wait for your opponent....\n");
		}
		else if(buf[0] == '#')
		{
			x[atoi(&buf[1])] = le2;
			if(iswin(le2))
			{
				print();
				printf("[You lose!]\n\n");
				Game = 0;
			}
			else if(isfair())
			{
				printf("[Fair!]\n\n");
			}
			else{
				myturn = 1;
				print();
				printf("Please enter #(0-8)\n");
			}
		}
		else{
			printf("%s\n", buf);
		}
	}
}

