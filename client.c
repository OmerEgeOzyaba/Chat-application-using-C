#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>

int main(int argc, char *argv[])
{
    struct sockaddr_in address;
    int sock_fd;
    char buf[1024];

    sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd < 0) {
        perror("socket");
        return 1;
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    address.sin_port = htons(6966);

    if (-1 == connect(sock_fd, (struct sockaddr *)&address, sizeof(address))) {
        perror("connect");
        return 1;
    }

    if (-1 == fcntl(0, F_SETFL, O_NONBLOCK)){
    	perror("fcntl");
	return 1;
    }



    if (-1 == fcntl(sock_fd, F_SETFL, O_NONBLOCK)){
    	perror("fcntl");
	return 1;
    }




    FILE *server = fdopen(sock_fd, "r+");

    while (1) {
	if (NULL != fgets(buf, sizeof(buf), stdin)) {
		if (fprintf(server, "%s", buf) < 0 || fflush(server) != 0) {
			perror("send to server");
		}
		else if (errno != EAGAIN && errno != EWOULDBLOCK){
			perror("fgets stdin");
			break;
		}
	}

	if (NULL != fgets(buf, sizeof(buf), server)) {
		if (fprintf(stdout, "%s", buf) < 0 || fflush(stdout) != 0) {
			perror("write to stdout");
		}
		else if (errno != EAGAIN && errno != EWOULDBLOCK) {
			perror ("fgets server");
			break;
		}
	}



        usleep(100 * 1000);
    }

    return 0;
}
