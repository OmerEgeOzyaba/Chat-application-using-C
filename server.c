#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#define MAX_CLIENTS 4
FILE *CLIENTS[MAX_CLIENTS] = {0};

void redistribute_message(int sender_index, char *buf) {
	for (int i = 0; i < MAX_CLIENTS; i++){
		if (sender_index == i || NULL ==  CLIENTS[i]) continue;
		if (fprintf(CLIENTS[i], "%s", buf)<0 || 0 != fflush(CLIENTS[i])){
			fclose(CLIENTS[i]);
                        CLIENTS[i] = NULL;
		}
	}
}

int poll_message(char *buf, size_t len, int client_index){
	if (NULL == fgets(buf, len, CLIENTS[client_index])){
		if (errno == EAGAIN || errno == EWOULDBLOCK){
        		return 0;
        	}
		fclose(CLIENTS[client_index]);
		CLIENTS[client_index] = NULL;

		return 0;
	}
	return 1;
}

void try_add_client(int server_fd){
	int client_fd;
	if (-1 == (client_fd = accept (server_fd, NULL, NULL))){
		if (errno != EWOULDBLOCK && errno != EAGAIN){
			perror("accept");
			exit (1);
		}
		return;
	}

	if (-1 == fcntl(client_fd, F_SETFL, O_NONBLOCK)) {
		perror("fcntl");
		exit(1);
	}
	
	FILE* client;
	if (NULL == (client = fdopen(client_fd, "r+"))){
		perror("fdopen");
		exit(1);
	}
	int success = 0;
	for (int i=0; i<MAX_CLIENTS; i++){
		if (NULL == CLIENTS[i]){
			CLIENTS[i] = client;
			success = 1;
			break;
		}
	}
	if (0 == success) {
		fprintf(client, "There is no room!");
		fclose(client);
	}
}

int main_loop(int server_fd)
{
    char buf[1024];

    while (1) {
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (NULL == CLIENTS[i]) continue;
            if (!poll_message(buf, sizeof(buf), i)) continue;
            printf("Received message from client %i: %s", i, buf);
            redistribute_message(i, buf);
        }

        try_add_client(server_fd);

        usleep(100 * 1000); // wait 100ms before checking again
    }
}

int main(int argc, char* argv[])
{
    struct sockaddr_in address;
    int server_fd;

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket");
        return 1;
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    address.sin_port = htons(6966);

    if (-1 == bind(server_fd, (struct sockaddr *)&address, sizeof(address))) {
        perror("bind");
        close(server_fd);
        return 1;
    }

    if (-1 == listen(server_fd, 5)) {
        perror("listen");
        close(server_fd);
        return 1;
    }

    if (-1 == fcntl(server_fd, F_SETFL, O_NONBLOCK)) {
        perror("fcntl server_fd NONBLOCK");
        close(server_fd);
        return 1;
    }

    int status = main_loop(server_fd);
    close(server_fd);
    return status;
}
