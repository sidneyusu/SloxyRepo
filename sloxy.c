/*
 * sloxy.c - Assignment 1 - CPSC 441
 *
 *	This program is simple proxy that slows down web Requests if the file being
 *	requested is an HTML file, Otherwise it delivers requests and response messages
 *	at full speeds that is possible.
 *
 *  Created on: Jan 23, 2018
 *      Author: Sidney Shane Dizon
 */

/* Standard libraries */
#include <stdio.h>
#include <stdlib.h>

/* Libraries for socket programming */
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

/* Library for parsing strings */
#include <string.h>
#include <strings.h>

/* h_addr?! */
#include <netdb.h>

/* Clean exit! */
#include <signal.h>



int lstn_sock;

/* The function will run after catching Ctrl+c in terminal */
void catcher(int sig) {
	close(lstn_sock);
	printf("catcher with signal  %d\nlstn_sock closed.\n", sig);
	exit(0);
}

int main() {

	/* If sloxy = 1 then we will slow down the request */
	int sloxy = 0;

	/* For catching Crtl+c in terminal */
	signal(SIGINT, catcher);

	/* Address initialization */
	struct sockaddr_in proxy_server;
	int SERVERPORTNUM = 9775;
	//memset(&proxy_server, 0, sizeof(proxy_server));
	proxy_server.sin_family = AF_INET;
	proxy_server.sin_port = htons(SERVERPORTNUM);
	proxy_server.sin_addr.s_addr = htonl(INADDR_ANY);

	/* Create the listening socket */
	lstn_sock = socket(AF_INET, SOCK_STREAM, 0);
	if (lstn_sock < 0) {
		printf("Error in socket() while creating lstn_sock\n");
		exit(-1);
	} else {
		printf("The lstn_socket is created.\n\n");
	}

	/* Bind the socket to address and port */
	int bind_status;
	bind_status = bind(lstn_sock, (struct sockaddr *) &proxy_server,
			sizeof(struct sockaddr_in));
	if (bind_status < 0) {
		printf("Error in bind()\n");
		exit(-1);
	} else {
		printf("Binding successful\n\n");
	}

	/* LISTENING on the binded work */
	int lstn_status = listen(lstn_sock, 5);
	if (lstn_status < 0) {
		printf("Error in listen()\n");
		exit(-1);
	} else {
		printf("Listening successful.\n\n");
	}

	/* Infinite while loop for listening and accepting connection requests */
	while(1){

		/* Accept a connection */
		int connected_sock;
		connected_sock = accept(lstn_sock, NULL, NULL);
		if (connected_sock < 0) {
			printf("Error in accept()\n");
			exit(-1);
		} else {
			printf("Connection accepted.\n\n");
		}

		printf("Web Client Connected! Congratulations!! \n\n");


		/*Receive data */
		int rcv_status;
		char rcv_message[1024];
		char send_rcv_message[1024];
		rcv_status = recv(connected_sock, rcv_message, sizeof(rcv_message), 0);
		if (rcv_status < 0 ){
			printf("Error in recv()\n");
			exit(-1);
		} else {
			printf("Message from the client received.\n\n");
		}

		/* Making a copy of the HTTP request for sending it to the web Server */
		strcpy(send_rcv_message, rcv_message);

		//printf("PARSING THE REQUEST:\n%s\n\n", send_rcv_message);
		printf("Parsing the request to get Host, Path, and Server address.\n\n");
		/* Parse the HTTP Request from the Web Client to get hostname */
		char hostname[1024];
		char URL[1024];
		char PATH[1024];
		char type[1024];
		int i;


		/* Find and parse the Get request, isolating the URL for later use */
		char *path = strtok(send_rcv_message, "\r\n"); //\r\n means a new line
		char *path_name = strtok(path, " ");
		path_name = strtok(NULL, " ");
		char *version = strtok(NULL, " ");
		//printf("The Request is: %s\n", path);
		if (sscanf(path_name, "http://%s", URL) == 1){ //get the string after the http://
			printf("URL = %s\n", URL);
			printf("The Version is: %s\n\n", version);
		}
		/* Separate the host name from the PATH */
		for (i = 0; i < strlen(URL); i++){
			if (URL[i] == '/'){
				strncpy(hostname, URL, i);
				hostname[i] = '\0';
				break;
			}
		}
		printf("Host: %s\n", hostname);
		/* Store the PATH */
		bzero(PATH, 500); //TO CLEAR junk at the beginning of this buffer.
		for (; i < strlen(URL); i++){
			strcat(PATH, &URL[i]); //copy out the path
			break;
		}
		printf("Path: %s\n", PATH);

		/* Creating the TCP socket for connecting to the desired web server */
		// Address Initialization
		struct sockaddr_in web_server_addr;
		struct hostent *web_server;

		// Getting web server's Address by it's host name
		web_server = gethostbyname(hostname);
		if (web_server == NULL){
			printf("Error in gethostbyname() call.\n");
			exit(-1);
		} else {
			printf("Web Server address is: %s\n\n", web_server->h_name);
		}

		// Initialize socket structure
		bzero((char *) &web_server_addr, sizeof(web_server_addr));
		web_server_addr.sin_family = AF_INET;
		bcopy((char *) web_server->h_addr, (char *) &web_server_addr.sin_addr.s_addr, web_server->h_length);
		web_server_addr.sin_port = htons(80);

		// creating the proxy client socket
		int client_sock = socket(AF_INET, SOCK_STREAM, 0);
		if (client_sock < 0){
			printf("Error in creating socket for proxy client.\n");
			exit(-1);
		} else {
			printf("Client Socket for Web Server created.\n\n");
		}

		// Connecting to the web server's socket
		int connection_status;
		connection_status = connect(client_sock, (struct sockaddr *) &web_server_addr, sizeof(web_server_addr));
		if (connection_status < 0){
			printf("Error in connecting to the Web Server's socket\n\n");
			exit(-1);
		} else {
			printf("Connection to the Web Server successful.\n\n");
		}

		/* Creating the Head Message for the Web Server */
		char head_message[1024] = "HEAD ";
		strcat(head_message, path_name);
		strcat(head_message, " ");
		strcat(head_message, version);
		strcat(head_message, "\r\n");
		strcat(head_message, "Host: ");
		strcat(head_message, hostname);
		strcat(head_message, "\r\n\r\n");
		//printf("\nThe message for the WEB SERVER is: \n%s\n", head_message);

		/* Sending the HTTP request of the client to the web server */
		int web_send_status;
		web_send_status = send(client_sock, head_message, sizeof(head_message), 0);
		if (web_send_status < 0 ){
			printf("Error in sending HTTP request to the web server.\n\n");
			exit(-1);
		} else {
			printf("Successful in sending the Header request to the Web Server.\n\n");
		}

		/* Receiving the HTTP Response from the web server */
		char web_response_header[10000];
		int web_rcv_status;
		web_rcv_status = recv(client_sock, web_response_header, sizeof(web_response_header), 0);
		if (web_rcv_status < 0) {
			printf("There is an error in receiving the message of the web server.\n\n");
			exit(-1);
		} else {
			printf("Successful in receiving the Web response\n\n");
			printf("The Header web response is: \n%s\n\n", web_response_header);
		}
		/* Close the socket for getting the Header request */
		close(client_sock);
		printf("Client_sock for header request is closed.\n\n");
		//Clear the head_response header
		bzero(head_message, sizeof(head_message));

		/* Parse the header reponse */
		int file_size;
		char copy_message[1024];
		bzero(copy_message, sizeof(copy_message)); //clear it to make sure
		strcpy(copy_message, web_response_header);
		char *type_response = strtok(copy_message, " ");
		type_response = strtok(NULL, " ");//get to the response type
		printf("The type of responses from the web browser: %s\n\n", type_response);
		//printf("the copy message is:\n%s\n\n", copy_message);
		/************************************************************/
		/* Checkl for the type of HTTP response from the Web Server */
		/* If the response is a 200 OK then we can extract the      */
		/* Content-Type and Content-Length. Otherwise, if we are    */
		/* getting 301, 302, 304, 404 we are just going to send back*/
		/* to the web client whatever the response of the web server*/
		/************************************************************/
		if (strcmp(type_response, "200") == 0){
			/* Getting the type of the File */
			char *type1 = strstr(web_response_header, "Content-Type: ");
			if (type1){
				char type_content[1024];
				sscanf(type1, "Content-Type: %s\r\n", type_content);
				printf("Extracted Content-Type: %s\r\n", type_content);

				/* Guard to know if we are getting an HTML file */
				if (strcmp(type_content, "text/html;") == 0){
					/* Getting the size of File */
					char *size = strstr(web_response_header, "Content-Length: ");
					char length[1024];

					if(size){
						sscanf(size, "Content-Length: %s\r\n", length);
						file_size = atoi(length);
						printf("The size is : %d\r\n\r\n", file_size);
						sloxy = 1;
						printf("WE ARE GETTING AN HTML FILE!! SLOW DOWN THE RESPONSE!\n");
					}
				} else {
					sloxy = 0;
					printf("We are not getting an html file request.\r\n Message not modified\r\n\r\n");
				}
			}
		} else { //Else we are not going to slow down the request
			sloxy = 0;
		}


		/* Do the sloxy part by doing the range requests if it is an HTML file request */
		if (sloxy == 1){ // create multiple messages to the web server if sloxy = 1
			int start = 0;
			char full_message_for_client[10000];
			bzero(full_message_for_client, sizeof(full_message_for_client)); //clear it for next requests
			strcpy(full_message_for_client, web_response_header);

			//Iteration for the range requests starts
			for (int end = 99; end < file_size; end = end + 100) {
				printf("The Range Request goes...\n\n");

				char num_string[1024];
				char range_request[1024] = "GET ";
				strcat(range_request, PATH);
				strcat(range_request, " HTTP/1.1\r\nHost: ");
				strcat(range_request, hostname);
				strcat(range_request, "\r\nRange: bytes=");
				sprintf(num_string, "%d", start); //cast from int to string
				strcat(range_request, num_string);
				start = start + 100;
				sprintf(num_string, "%d", end); // cast from int to string
				strcat(range_request, "-");
				strcat(range_request, num_string);
				strcat(range_request, "\r\n\r\n");
				//printf("The range request:\r\n%s\r\n\r\n", range_request);

				/* Create another socket to connect to the web Server */
				int client_sock1 = socket(AF_INET, SOCK_STREAM, 0);
				if (client_sock1 < 0){
					printf("Error in creating socket for GET message.\n");
					exit(-1);
				} else {
					printf("Clinet Socket 1 for Web server created.\n\n");
				}

				/* Connecting to the Web Server's socket */
				int connection_status1;
				connection_status1 = connect(client_sock1, (struct sockaddr *) &web_server_addr, sizeof(web_server_addr));
				if (connection_status1 < 0){
					printf("Error in connecting to the Webserver's socket for the 2nd time\n\n");
					exit(-1);
				} else {
					printf("Connection to the web server successful\n\n");
				}

				/* Send the Get request to the Web SERVER */
				int web_send_status_get;
				web_send_status_get = send(client_sock1, range_request, strlen(range_request), 0);
				if (web_send_status_get < 0 ){
					printf("Error in sending HTTP GET request to the web server.\r\n");
					exit(-1);
				} else {
					printf("Successful in sending the GET request to the Web Server.\n\n");
				}

				/* Receiving the HTTP Response from the web server */
				char web_reply[10000];
				int web_status;
				bzero(web_reply, sizeof(web_reply)); //to make sure we get the proper responses
				web_status = recv(client_sock1, web_reply,sizeof(web_reply) ,0);
				if (web_status < 0){
					printf("There is an error in receeiving the GET response\r\n");
					exit(-1);
				} else {
					printf("Successful in receiving the web RESPONSE Range REQUEST\n\n");
					//printf("The Web Get RESPONSE is:\r\n%s\r\n\r\n", web_reply);
				}
				/* CLosing the socket connection with the web server */
				close(client_sock1);
				printf("Client Socket1 closed.\n\n");
				/* Parse the Body of the range request */
				char *body = strstr(web_reply, "\r\n\r\n"); //search for the empty line from the web_reply
				char part_body[1024];
				strcpy(part_body, body + 4); //start at index 4 to not copy the 2 new lines
				//printf("The body without the break line is:%s\n\n", part_body);
				strcat(full_message_for_client, part_body);
				//printf("The full message is:\n%s\n", full_message_for_client);
			}

			/*******************************************************************/
			/* Finish the rest of the request; the requested file size is < 100*/
			/*******************************************************************/

			/* Create the last Range request for the Web Server */
			char num_string_last[1024];
			char range_request_last[1024] = "GET ";
			strcat(range_request_last, PATH);
			strcat(range_request_last, " HTTP/1.1\r\nHost: ");
			strcat(range_request_last, hostname);
			strcat(range_request_last, "\r\nRange: bytes=");
			sprintf(num_string_last, "%d", start); //cast from int to string
			strcat(range_request_last, num_string_last);
			sprintf(num_string_last, "%d", file_size); // cast from int to string
			strcat(range_request_last, "-");
			strcat(range_request_last, num_string_last);
			strcat(range_request_last, "\r\n\r\n");
			//printf("\nThe last Range request is:\n%s\n", range_request_last);

			/* Create another socket to connect to the web Server */
			int client_sock2 = socket(AF_INET, SOCK_STREAM, 0);
			if (client_sock2 < 0){
				printf("Error in creating socket 2 for GET message.\n");
				exit(-1);
			} else {
				printf("Clinet Socket 2 for Web server created.\n\n");
			}

			/* Connecting to the Web Server's socket */
			int connection_status2;
			connection_status2 = connect(client_sock2, (struct sockaddr *) &web_server_addr, sizeof(web_server_addr));
			if (connection_status2 < 0){
				printf("Error in connecting to the Webserver's socket for the 3nd time\n\n");
				exit(-1);
			} else {
				printf("Connection 2 to the web server successful.\n\n");
			}

			/* Send the Get request to the Web SERVER */
			int web_send_status_get_last;
			web_send_status_get_last = send(client_sock2, range_request_last, strlen(range_request_last), 0);
			if (web_send_status_get_last < 0 ){
				printf("Error in sending last HTTP GET request to the web server.\r\n");
				exit(-1);
			} else {
				printf("Successful in sending the last GET request to the Web Server.\n\n");
			}

			/* Receiving the HTTP Response from the web server */
			char web_reply_last[10000];
			int web_status_last;
			bzero(web_reply_last, sizeof(web_reply_last));
			web_status_last = recv(client_sock2, web_reply_last, sizeof(web_reply_last) ,0);
			if (web_status_last < 0){
				printf("There is an error in receiving the last GET response\r\n");
				exit(-1);
			} else {
				printf("Successful in receiving the last web RESPONSE Range REQUEST\n\n");
				//printf("The Web Get RESPONSE is:\r\n%s\r\n\r\n", web_reply_last);
			}
			/* CLosing the socket connection with the web server */
			close(client_sock2);
			printf("Client Socket2 closed.\n\n");
			bzero(range_request_last, sizeof(range_request_last)); //clear to make sure

			/* Parse the Body of the range request response */
			//printf("The web response is:\n%s\n\n", web_reply_last);
			char *body_last = strstr(web_reply_last, "\r\n\r\n"); //search for the empty line from the web_reply
			char part_body_last[1024];
			strcpy(part_body_last, body_last + 4);//start at index 4 to not copy the 2 new lines
			//printf("The body without the break line is: %s\n\n", part_body_last);
			strcat(full_message_for_client, part_body_last);
			//printf("The full message is:\n%s\n", full_message_for_client);

			/* Sending the HTTP response to the client */
			int client_send_status;
			client_send_status = send(connected_sock, full_message_for_client, sizeof(full_message_for_client), 0);
			if (client_send_status < 0){
				printf("There is an error in sending the data to the web client");
				exit(-1);
			} else {
				printf("Message sent to the WEb Client!\n\n");
				bzero(full_message_for_client, sizeof(full_message_for_client));
			}
			/* CLosing the Socket Connection with the client */
			close(connected_sock);
			printf("Connected socket for web client is closed.\n");
		} else {
			/********************************************************************************/
			/* Else we are not getting an HTML file request so do not slow down the request */
			/* by sending the original request message and sending the response of the      */
			/* server with modifications                                                    */
			/********************************************************************************/

			/* Create another socket to connect to the web Server */
			int client_sock_original = socket(AF_INET, SOCK_STREAM, 0);
			if (client_sock_original < 0){
				printf("Error in creating socket for Original GET message.\n");
				exit(-1);
			} else {
				printf("Client Socket for sending Original message to Web server created.\n\n");
			}

			/* Connecting to the Web Server's socket */
			int connection_status_original;
			connection_status_original = connect(client_sock_original, (struct sockaddr *) &web_server_addr, sizeof(web_server_addr));
			if (connection_status_original < 0){
				printf("Error in connecting to the Webserver's socket for the Original message.\n\n");
				exit(-1);
			} else {
				printf("Connection for the Original message to the web server successful.\n\n");
			}

			/* Send the Get request to the Web SERVER */
			int web_send_status_original;
			//printf("The original message is:\n%s\n\n", rcv_message);
			web_send_status_original = send(client_sock_original, rcv_message, strlen(rcv_message), 0);
			if (web_send_status_original < 0 ){
				printf("Error in sending original HTTP GET request to the web server.\r\n");
				exit(-1);
			} else {
				printf("Successful in sending the original GET request to the Web Server.\n\n");
			}

			/* Receiving the HTTP Response from the web server */
			char web_response_original[10000];
			int web_status_original;
			bzero(web_response_original, sizeof(web_response_original));
			web_status_original = recv(client_sock_original, web_response_original, sizeof(web_response_original) ,0);
			if (web_status_original < 0){
				printf("There is an error in receiving the original GET response\r\n");
				exit(-1);
			} else {
				printf("Successful in receiving the web RESPONSE for the original request.\n\n");
				printf("The Original Web Get RESPONSE is:\r\n%s\r\n\r\n", web_response_original);
			}
			/* CLosing the socket connection with the web server */
			close(client_sock_original);
			printf("Client Socket original closed.\n\n");

			/* Sending the HTTP response to the client */
			int client_send_status_original;
			client_send_status_original = send(connected_sock, web_response_original, sizeof(web_response_original), 0);
			if (client_send_status_original < 0){
				printf("There is an error in sending the original data to the web client");
				exit(-1);
			} else {
				printf("Original Message sent to the WEb Client!\n\n");
				bzero(web_response_original, sizeof(web_response_original));
			}

			/* CLosing the Socket Connection with the client */
			close(connected_sock);
			printf("Connected socket for web client is closed.\n");
		}
	}
	/* Close the socket */
	close(lstn_sock);
	printf("linstener socket for CTRL+C press is closed.\n");
	return 0;
}
