#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/sendfile.h>

char webContent[4096];

int main(int argc, char** argv){
	
	char htmlContent[2048], temp[50];
	FILE* fp_html = fopen("index.html", "r+");
	fread(htmlContent, 2048, 1, fp_html);
	//printf("length:%ld\n", strlen(htmlContent));
	//printf("html:\n%s\n", htmlContent);
	
	webContent[0] = '\0';
	strcat(webContent, "HTTP/1.1 200 OK\r\n");
	sprintf(temp, "Content-Length: %ld\r\n", strlen(htmlContent));
	strcat(webContent, temp);
	strcat(webContent, "Content-Type: text/html\r\n");
	strcat(webContent, "Connection: close\r\n\r\n");
	strcat(webContent, htmlContent);
	strcat(webContent, "\r\n");
	//printf("%s\n", webContent);

	struct sockaddr_in server_addr, client_addr;
	socklen_t sin_len = sizeof(client_addr);
	
	//fd_server(fd_client) is socket address of server(client)
	int fd_server, fd_client;
	char buf[2048];
	int fdimg;
	int on = 1;
	
	fd_server = socket(AF_INET, SOCK_STREAM, 0);
	if(fd_server < 0){
		perror("socket");
		exit(1);
	}
	
	setsockopt(fd_server, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(int));
	
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	server_addr.sin_port = htons(8080);
	
	if(bind(fd_server, (struct sockaddr*) &server_addr, sizeof(server_addr)) < 0){
		perror("bind");
		close(fd_server);
		exit(1);
	}
	
	if(listen(fd_server, 10) < 0){
		perror("listen");
		close(fd_server);
		exit(1);
	}
	
	printf("Server Starts.\n");
	while(1){
		printf("\n==========\nloop\n");
		fd_client = accept(fd_server, (struct sockaddr*) &client_addr, &sin_len);
		if(fd_client == -1){
			perror("Connection failed....\n");
			continue;
		}
		
		printf("connect to Client successfully.\n");
		
		if(!fork()){/*child*/
			close(fd_server);
			memset(buf, 0, 2048);
			int nletter = read(fd_client, buf, 2047);
			//printf("\nread %d letter.\n", nletter);
			printf("buf:\n%s\n", buf);
			
			//char eat = getchar();
			
			if(strncmp(buf, "GET /cat.jpg", 12) == 0){
				fdimg = open("cat.jpg", O_RDONLY);
				sendfile(fd_client, fdimg, NULL, 45000);
				close(fdimg);
			}
			else if(strncmp(buf, "GET /dog.jpg", 12) == 0){
				fdimg = open("dog.jpg", O_RDONLY);
				sendfile(fd_client, fdimg, NULL, 45000);
				close(fdimg);
			}
			else if(strncmp(buf, "POST", 4) == 0){
				char* ptr;
				char* ptr2;
				char filename[512], fileContent[8192];
				char boundary[100];
				filename[0] = '\0';
				
				/*get the boundary*/
				ptr = strstr(buf, "boundary=-----");
				ptr += 9;
				ptr2 = strstr(ptr, "Content-Length");
				while(*ptr == '-')
					ptr++;
				for(char* p = ptr ; p < ptr2 ; p++){
					boundary[p - ptr] = *p;
					//printf("%c\n", *p);
				}
				boundary[ptr2 - ptr - 2] = '\0';
				//printf("boundary:%s\n", boundary);
				
				
				/*get the filename*/
				ptr = strstr(buf, "filename=");
				ptr += strlen("filename") + 2;
				ptr2 = strstr(ptr, "Content-Type");
				for(char* p = ptr ; p < ptr2 ; p++)
					filename[p - ptr] = *p;
				filename[ptr2 - ptr - 3] = '\0';
				//printf("file name : %s\n", filename);
				
				
				/*get the file content*/
				ptr = strstr(ptr, "\r\n");
				ptr++;
				ptr = strstr(ptr, "\r\n");
				ptr++;
				ptr = strstr(ptr, "\r\n");
				ptr += 2;
				
				ptr2 = strstr(ptr, boundary);
				//printf("%p %p\n", ptr, ptr2);
				for(char* p = ptr ; p < ptr2 ; p++)
					fileContent[p - ptr] = *p;
				fileContent[ptr2 - ptr - 3] = '\0';
				//printf("fileContent:\n%s", fileContent);
				/*
				now the file content should be
				file content
				
				----------------
				, so I have to delete \r\n and --- 
				*/
				
				ptr = fileContent + strlen(fileContent) - 1;
				while(*ptr == '-')
					ptr--;
				ptr -= 2;
				*ptr = '\0';
				//printf("\nfileContent:\n%s\n", fileContent);
				
				char path[512];
				strcpy(path, "Upload/");
				strcat(path, filename);
				FILE* fp_target = fopen(path, "w");
				if(fwrite(fileContent, 1, strlen(fileContent), fp_target) == strlen(fileContent))
					printf("\nUpload Successfully!\n");
				
				
				write(fd_client, webContent, sizeof(webContent) - 1);
			}
			else
				write(fd_client, webContent, sizeof(webContent) - 1);
			
			close(fd_client);
			printf("closing...\n");
			exit(0);
		}
		/*parent*/
		printf("\nparent fd_client closed.\n");
		close(fd_client);
		
		
	}
	
	
	
	
	

	return 0;
}
