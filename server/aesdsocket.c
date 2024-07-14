#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <syslog.h>

#define PORT 9000
#define BUFFER_SIZE 8192
#define FILENAME "/var/tmp/aesdsocketdata"

volatile int  signal_handle = 0;

void handle_client(int client_fd, struct sockaddr_in address) {
    int buffer_size = BUFFER_SIZE;
    char *buffer = malloc(BUFFER_SIZE*sizeof(char));
    ssize_t bytes_received;
    size_t total_bytes_received = 0;
    int file_fd;

    while (!signal_handle) {
	if (setsockopt(sock, SOL_SOCKET, SO_RCVBUF, &buffer_size, sizeof(buffer_size)) == -1) {
            perror("setsockopt() error");
	    free(buffer);
            close(client_fd);
            return;
        }
        bytes_received = recv(client_fd, buffer + total_bytes_received, buffer_size - total_bytes_received, 0);
        if (bytes_received < 0) {
            perror("recv");
	    free(buffer);
            close(client_fd);
            return;
        } else if (bytes_received == 0) {
            // Connection closed by client
	    free(buffer);
            close(client_fd);
	    syslog(LOG_INFO,"Closed connetion %s",inet_ntoa(address.sin_addr));
	    printf("Closed connetion %s\n",inet_ntoa(address.sin_addr));
            return;
        }

        total_bytes_received += bytes_received;

	if(total_bytes_received == buffer_size){
	    buffer_size *= 2;
	    buffer = realloc(buffer, buffer_size);
            if (buffer == NULL) {
                perror("realloc");
                close(client_fd);
                return;
            }
	}

        // Check if we have received a newline character
        if (memchr(buffer, '\n', total_bytes_received) != NULL) {
	    file_fd = open(FILENAME, O_RDWR | O_APPEND | O_CREAT, 0644);
            if (file_fd < 0) {
                perror("open file");
                free(buffer);
                close(client_fd);
                return;
            }
            if (write(file_fd, buffer, total_bytes_received) < 0) {
                perror("write to file");
		free(buffer);
                close(client_fd);
		close(file_fd);
                return;
            }

            if (file_fd < 0) {
                perror("open file");
		free(buffer);
                close(client_fd);
		close(file_fd);
                return;
            }

	    lseek(file_fd,0,SEEK_SET);
            ssize_t bytes_read;
            while ((bytes_read = read(file_fd, buffer,BUFFER_SIZE )) > 0) {
                if (send(client_fd, buffer, bytes_read, 0) < 0) {
                    perror("send");
		    free(buffer);
                    close(client_fd);
		    close(file_fd);
                    return;
                }
            }

            close(file_fd);
            total_bytes_received = 0; // Reset buffer for the next message
        }
    }
    free(buffer);
    close(client_fd);
    return;
}


int server_fd, client_fd;
static void pSaHandle(int signo){
    switch(signo){
    	case SIGINT:
	case SIGTERM:
		signal_handle = 1;
		close(client_fd);
		close(server_fd);
		break;
    }
}

int daemonize(){
	switch(fork()){
		case -1: return -1;
		case 0: break;
		default: _exit(EXIT_SUCCESS);
	}

	if(setsid() == -1)
		return -1;

	switch(fork()){
		case -1: return -1;
		case 0: break;
		default: _exit(EXIT_SUCCESS);
	}
	return 0;
}
int main(int argc, char **argv) {
    struct sockaddr_in address;
    socklen_t addrlen = sizeof(address);
    struct sigaction sa;

    memset(&sa,0,sizeof sa);
    sa.sa_handler = pSaHandle;
    sa.sa_flags = SA_RESTART;
    sigaction(SIGINT,&sa,NULL);
    sigaction(SIGTERM,&sa,NULL);

    openlog("Assignment5_1:socket-server",LOG_NDELAY,LOG_USER);

    // Create server socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
	goto close_log;
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // Bind the socket to the address
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    if(argc == 2 && strcmp(argv[1],"-d") == 0){
    	if(daemonize() < 0)
		goto close_sys;
    }
    // Listen for incoming connections
    if (listen(server_fd, SOMAXCONN) < 0) {
        perror("listen failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    while (!signal_handle) {
        // Accept a new connection
        if ((client_fd = accept(server_fd, (struct sockaddr *)&address, &addrlen)) < 0) {
            perror("accept");
	    goto close_sys;
        }

        syslog(LOG_INFO,"New connection from %s\n", inet_ntoa(address.sin_addr));
        printf("New connection from %s\n", inet_ntoa(address.sin_addr));

        // Handle the client in a blocking manner
        handle_client(client_fd, address);

    }

close_sys:
    if(server_fd)
    	close(server_fd);
    if(client_fd)
	close(client_fd);
close_log:
    closelog();
    printf("removing file %s\n", FILENAME);
    remove(FILENAME);
    return 0;
}

