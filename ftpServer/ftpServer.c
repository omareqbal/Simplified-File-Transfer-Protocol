
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h> 
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <fcntl.h>
#include <sys/wait.h>

#define SERVER_PACKET 10


#define X 50000
#define Y 55000

int main(int argc, char *argv[]){
	int sockfd, newsockfd, datasockfd,status;
	int clilen;
	pid_t pid;
	struct sockaddr_in serv_addr, cli_addr, data_serv_addr;

	char buf[100], buf2[100], buf3[100];
	char line[80];
	char cmd[5][100];
	int n,i,j,k, len,args, first_cmd, enable, port_no=Y, infile, outfile, resp_code;
	short blocklen;
	char header1[5];
    char header2[5];

	if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
		perror("Cannot create socket");
		exit(1);
	}

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr	= INADDR_ANY;
	serv_addr.sin_port		= htons(X);

	data_serv_addr.sin_family = AF_INET;
    inet_aton("127.0.0.1", &data_serv_addr.sin_addr);
    

	enable = 1;
	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0){
	    perror("setsockopt(SO_REUSEADDR) failed");
	    exit(1);
	}

	if (bind(sockfd, (struct sockaddr *) &serv_addr,	sizeof(serv_addr)) < 0) {
		perror("Unable to bind local address");
		exit(1);
	}

	listen(sockfd, 5);

	printf("Server Running\n");

	while(1){

		clilen = sizeof(cli_addr);

		newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);


		if (newsockfd < 0) {
			perror("Accept error\n");
			exit(1);
		}

		first_cmd = 1;
		while(1){
			memset(buf, 0, sizeof(buf));
			memset(line, 0, sizeof(line));
			memset(cmd, 0, sizeof(cmd));
			n = recv(newsockfd, buf, SERVER_PACKET, 0);
			j=0;
			while(1){
				for(i=0;i<n;i++){
					line[j++] = buf[i];
					if(buf[i] == '\n' || buf[i] == '\0')
						break;
				}
				if(i<n)
					break;
				n = recv(newsockfd, buf, SERVER_PACKET, 0);
			}
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
			if(strcmp(cmd[0],"port")!=0 && first_cmd == 1){
				resp_code = htonl(503);
				send(newsockfd, &resp_code, sizeof(resp_code), 0);
				close(newsockfd);
				break;
			}
			else if(strcmp(cmd[0],"port")==0){
				if(args != 2){
					resp_code = htonl(501);
					send(newsockfd, &resp_code, sizeof(resp_code), 0);
					continue;
				}
				first_cmd = 0;
				port_no = atoi(cmd[1]);
				if(port_no > 1024 && port_no < 65536){
					data_serv_addr.sin_port = htons(port_no);
					resp_code = htonl(200);
					send(newsockfd, &resp_code, sizeof(resp_code), 0);
				}
				else{
					resp_code = htonl(550);					
					send(newsockfd, &resp_code, sizeof(resp_code), 0);
					close(newsockfd);
					break;
				}
				
			}
			else if(strcmp(cmd[0],"cd")==0){
				if(args != 2){
					resp_code = htonl(501);
					send(newsockfd, &resp_code, sizeof(resp_code), 0);
					continue;
				}
				if(chdir(cmd[1])==0){
					resp_code = htonl(200);
					send(newsockfd, &resp_code, sizeof(resp_code), 0);
				}
				else{
					resp_code = htonl(501);					
					send(newsockfd, &resp_code, sizeof(resp_code), 0);
				}
				
			}
			else if(strcmp(cmd[0],"get")==0){
				if(args != 2){
					resp_code = htonl(501);
					send(newsockfd, &resp_code, sizeof(resp_code), 0);
					continue;
				}
				infile = open(cmd[1], O_RDONLY);
				if(infile == -1){	
					resp_code = htonl(550);			
					send(newsockfd, &resp_code, sizeof(resp_code), 0);
				}
				else{
					pid = fork();
					if(pid == 0){
						sleep(1);
						if ((datasockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
					        perror("Unable to create socket\n");
					        exit(1);
					    }


					    if ((connect(datasockfd, (struct sockaddr *) &data_serv_addr, sizeof(data_serv_addr))) < 0 || (port_no==ntohs(serv_addr.sin_port))) {
					        perror("Unable to connect to server\n");
					        exit(1);
					    }


    					memset(buf, 0, sizeof(buf));
    					memset(buf2, 0, sizeof(buf2));
    					memset(buf3, 0, sizeof(buf3));
    					n = read(infile, buf, SERVER_PACKET);
    					while(n!=0){
    						blocklen = (short)n;
    						for(i=0;i<n;i++)
    							buf2[i]=buf[i];
    						n = read(infile, buf, SERVER_PACKET);
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
    						send(datasockfd, buf3, blocklen+3, 0);

    					}
    		
						close(datasockfd);
						close(infile);
						exit(0);
					}
					else{
						wait(&status);
						close(infile);
						if(status == 0)
							resp_code = htonl(250);
						else
							resp_code = htonl(550);			
						send(newsockfd, &resp_code, sizeof(resp_code), 0);
					}
					
					
				}
			}
			else if(strcmp(cmd[0],"put")==0){
				if(args != 2){
					resp_code = htonl(501);
					send(newsockfd, &resp_code, sizeof(resp_code), 0);
					continue;
				}

				for(i=0;i<strlen(cmd[1]);i++){
					if(cmd[1][i]=='/'){
						resp_code = htonl(501);			
						send(newsockfd, &resp_code, sizeof(resp_code), 0); 
						break;
					}
				}

				if(i<strlen(cmd[1]))
					continue;

				pid = fork();
				if(pid == 0){
					sleep(1);
					if ((datasockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
				        perror("Unable to create socket\n");
				        exit(1);
				    }


				    if ((connect(datasockfd, (struct sockaddr *) &data_serv_addr, sizeof(data_serv_addr))) < 0 || (port_no==ntohs(serv_addr.sin_port))) {
				        perror("Unable to connect to server\n");
				        exit(1);
				    }
					
					outfile = open(cmd[1], O_WRONLY | O_CREAT | O_TRUNC , 0666);
					if(outfile == -1){	
						exit(1);
					}
					do{
						k=recv(datasockfd, header1, 1, 0);
						k=recv(datasockfd, header2, 2, 0);
						blocklen = (header2[0]<<8) | header2[1];
						memset(buf, 0, sizeof(buf));
						n=0;
						while(n<blocklen){
							i = recv(datasockfd, buf, SERVER_PACKET, 0);
							n += i;
							for(j=0;j<i;j++){
								write(outfile, &buf[j], 1);
							}
						}
					}while(header1[0] != 'L');

					close(datasockfd);
					close(outfile);
					exit(0);
				}
				else{
					wait(&status);
					if(status == 0)
						resp_code = htonl(250);
					else
						resp_code = htonl(550);			
					send(newsockfd, &resp_code, sizeof(resp_code), 0);
				}
					
			}
			else if(strcmp(cmd[0],"quit")==0){
				if(args != 1){
					resp_code = htonl(501);
					send(newsockfd, &resp_code, sizeof(resp_code), 0);
					continue;
				}
				resp_code = htonl(421);	
				send(newsockfd, &resp_code, sizeof(resp_code), 0);
				close(newsockfd);
				break;
			}
			else{
				resp_code = htonl(502);	
				send(newsockfd, &resp_code, sizeof(resp_code), 0);
			}
		}
	}
	close(sockfd);

}