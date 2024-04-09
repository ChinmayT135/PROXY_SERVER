#include "headers.h"

sem_t semaphore;                //used to put waiting threads to sleep and wake up
pthread_mutex_t mutex_lock;     //used for locking the cache

lru_cache_element* head;        //head pointer of the lru cache
lru_cache_element* tail;        //tail pointer of the lru cache

/************************************************************************************************
Function Name	: connect_to_server
Return Type		: int
Purpose			: connects to server & returns socket_fd of the server
*************************************************************************************************/
int connect_to_server(char* client_host, int server_port){

	int server_ID = socket(AF_INET, SOCK_STREAM, 0);
	if(server_ID < 0){
		printf("Error creating socket while connecting to server\n");
		return -1;
	}

	struct hostent *host = gethostbyname(client_host);
	if(host == NULL){
		printf("ERROR: no such host exist\n");
		return -1;
	}

	struct sockaddr_in server_addr; 

	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(server_port);

	// if we have ip of the client
	// inet_pton(AF_INET, client_ip, &server_addr.sin_addr);

	bcopy((char*)host->h_addr,(char*)&server_addr.sin_addr.s_addr,host->h_length);

	if(connect(server_ID, (struct sockaddr*)&server_addr, (socklen_t)sizeof(server_addr)) < 0){
		printf("ERROR: connection to the server failed\n");
		return -1;
	}

	return server_ID;
}

/************************************************************************************************
Function Name	: handle_client_request
Return Type		: int
Purpose			: reconstructs the client_request, continuously recvs data from server and sends to client
*************************************************************************************************/
int handle_client_request(int client_connection, HTTP_Request* request, char* client_request){

	char response[MAX_LEN] = {0};
	strcpy(response,"GET ");
	strcat(response, request->path);
	strcat(response," ");
	strcat(response, request->version);
	strcat(response,"\r\n"); 

	size_t response_length = strlen(response);

	// construct the client request from all the headers 
	if(set_header_for_http_request(request,"Connection","close")){
		printf("unable to set key in header\n");
		return -1;
	}

	if(get_header_from_http_request(request,"Host") == NULL){
		if(set_header_for_http_request(request,"Host",request->host)){
			printf("unable to set key in header\n");
			return -1;
		}
	}

	if(reconstruct_response_from_headers(request, response+response_length, (size_t)(MAX_LEN - response_length))){
		printf("Unable to fill complete buffer\n");
		return -1;
	}

	int server_port = 80; 		// Default port for further connection
	if(request->port != NULL)
		server_port = atoi(request->port);


	int remote_server_socketID = connect_to_server(request->host, server_port);

	if(remote_server_socketID < 0)
		return -1;

	close(remote_server_socketID);
	return -1;

	int bytes_to_send = send(remote_server_socketID,client_request, strlen(client_request), 0);
	
	bzero(response, MAX_LEN);

	bytes_to_send = recv(remote_server_socketID,response, MAX_LEN-1, 0);
	char* response_to_client = (char*)malloc(MAX_LEN*sizeof(char));
	int response_size = MAX_LEN;
	int response_index = 0;

	while(bytes_to_send > 0){
		bytes_to_send = send(client_connection, response, strlen(response), 0);

		for(int i=0 ; i<strlen(response) ; i++){
			response_to_client[response_index] = response[i];
			response_index++;
		}

		bzero(response,strlen(response));

		if(bytes_to_send < 0){
			perror("Error while sending response to client\n");
			break;
		}

		response_size += MAX_LEN;
		response_to_client = (char*)realloc(response_to_client,response_size);
		bytes_to_send = recv(remote_server_socketID, response, MAX_LEN-1,0);
	}

	response_to_client[response_size] = '\0';
	// response_to_client stores the actual response and will be used to store the request-response in cache
	//add_cache_element(response_to_client,strlen(response_to_client),client_request);
	free(response_to_client);

	return 1;
}
/************************************************************************************************
Function Name   : send_internal_error
Return Type     : void
Purpose         : sending error 500 in case of failure
*************************************************************************************************/
void send_internal_error(int client_fd){
	char error_msg[MAX_LEN] = {0};
	char current_time[100] = {0};
	time_t now = time(NULL);

	struct tm data = *gmtime(&now);

	strftime(current_time,sizeof(current_time),"%a, %d %b %Y %H:%M:%S %Z",&data);

	snprintf(error_msg,sizeof(error_msg),"HTTP/1.1 500 Internal Server Error\r\nContent-Length: 115\r\nConnection: close\r\nContent-Type: text/html\r\nDate: %s\r\nServer: SERVER-VI_V_VII_V\r\n\r\n<HTML><HEAD><TITLE>500 Internal Server Error</TITLE></HEAD>\n<BODY><H1>500 Internal Server Error</H1>\n</BODY></HTML>",current_time);
	send(client_fd,error_msg,strlen(error_msg),0);	

	return;
}
/************************************************************************************************
Function Name   : verify_HTTP_version
Return Type     : int
Purpose         : verifies if HTTP version is 1
*************************************************************************************************/
int verify_HTTP_version(char* version_string){

	if(strncmp(version_string,"HTTP/1.1",strlen("HTTP/1.1")) == 0)
		return 1;

	if(strncmp(version_string,"HTTP/1.0",strlen("HTTP/1.0")) == 0)
		return 1;

	return 0; // any case apart from http version 1.1 or 1.0
}
/************************************************************************************************
Function Name   : my_thread_function
Return Type     : void
Purpose         : handles each client request individually and parallely
*************************************************************************************************/
void* my_thread_function(void* client_socket_connection){

	sem_wait(&semaphore);

	int client_connection = *(int*)client_socket_connection;

	char* buffer = (char*)calloc(MAX_LEN,sizeof(char));

	bzero(buffer,MAX_LEN);
	int bytes_sent_by_client = recv(client_connection, buffer, MAX_LEN, 0);

	while(bytes_sent_by_client > 0){
		int len = strlen(buffer);

		if(strstr(buffer,"\r\n\r\n") == NULL){
			bytes_sent_by_client = recv(client_connection,buffer+len,MAX_LEN-len,0);
		}else
			break;
	}

	char tmp_buf[strlen(buffer)];
	bzero(tmp_buf,strlen(buffer));
	snprintf(tmp_buf,strlen(buffer)+1,"%s",buffer);

	lru_cache_element* temp = NULL;//find_in_cache(tmp_buf);

	if(temp != NULL){ // we found it in cache
		int total_length = temp->length;
		int position = 0;
		int response_len = 0;
		char response[MAX_LEN] = {0};
		while(position < total_length){
			bzero(response,MAX_LEN);
			for(int i=0 ; i<MAX_LEN ; i++){
				response[i] = temp->response[response_len];
				response_len++;
			}
			send(client_connection,temp->response,MAX_LEN,0);
		}
	}else if(bytes_sent_by_client > 0){ // we have to request to the actual server
		
		HTTP_Request* request = create_request(); // allocated memory
		if(parse_client_request(buffer,request)){
			perror("Parsing failed\n");
		}else{
			bzero(buffer,MAX_LEN);
			if(!strncmp(request->method,"GET",strlen("GET"))){
				if(request->host && request->path && verify_HTTP_version(request->version)){
					int ret_val = handle_client_request(client_connection,request,tmp_buf);
					if(ret_val == -1)
						send_internal_error(client_connection);
				}else
					send_internal_error(client_connection);
			}else{
				printf("This server only supports \"GET\" method.\nPlease try again\n");
			}
		}
		destroy_request(request);
	}else if(bytes_sent_by_client < 0){ // error in receiving
		perror("Error in receiving request from client\n");
	}else if(bytes_sent_by_client == 0){ // disconnected case
		printf("Client disconnected\n");
	}

	shutdown(client_connection, SHUT_RDWR);
	close(client_connection);

	free(buffer);

	sem_post(&semaphore);

	return NULL;
}
/************************************************************************************************
Function Name   : start_server
Return Type     : void
Purpose         : opens a connection for clients to connect and fetch request
*************************************************************************************************/
void start_server(int print_on_console){

	//init_cache();
	sem_init(&semaphore,0,MAX_CLIENTS);

	// initiate the proxy server socket
	int server_socket = socket(AF_INET, SOCK_STREAM,0);
	if(server_socket < 0){
		if(print_on_console)
			printf("ERROR establishing connection...\n");
		exit(1);
	}

	struct sockaddr_in server_addr;

	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(6575);
	server_addr.sin_addr.s_addr = INADDR_ANY;

	if(bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr))){
		if(print_on_console)
			printf("ERROR binding socket...\n");
		exit(1);
	}

	//socklen_t size = sizeof(server_addr);

	int listen_status = listen(server_socket,MAX_CLIENTS);
	if(listen_status<0){
		if(print_on_console)
			printf("ERROR while listening...\n");
		exit(1);
	}else{
		if(print_on_console)
			printf("Waiting for client connections...\n");
	}

	pthread_t fds_threads[MAX_CLIENTS];
	int client_connection_fds[MAX_CLIENTS];
	int fds_counter = 0;

	while(1){
		int client_socket = accept(server_socket,0,0);
		if(client_socket < 0){
			if(print_on_console)
				printf("ERROR while accpeting...\n");
			continue;
		}
		client_connection_fds[fds_counter] = client_socket;

		pthread_create(&fds_threads[fds_counter],NULL,my_thread_function,(void*)&client_connection_fds[fds_counter]);

		fds_counter++;
	}

	//destroy_cache();
	close(server_socket);

	return;
}

//int main(int argc, char* argv[]){
int main(){

	/*int start_daemon = 0;
    if(argc>1 && strncmp(argv[1],"run-daemon",10) == 0)
        start_daemon = 1;

    if(start_daemon){
        pid_t pid;

        pid = fork();

        if(pid<0)
            exit(EXIT_FAILURE);
	
        if(pid>0)
            exit(EXIT_SUCCESS);

        if(setsid() < 0)
            exit(EXIT_FAILURE);

        pid = fork();
        if(pid<0)
            exit(EXIT_FAILURE);

        if(pid>0){
			printf("Server initiated... pid: [%ld]\n",(int64_t)pid);
            exit(EXIT_SUCCESS);
        }

        umask(0777);
        chdir("/");

        start_server(0);
        return 0;
    }*/

    start_server(1);
	return 0;
}
