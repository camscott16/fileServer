#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <dirent.h>
#include <sys/stat.h>
#include "Practical.h"

#define MAXPENDING 5  // Maximum number of pending connections

void HandleClient(int clntSock);

int main(int argc, char *argv[]) {

	if (argc != 2) {  // Test for correct number of arguments
		DieWithUserMessage("Parameter(s)", "<Server Port>");
	}

	in_port_t servPort = atoi(argv[1]);  // Local port

	// Create socket for incoming connections
	int servSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (servSock < 0) {
		DieWithSystemMessage("socket() failed");
	}

	// Construct local address structure
	struct sockaddr_in servAddr;
	memset(&servAddr, 0, sizeof(servAddr));  // Zero out structure
	servAddr.sin_family = AF_INET;           // IPv4 address family
	servAddr.sin_addr.s_addr = htonl(INADDR_ANY);  // Any incoming interface
	servAddr.sin_port = htons(servPort);     // Local port

	// Bind to the local address
	if (bind(servSock, (struct sockaddr *)&servAddr, sizeof(servAddr)) < 0) {
		DieWithSystemMessage("bind() failed");
	}

	// Listen for incoming connections
	if (listen(servSock, MAXPENDING) < 0) {
		DieWithSystemMessage("listen() failed");
	}

	printf("Welcome to Simple File Server\n");

	for (;;) {  // Run forever
		struct sockaddr_in clntAddr;
		socklen_t clntAddrLen = sizeof(clntAddr);

		// Wait for a client to connect
		int clntSock = accept(servSock, (struct sockaddr *)&clntAddr, &clntAddrLen);
		if (clntSock < 0) {
			DieWithSystemMessage("accept() failed");
		}

		// clntSock is connected to a client
		char clntName[INET_ADDRSTRLEN];
		if (inet_ntop(AF_INET, &clntAddr.sin_addr.s_addr, clntName, sizeof(clntName)) != NULL) {
			printf("Connected to IP: %s\n", clntName);
		} else {
			puts("Unable to get client address");
		}

		HandleClient(clntSock);
		printf("Connection with %s is terminated\n", clntName);
	}
}

void HandleClient(int clntSock) {

	int command;
	char buffer[BUFSIZE];
	ssize_t numBytesRcvd;
	int keepRunning = 1;

	while (keepRunning) {
		numBytesRcvd = recv(clntSock, &command, sizeof(command), 0);
		if (numBytesRcvd < 0) {
			DieWithSystemMessage("recv() failed");
		} else if (numBytesRcvd == 0) {
			// Client disconnected
			break;
		}

		switch (command) {
			case 1:  // LIST

				printf("-- client requested file listing\n");
				// Send file listing
				DIR *dir = opendir(".");
				if (dir == NULL) {
					DieWithSystemMessage("opendir() failed");
				}

				struct dirent *entry;
				while ((entry = readdir(dir)) != NULL) {
					if (entry->d_type == DT_REG) {  // Only regular files
						struct stat fileStat;
						if (stat(entry->d_name, &fileStat) < 0) {
							continue;
						}
						snprintf(buffer, BUFSIZE, "%s %ld bytes %ld\n", entry->d_name, (long)fileStat.st_size, (long)fileStat.st_ino);
						send(clntSock, buffer, strlen(buffer), 0);
					}
				}
				closedir(dir);
				const char *endMarker = "END_OF_LIST";
				send(clntSock, endMarker, strlen(endMarker) + 1, 0);

				break;

			case 2:  // DOWNLOAD

				numBytesRcvd = recv(clntSock, buffer, BUFSIZE - 1, 0);
				if (numBytesRcvd <= 0) {
					break;
				}
				buffer[numBytesRcvd] = '\0';
				printf("-- client requested to download %s\n", buffer);

				FILE *file = fopen(buffer, "rb");
				if (file == NULL) {
					snprintf(buffer, BUFSIZE, "ERROR: File not found\n");
					send(clntSock, buffer, strlen(buffer), 0);
				} else {
					while ((numBytesRcvd = fread(buffer, 1, BUFSIZE, file)) > 0) {
						send(clntSock, buffer, numBytesRcvd, 0);
					}
					fclose(file);
					printf("Sent %s to the client\n", buffer);
				}
				break;

			case 3:  // UPLOAD

				numBytesRcvd = recv(clntSock, buffer, BUFSIZE - 1, 0);
				if (numBytesRcvd <= 0) {
					break;
				}
				buffer[numBytesRcvd] = '\0';
				printf("-- client requested to upload: %s\n", buffer);

				FILE *fileUpload = fopen(buffer, "wb");
				if (fileUpload == NULL) {
					snprintf(buffer, BUFSIZE, "ERROR: Cannot create file\n");
					send(clntSock, buffer, strlen(buffer), 0);
				} else {
					while ((numBytesRcvd = recv(clntSock, buffer, BUFSIZE, 0)) > 0) {
						fwrite(buffer, 1, numBytesRcvd, fileUpload);
					}
					fclose(fileUpload);
					printf("Received %s from the client\n", buffer);
				}

				break;

			case 4:  // EXIT

				printf("-- client requested to exit\n");
				keepRunning = 0;
				break;

			default:
				snprintf(buffer, BUFSIZE, "ERROR: Unknown command\n");
				send(clntSock, buffer, strlen(buffer), 0);
				break;
		}
	}

	close(clntSock);  // Close client socket
}

