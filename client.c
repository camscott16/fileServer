#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include "Practical.h"

#define BUFSIZE 512

void PrintMenu();

int main(int argc, char *argv[]) {

	if (argc != 3) {  // Test for correct number of arguments
		DieWithUserMessage("Parameter(s)", "<Server IP> <Server Port>");
	}

	char *servIP = argv[1];     // Server IP address (dotted quad)
	in_port_t servPort = atoi(argv[2]);  // Server port

	// Create a reliable, stream socket using TCP
	int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sock < 0) {
		DieWithSystemMessage("socket() failed");
	}

	// Construct the server address structure
	struct sockaddr_in servAddr;
	memset(&servAddr, 0, sizeof(servAddr));  // Zero out structure
	servAddr.sin_family = AF_INET;           // IPv4 address family
	int rtnVal = inet_pton(AF_INET, servIP, &servAddr.sin_addr.s_addr);
	if (rtnVal == 0) {
		DieWithUserMessage("inet_pton() failed", "invalid address string");
	} else if (rtnVal < 0) {
		DieWithSystemMessage("inet_pton() failed");
	}
	servAddr.sin_port = htons(servPort);  // Server port

	// Establish the connection to the server
	if (connect(sock, (struct sockaddr *)&servAddr, sizeof(servAddr)) < 0) {
		DieWithSystemMessage("connect() failed");
	}

	printf("Connected to server %s\n", servIP);

	char buffer[BUFSIZE];
	char accumulatedBuffer[BUFSIZE * 4] = {0};  // Initialize buffer to avoid uninitialized value usage
	int accumulatedLen = 0;
	int choice;
	int command;

	PrintMenu();
	while (scanf("%d", &choice) != EOF) {
		getchar();  // Consume newline
		switch (choice) {

			case 1:  // Request file listing
				command = 1;
				send(sock, &command, sizeof(command), 0);
				printf(">> Listing received from server:\n");
				printf("filename\tsize\tinode\n");
				accumulatedLen = 0;
				while ((rtnVal = recv(sock, buffer, BUFSIZE - 1, 0)) > 0) {
					buffer[rtnVal] = '\0';

					// Check if the buffer contains the end-of-list marker
					if (strstr(buffer, "END_OF_LIST") != NULL) {
						break;
					}

					// Ensure accumulated buffer doesn't overflow
					if (accumulatedLen + rtnVal >= BUFSIZE * 4) {
						fprintf(stderr, "Accumulated buffer overflow risk, aborting\n");
						break;
					}

					strncat(accumulatedBuffer + accumulatedLen, buffer, BUFSIZE * 4 - accumulatedLen - 1);
					accumulatedLen += rtnVal;

					char *lineEnd;
					while ((lineEnd = strchr(accumulatedBuffer, '\n')) != NULL) {
						*lineEnd = '\0';
						char filename[256];
						long size, inode;
						int parsed = sscanf(accumulatedBuffer, "%255s %ld bytes %ld", filename, &size, &inode);
						if (parsed == 3) {
							printf("%s\t%ld bytes\t%ld\n", filename, size, inode);
						} else {
							printf("Error parsing file information: %s\n", accumulatedBuffer);
						}

						// Move remaining data to the start of accumulatedBuffer
						memmove(accumulatedBuffer, lineEnd + 1, accumulatedLen - (lineEnd - accumulatedBuffer + 1));
						accumulatedLen -= (lineEnd - accumulatedBuffer + 1);
						accumulatedBuffer[accumulatedLen] = '\0';
					}
				}
				break;

			case 2:  // Download file
				printf(">> Enter filename you would like to download:\n>> ");
				fgets(buffer, BUFSIZE, stdin);
				buffer[strcspn(buffer, "\n")] = 0;  // Remove newline
				command = 2;
				send(sock, &command, sizeof(command), 0);
				send(sock, buffer, strlen(buffer) + 1, 0);  // Send filename with null terminator
				FILE *downloadFile = fopen(buffer, "wb");
				if (downloadFile == NULL) {
					perror("Cannot create file");
					break;
				}
				while ((rtnVal = recv(sock, buffer, BUFSIZE, 0)) > 0) {
					fwrite(buffer, 1, rtnVal, downloadFile);
				}
				fclose(downloadFile);
				printf("Downloaded %s\n", buffer);
				break;

			case 3:  // Upload file
				printf(">> Enter filename to upload:\n>> ");
				fgets(buffer, BUFSIZE, stdin);
				buffer[strcspn(buffer, "\n")] = 0;  // Remove newline
				command = 3;
				send(sock, &command, sizeof(command), 0);
				send(sock, buffer, strlen(buffer) + 1, 0);  // Send filename with null terminator

				struct stat fileStat;
				if (stat(buffer, &fileStat) < 0) {
					perror("Cannot retrieve file information");
					break;
				}
				FILE *uploadFile = fopen(buffer, "rb");
				if (uploadFile == NULL) {
					perror("Cannot open file");
					break;
				}
				while ((rtnVal = fread(buffer, 1, BUFSIZE, uploadFile)) > 0) {
					send(sock, buffer, rtnVal, 0);
				}
				fclose(uploadFile);
				printf("Uploaded: %s\n", buffer);
				break;
			case 4:  // Exit
				printf("\n>> Goodbye!!!\n");
				command = 4;
				send(sock, &command, sizeof(command), 0);
				close(sock);
				exit(0);
				break;
			default:
				printf("Invalid choice. Please try again.\n");
		}
		PrintMenu();
	}

	close(sock);
	return 0;
}

void PrintMenu() {
	printf("\nSelect an action from the menu:\n");
	printf("1. Request file listing\n");
	printf("2. Download file\n");
	printf("3. Upload file\n");
	printf("4. Exit\n");
	printf(">> ");
}

