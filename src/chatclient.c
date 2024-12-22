#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include "util.h"

int client_socket = -1;
char username[MAX_NAME_LEN + 1];
char inbuf[BUFLEN + 1];
char outbuf[MAX_MSG_LEN + 1];

int handle_stdin() {	
    char temp_outbuf[16384];
    if (fgets(temp_outbuf, 16383, stdin) == NULL) {
	if (isatty(STDIN_FILENO) != 1 && feof(stdin)) {
	    return 1;		
 	}
	fprintf(stderr, "Error: Failed to read message. %s.\n", strerror(errno));
	return -1;
    }
    if (strlen(temp_outbuf) > MAX_MSG_LEN + 1) {
	printf("Sorry, limit your message to 1 line of at most %d characters.\n", 
	    MAX_MSG_LEN);
	    
    } else {
	strcpy(outbuf, temp_outbuf);
	if (outbuf[0] == '\n') return 0;
	outbuf[strlen(outbuf) - 1] = '\0';
	if (strcmp(outbuf, "bye") == 0) {
		printf("Goodbye.\n");
		return 1;
	}
	int i = 0;
	if ((i = send(client_socket, outbuf, strlen(outbuf) + 1, 0)) < 0) {
	    fprintf(stderr, "Error: Failed to send message to server. %s.\n", 
		strerror(errno));
	    return -1;
	}
    } 
    return 0;
} 
 

int handle_client_socket() {
    int i = 0;
    int bytes_recvd = recv(client_socket, inbuf, 1, 0);
    if (bytes_recvd < 0) {
    	fprintf(stderr, "Error: Failed to receive message from server. %s.\n", strerror(errno));
	return -1;
    } 
    while (inbuf[i] != '\0') {
	i++;
    	bytes_recvd = recv(client_socket, inbuf + i, 1, 0);
        if (bytes_recvd < 0) {
   	    fprintf(stderr, "Error: Failed to receive message from server. %s.\n", strerror(errno));
	    return -1;
	}
    	if (strcmp(inbuf, "bye") == 0) {
	    printf("\nServer initiated shutdown.\n");
	    return 1;
        }

    	if (bytes_recvd == 0) {
	    fprintf(stderr, "\nConnection to server has been lost.\n");
	    return -1;
  	}
    }

    return 0;
}

int main(int argc, char **argv) {
    struct sockaddr_in serveraddr;
    int retval = EXIT_SUCCESS;
 
    if (argc != 3) {
    	fprintf(stderr, "Usage: %s <server IP> <port>\n", argv[0]);
	return EXIT_FAILURE;
    }

    int ip_conversion = inet_pton(AF_INET, argv[1], &serveraddr.sin_addr) ;
    if (ip_conversion == 0) {
	 fprintf(stderr, "Error: Invalid IP address '%s'.\n", argv[1]);
	 retval = EXIT_FAILURE;
	 goto EXIT;
    }

    else if (ip_conversion < 0) {
    	fprintf(stderr, "Error: Failed to convert IP address. %s.\n", strerror(errno));
	retval = EXIT_FAILURE;
	goto EXIT;
    }

    int port = atoi(argv[2]);
    if (port < 1024 || port > 65535) {
    	fprintf(stderr, "Error: Port number must be an integer in the range [1024, 65535].\n");
	return EXIT_FAILURE;
    }

    while (1) {
   	if (isatty(STDIN_FILENO) == 1) printf("Enter your username: ");
    	fflush(stdout);
	char temp_username[1024];
	memset(temp_username, 0, 1024);
    	if (fgets(temp_username, 1023, stdin) == NULL) {
		fprintf(stderr, "Error: Failed to read username. %s.", strerror(errno));
		retval = EXIT_FAILURE;
		goto EXIT;
	} else {
		if (strlen(temp_username) > MAX_NAME_LEN + 1) {
    		    printf("Sorry, limit your username to %d characters.\n", MAX_NAME_LEN);
		} 
		else if (strcmp(temp_username, "\n") == 0) continue;
		else {
		    strcpy(username, temp_username);
		    break;
		}
	}
    }
    username[strlen(username) - 1] = '\0';
    printf("Hello, %s. Let's try to connect to the server.\n", username);

    if ((client_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    	fprintf(stderr, "Error: Failed to create socket. %s.\n", strerror(errno));
	retval = EXIT_FAILURE;
	goto EXIT;
    }

    serveraddr.sin_family = AF_INET;
    serveraddr.sin_port = htons(port);

    if (connect(client_socket, (struct sockaddr *)&serveraddr, 
		    sizeof(struct sockaddr_in)) < 0) {
        fprintf(stderr, "Error: Failed to connect to server. %s.\n",
                strerror(errno));
        retval = EXIT_FAILURE;
        goto EXIT;
    }
 
    int bytes_recvd = recv(client_socket, inbuf, BUFLEN, 0);
    if (bytes_recvd < 0) {
    	fprintf(stderr, "Error: Failed to receive message from server. %s.\n", strerror(errno));
	retval = EXIT_FAILURE;
	goto EXIT;
    }

    inbuf[bytes_recvd] = '\0';
    printf("%s\n", inbuf);
    printf("\n");

    if (send(client_socket, username, strlen(username) + 1, 0) < 0) {
    	fprintf(stderr, "Error: Failed to send username to server. %s.\n", strerror(errno));
	retval = EXIT_FAILURE;
	goto EXIT;
    }

    fd_set readsocks;
    fd_set writesocks;
    int i = 0;
    while (1) {
    	FD_ZERO(&readsocks);
    	FD_ZERO(&writesocks);
	FD_SET(STDIN_FILENO, &writesocks);
	FD_SET(client_socket, &readsocks);

	FD_SET(STDIN_FILENO, &readsocks);
	FD_SET(client_socket, &writesocks);

    	if (select(client_socket + 1, &readsocks, &writesocks, NULL, NULL) < 0) {
    	    fprintf(stderr, "Error: select() failed. %s.\n", strerror(errno));
	    retval = EXIT_FAILURE;
	    goto EXIT;
        }
	if (i == 0 && isatty(STDIN_FILENO) == 1) {
	    printf("[%s]: ", username);
	    fflush(stdout);
	}
	
	if (FD_ISSET(STDIN_FILENO, &readsocks)) {	
	    i = 2;
	    int ret = handle_stdin();
	    if (ret != 0) {
	        if (ret < 0) retval = EXIT_FAILURE;
    	        goto EXIT;	    
	    }
	}

	if (FD_ISSET(client_socket, &readsocks)) {
   	    int ret = handle_client_socket();
	    if (i != 2 && ret == 0) printf("\n");
	    if (ret == 0) printf("%s\n", inbuf);
	    i = 0;
   	    if (ret != 0) {
		if (ret < 0) retval = EXIT_FAILURE;
    	    	goto EXIT;	    
	    }
		continue;
    	}
	else {
	    if (i == 2) {
		i = 0;
		continue;
	    }
	}
	i = 1;
    }

 
EXIT:
 if (fcntl(client_socket, F_GETFD) != -1) {
        close(client_socket);
    }
    return retval;    
}
