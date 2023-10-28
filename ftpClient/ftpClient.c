
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>
#include <sys/wait.h>

#define CLIENT_PACKET 10

#define X 50000
#define Y 55000

int main(int argc, char *argv[])
{
    int sockfd, datasockfd, data_clilen, newsockfd, infile, outfile ;
    pid_t pid=getpid();
    struct sockaddr_in  serv_addr, data_serv_addr, data_cli_addr;

    int i, j, k, n, enable;
    char buf[100], buf2[100], buf3[100];
    char filename[100];


    char line[80];
    char cmd[5][100];
    char header1[5];
    char header2[5];
    short blocklen;
    int resp, resp_code, len, args, port_no=Y;

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Unable to create socket\n");
        exit(1);
    }



    serv_addr.sin_family = AF_INET;
    inet_aton("127.0.0.1", &serv_addr.sin_addr);
    serv_addr.sin_port = htons(X);

    data_serv_addr.sin_family = AF_INET;
	data_serv_addr.sin_addr.s_addr	= INADDR_ANY;

    if ((connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr))) < 0) {
        perror("Unable to connect to server\n");
        exit(1);
    }
    

    while(1){
		memset(line, 0, sizeof(line));
		memset(cmd, 0, sizeof(cmd));
    	printf("> ");
	    fgets(line, 100, stdin);
	    len = strlen(line);
		args = 0;
		for(i=0;i<len;i++){
			memset(buf, 0, sizeof(buf));
			j=0;
			while(line[i]!=' ' && line[i]!='\n'){
				buf[j++] = line[i++];
			}
			buf[j] = '\0';
			strcpy(cmd[args], buf);
			args++;

		}


		if(strcmp(cmd[0],"get")==0){

			pid = fork();
			if(pid == 0){
				data_serv_addr.sin_port = htons(port_no);
				if ((datasockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
			        perror("Unable to create socket\n");
			        exit(1);
			    }
			    enable = 1;
				if (setsockopt(datasockfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0){
				    perror("setsockopt(SO_REUSEADDR) failed");
				    exit(1);
				}
			    if (bind(datasockfd, (struct sockaddr *) &data_serv_addr,	sizeof(data_serv_addr)) < 0) {
					perror("Unable to bind local address");
					exit(1);
				}

				listen(datasockfd, 5);

				data_clilen = sizeof(data_cli_addr);
				newsockfd = accept(datasockfd, (struct sockaddr *)&data_cli_addr, &data_clilen);
				if (newsockfd < 0) {
					perror("Accept error\n");
					exit(1);
				}

				j=0;
				for(i=strlen(cmd[1])-1;i>=0;i--){
					if(cmd[1][i]=='/')
						break;
				}
				for(k=i+1;k<strlen(cmd[1]);k++){
					filename[j++]=cmd[1][k];
				}
				filename[j]='\0';
				outfile = open(filename,O_WRONLY | O_CREAT | O_TRUNC, 0666);
				if(outfile==-1){
					perror("Error in opening file\n");
					exit(1);
				}
				do{
					k=recv(newsockfd, header1, 1, 0);
					k=recv(newsockfd, header2, 2, 0);
					blocklen = (header2[0]<<8) | header2[1];
					memset(buf, 0, sizeof(buf));
					n=0;
					while(n<blocklen){
						i = recv(newsockfd, buf, CLIENT_PACKET, 0);
						n += i;
						for(j=0;j<i;j++){
							write(outfile, &buf[j], 1);
						}
					}
				}while(header1[0] != 'L');
				
				do{
					n = recv(newsockfd, buf, CLIENT_PACKET, 0);
				}while(n!=0);

				close(outfile);
				close(newsockfd);
				close(datasockfd);
				exit(0);
			}
			
		}

		if(strcmp(cmd[0],"put")==0){

			pid = fork();
			if(pid == 0){
				infile = open(cmd[1], O_RDONLY);
				if(infile==-1){
					perror("Error in opening file\n");
					exit(1);
				}
				data_serv_addr.sin_port = htons(port_no);
				if ((datasockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
			        perror("Unable to create socket\n");
			        exit(1);
			    }
			    enable = 1;
				if (setsockopt(datasockfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0){
				    perror("setsockopt(SO_REUSEADDR) failed");
				    exit(1);
				}
			    if (bind(datasockfd, (struct sockaddr *) &data_serv_addr,	sizeof(data_serv_addr)) < 0) {
					perror("Unable to bind local address");
					exit(1);
				}

				listen(datasockfd, 5);

				data_clilen = sizeof(data_cli_addr);
				newsockfd = accept(datasockfd, (struct sockaddr *)&data_cli_addr, &data_clilen);

				if (newsockfd < 0) {
					perror("Accept error\n");
					exit(1);
				}

				memset(buf, 0, sizeof(buf));
				memset(buf2, 0, sizeof(buf2));
				memset(buf3, 0, sizeof(buf3));
				n = read(infile, buf, CLIENT_PACKET);
				while(n!=0){
					blocklen = (short)n;
					for(i=0;i<n;i++)
						buf2[i]=buf[i];
					n = read(infile, buf, CLIENT_PACKET);
					memset(buf3, 0, sizeof(buf3));
					if(n==0)
						buf3[0] = 'L';
					else
						buf3[0] = 'A';
					buf3[1] = (blocklen>>8) & 0xff;
					buf3[2] = blocklen & 0xff;

					for(i=0;i<blocklen;i++){
						buf3[i+3]=buf2[i];
					}
					send(newsockfd, buf3, blocklen+3, 0);

				}

				do{
					n = recv(newsockfd, buf, CLIENT_PACKET, 0);
				}while(n!=0);

				close(infile);
				close(newsockfd);
				close(datasockfd);
				exit(0);
			}
			
		}

		if(pid > 0){
			send(sockfd, line, strlen(line), 0);

			recv(sockfd, &resp, sizeof(resp), 0);
			resp_code = ntohl(resp);


			if(strcmp(cmd[0],"get")!=0 && strcmp(cmd[0],"put")!=0){
				printf("response code - %d\n",resp_code);
			}

			if(strcmp(cmd[0],"port")==0){
				if(resp_code == 200){
					port_no = atoi(cmd[1]);
					data_serv_addr.sin_port = htons(port_no);
					printf("Port command successful.\n");
				}
				else if(resp_code == 550){
					printf("Invalid port number.\n");
					close(sockfd);
					break;
				}
			}
			if(strcmp(cmd[0],"get")==0){
				if(resp_code == 250){
					waitpid(pid,NULL,0);
				}

				printf("response code - %d\n", resp_code);
				
				if(resp_code == 550){
					printf("Error in file transfer.\n");
				}
				else if(resp_code == 250){
					printf("File transfer successful.\n");
				}
				kill(pid, SIGKILL);
			}
			
			if(strcmp(cmd[0],"put")==0){
				if(resp_code == 250){
					waitpid(pid,NULL,0);
				}
				
				printf("response code - %d\n", resp_code);
				
				if(resp_code == 550){
					printf("Error in file transfer.\n");
				}
				else if(resp_code == 250){
					printf("File transfer successful.\n");
				}
				kill(pid, SIGKILL);
			}


			if(strcmp(cmd[0],"cd")==0 && resp_code == 200){
				printf("Directory change successful.\n");
			}
			if(resp_code == 501){
				if(strcmp(cmd[0],"cd")==0){
					printf("Cannot change directory.\n");
				}
				else{
					printf("Invalid argument.\n");
				}
				
			}
			else if(resp_code == 502){
				printf("Invalid command.\n");
			}
		    else if(resp_code == 503){
				printf("First command must be PORT command.\n");
				close(sockfd);
				break;
			}
			else if(resp_code == 421){
				printf("Exiting.\n");
				close(sockfd);
				exit(0);
			}

		}
	    
    }
    
}


