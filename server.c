#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>
#include <signal.h>
#include <string.h>

int sockfd, size = 100, on = 1;
int fds[100];
char account[105][11];

typedef struct sockaddr SA;

int check(char*);
void broadcast(char*);
void getAlluser(int);
void* service_thread(void*);

int main(){
	struct sockaddr_in addr;
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd == -1){
		perror("Create socket Failed");
		exit(-1);
	}
	
	setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(int));
	
	addr.sin_family = AF_INET;
	addr.sin_port = htons(8080);
	addr.sin_addr.s_addr=htonl(INADDR_ANY);
	
	if (bind(sockfd, (SA*)&addr, sizeof(addr)) == -1){
		perror("Bind Failed");
		exit(-1);
	}
	
	if (listen(sockfd, 100) == -1){
		perror("Listen Failed");
		exit(-1);
	}
		
	printf("Server starts.\n");
	while(1){
		struct sockaddr_in fromaddr;
		socklen_t len = sizeof(fromaddr);
		int fd = accept(sockfd, (SA*)&fromaddr, &len);
		if (fd == -1){
			printf("Client occurr Error ...\n");
			continue;
		}
		int i = 0;
		for (i = 0 ; i < size ; i++){
			if (fds[i] == 0){
				fds[i] = fd;
				pthread_t tid;
				pthread_create(&tid, 0, service_thread, &fd);
				break;
			}
			else if (i == size - 1 && fds[size - 1] != 0){
				char* str = "the room is full!Try again later.";
				send(fd, str, strlen(str), 0);
				close(fd);
			}
		}
	}
	
	return 0;
}

int check(char* buf)
{
	char tmp1[100];
	FILE* fp = fopen("password.txt", "r");
	if(fp == NULL){
		printf("password.txt not found!\n");
		exit(1);
	}
	
	//while(fscanf(fp, "%s", tmp1) != EOF)
	while(fgets(tmp1, 100, fp) != NULL)
	{
		char* p = strstr(tmp1, "\n");
		*p = '\0';
		if(strcmp(tmp1, buf) == 0){
			fclose(fp);
			return 1;
		}
			
	}
	fclose(fp);
	return 0;
}

void broadcast(char* msg){
	int i;
	for (i = 0 ; i < size;i++){
		if (fds[i] != 0){
			printf("System sending to %d\n", fds[i]);
			send(fds[i], msg, strlen(msg), 0);
		}
	}
}

void getAlluser(int fd){
	send(fd,"---online---\n",strlen("---online---\n"),0);
	for (int i = 0 ; i < size ; i++){
		if (fds[i] != 0){
			char buf[100] = {};
			if(fds[i] != fd){
				sprintf(buf, "user: [%s] fd:%d\n" , account[fds[i]] , fds[i]);
				send(fd, buf, strlen(buf), 0);
			}
		}
	}
	send(fd,"------------\n",strlen("------------\n"),0);
}


void* service_thread(void* p){
	int fd = *(int*)p, ok;
	char *ptr, tmp[100];
	printf("pthread = %d\n", fd);

	char buf[100] = {};
	while(1){
		send(fd , "login", strlen("login"), 0);
		recv(fd, buf, sizeof(buf), 0);
		
		if(strcmp(buf, "new") == 0){
			break;
		}
		
		
		printf("buf=%s\n", buf);
		ptr = strstr(buf, ":");
		*ptr = '\0';
		strcpy(account[fd], buf);
		account[fd][strlen(account[fd])] = '\0';
		*ptr = ':';
		ptr = strstr(buf, "@");
		*ptr = '\0';
		ok = check(buf);
		if(ok == 1){
			ptr = strstr(buf, ":");
			*ptr = '\0';
			printf("New account : %s\n",  account[fd]);
			broadcast(account[fd]);
			break;
		}
		else{
		    printf("fail\n");
		    send(fd, "fail", strlen("fail"), 0);
		    break;
		}
	}
	while(1){
		char buf2[100] = {};

		if (recv(fd, buf2, sizeof(buf2), 0) <= 0){
			int i;
			for (i = 0 ; i < size ; i++){
				if (fd == fds[i]){
					fds[i] = 0;
					break;
				}
			}
			printf("fd = %d quit\n", fd);
			pthread_exit((void*)&i);
		}

		if (strcmp(buf2, "ls") == 0){
			getAlluser(fd);
		}
		else if(strncmp(buf2, "/all ", 4) == 0){
			broadcast(buf2);
		}
		else if(strncmp(buf2, "/msg ", 4) == 0){
			/*  /msg 4 [apple] : hi    */
			char *msg = (char*)malloc( 256*sizeof(char) );
			char* p = buf2;
			p = strstr(buf2, "msg");
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
			
			sprintf(msg,"/msg %s", p);
			printf("msg : %s\n", msg);
			send(msgfd, msg, strlen(msg), 0);
			free(msg);
		}
		else if(buf2[0] == '@')
		{
			int oppofd = atoi(&buf2[1]);
			char *msg = (char*)malloc( 256*sizeof(char) );
			strcpy(msg, "CONNECT ");
			strcat(msg, account[fd]);
			sprintf(tmp, " %d", fd);
			strcat(msg, tmp);
			send(oppofd, msg, strlen(msg), 0);
			send(fd, "wait for reply...", strlen("wait for reply..."), 0);
			free(msg);
		}
		else if(strncmp(buf2, "AGREE ", 6)==0)
		{
			int oppofd = atoi(&buf2[6]);
			char *msg = (char*)malloc( 256*sizeof(char) );
			strcpy(msg, "AGREE ");
			strcat(msg, account[fd]);
			sprintf(tmp, " %d", fd);
			strcat(msg, tmp);
			send(oppofd, msg, strlen(msg), 0);
			free(msg);
		}
		else if(buf2[0] == '#')
		{
			int n = atoi(&buf2[1]), oppofd;
			char *ptr, tmp[100];
			ptr = strstr(buf2, " ");
			ptr++;
			oppofd = atoi(ptr);
			sprintf(tmp, "#%d", n);
			printf("[%d] buf:%s\n", fd, tmp);
			send(oppofd, tmp, strlen(tmp), 0);
		}
		else{
			broadcast(buf2);
		}
		memset(buf2, 0, sizeof(buf2));
	}
}

