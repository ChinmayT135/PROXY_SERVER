#include "headers.h"

#define MIN_BUFFER_LEN 4
/************************************************************************************************
Function Name   : reconstruct_response_from_headers
Return Type     : int
Purpose         : from parsed headers, it reconstructs the client request
*************************************************************************************************/
int reconstruct_response_from_headers(HTTP_Request* request, char* buffer, size_t buffer_len){

	if(request == NULL || request->buf == NULL)
		return 1;

	char temp_buf[MAX_LEN] = {0};
	struct Headers* header;
	size_t counter = 0;

	while(request->headersused>counter){
		header = request->headers+counter;
		if(header->key){
			strncat(temp_buf,header->key,strlen(header->key));
			strncat(temp_buf,": ",strlen(": "));
			strncat(temp_buf,header->value,strlen(header->value));
		}
		if(strlen(temp_buf) > buffer_len)
			return 1;
		counter++;
	}

	if(strlen(temp_buf)+2 <= buffer_len){ 
		strcat(buffer,temp_buf);
		strcat(buffer,"\r\n");
		return 0;
	}else
		return 1;
}
/************************************************************************************************
Function Name   : destroy_single_header
Return Type     : void
Purpose         : free up the memory of header
*************************************************************************************************/
void destroy_single_header(struct Headers* header){
	if(header->key != NULL){
		free(header->key);
		header->key = NULL;
		free(header->value);
		header->value = NULL;
		header->keylen = 0;
		header->valuelen = 0;
	}
}
/************************************************************************************************
Function Name   : destory_headers
Return Type     : void
Purpose         : free memory for all headers
*************************************************************************************************/
void destroy_headers(HTTP_Request* request){
	size_t counter = 0;
	while(request->headersused > counter){
		destroy_single_header(request->headers+counter);
		counter++;
	}

	request->headersused = 0;
	request->headerslen = 0;
	free(request->headers);
}
/************************************************************************************************
Function Name   : destroy_request
Return Type     : void
Purpose         : free memory for HTTP_Request struct
*************************************************************************************************/
void destroy_request(HTTP_Request* request){
	if(request->buf != NULL)
		free(request->buf);

	if(request->path != NULL)
		free(request->path);

	if(request->headerslen > 0)
		destroy_headers(request);

	free(request);
}
/************************************************************************************************
Function Name   : create_headers_for_request
Return Type     : void
Purpose         : allocated memory for a single header
*************************************************************************************************/
void create_headers_for_request(HTTP_Request* temp){
	temp->headers = (struct Headers*)malloc(sizeof(struct Headers)*HEADER_LEN);

	temp->headerslen = HEADER_LEN;
	temp->headersused = 0;
}
/************************************************************************************************
Function Name   : create_request
Return Type     : HTTP_Request*
Purpose         : allocates memory to HTTP_Request variable
*************************************************************************************************/
HTTP_Request* create_request(){
	HTTP_Request* temp = (HTTP_Request*)malloc(sizeof(HTTP_Request));

	if(temp != NULL){
		temp->buflen = 0;
		temp->method = NULL;
		temp->protocol = NULL;
		temp->host = NULL;
		temp->port = NULL;
		temp->path = NULL;
		temp->version = NULL;
		temp->buf = NULL;
		create_headers_for_request(temp);
	}
	return temp;
}
/************************************************************************************************
Function Name   : get_header_from_http_request
Return Type     : struct Headers*
Purpose         : fetch the required header with specific key
*************************************************************************************************/
struct Headers* get_header_from_http_request(HTTP_Request* request,const char* key){

	if(!key)
		return NULL;

	size_t counter = 0;
	struct Headers* temp;
	while(request->headersused>counter){
		temp = request->headers+counter;
		if(temp->key && strcmp(temp->key,key)==0)
			return temp;
		counter++;
	}
	return NULL;
}
/************************************************************************************************
Function Name   : remove_header
Return Type     : int
Purpose         : free memory for a struct Headers
*************************************************************************************************/
int remove_header(HTTP_Request* request, const char* key){
	struct Headers* temp = get_header_from_http_request(request,key);

	if(temp==NULL)
		return 1;

	free(temp->key);
	free(temp->value);

	return 0;
}
/************************************************************************************************
Function Name   : set_header_for_http_request
Return Type     : int
Purpose         : allocate memory and store a single header
*************************************************************************************************/
int set_header_for_http_request(HTTP_Request* request, char* key, char* value){
// Failure: return 1 
// Success: return 0
	if(request->headerslen <= request->headersused + 1){
		request->headerslen = request->headerslen*2;
		request->headers = (struct Headers*)realloc(request->headers,request->headerslen*sizeof(struct Headers));
		if(!request->headers)
			return 1;
	}

	struct Headers* temp;
	remove_header(request,key);

	temp = request->headers + request->headersused;
	request->headersused += 1;

	temp->keylen = strlen(key)+1;
	temp->valuelen = strlen(value)+1;

	temp->key = (char*)malloc(temp->keylen);
	memcpy(temp->key,key,strlen(key));
	temp->key[strlen(key)] = '\0';

	temp->value = (char*)malloc(temp->valuelen);
	memcpy(temp->value,value,strlen(value));
	temp->value[strlen(value)] = '\0';

	return 0;
}
/************************************************************************************************
Function Name   : parse_headers_for_client_request
Return Type     : int
Purpose         : parse the client request and store all the headers in that request
*************************************************************************************************/
int parse_headers_for_client_request(HTTP_Request* request, char* curr_header){
	// failure case : return 1 AND success case : return 0

	char* tok = strchr(curr_header,':');
	if(tok == NULL){
		printf("ERROR : no ':' found\n");
		return 1;
	}

	char* key = (char*)malloc((tok-curr_header+1)*sizeof(char));
	memcpy(key,curr_header,tok-curr_header);
	key[tok-curr_header] = '\0';

	char* tok2 = strchr(tok+1,"\r\n");
	if(tok2 == NULL){
		printf("ERROR : no \"\r\n\" found\n");
		return 1;
	}

	char* value = (char*)malloc((tok2-(tok+1)+1)*sizeof(char));
	memcpy(value,tok+1,tok2-(tok+1));
	value[tok2-(tok+1)] = '\0';

	set_header_for_http_request(request,key,value);

	free(key);
	free(value);

	return 0;
}
/************************************************************************************************
Function Name   : parse_client_request
Return Type     : int
Purpose         : parse the whole client request
*************************************************************************************************/
int parse_client_request(char* client_request, HTTP_Request* request){
	// for failure case return 1 and for success case return 0
	if(strlen(client_request) < MIN_BUFFER_LEN || strlen(client_request) > (MAX_LEN*10)){
		printf("Invalid buffer length\n");
		return 1;
	}

	if( request->buf != NULL){
		printf("the request object is already assigned a value\n");
		return 1;
	}

	char* temp_buffer = (char*)malloc(strlen(client_request) + 1);
	memcpy(temp_buffer,client_request,strlen(client_request)+1);
	temp_buffer[strlen(client_request)] = '\0';	

	char* token = strstr(temp_buffer,"\r\n\r\n");

	if(token == NULL){
		printf("ERROR : Not a valid client request. Reason: no end of headers\n");
		free(temp_buffer);
		return 1;
	}

	token = strstr(temp_buffer,"\r\n");
	if(request->buf == NULL){
		request->buf = (char*)malloc((token-temp_buffer)+1);
		request->buflen = (token-temp_buffer)+1;
		memcpy(request->buf,temp_buffer,token-temp_buffer);
		request->buf[token-temp_buffer] = '\0';
	}

	char* str_tok;
	request->method = strtok_r(request->buf," ",&str_tok);
	if(request->method == NULL){
		printf("ERROR : Not a valid client request. Reason: no whitespace encountered\n");
        free(temp_buffer);
		if(request->buf != NULL){
			free(request->buf);
			request->buf = NULL;
		}
        return 1;
	}

	if(strncmp(request->method,"GET",strlen("GET") != 0)){
		printf("ERROR : Not a valid client request. Reason: only GET request is accepted\n");
        free(temp_buffer);
        if(request->buf != NULL){
            free(request->buf);
            request->buf = NULL;
        }
        return 1;
	}

	char* address = strtok_r(NULL," ",&str_tok);

	if(address == NULL){
        printf("ERROR : Not a valid client request. Reason: not able to find address\n");
        free(temp_buffer);
        if(request->buf != NULL){
            free(request->buf);
            request->buf = NULL;
        }
        return 1;
	}

	request->version = address + strlen(address) + 1;

	if(request->version == NULL){
        printf("ERROR : Not a valid client request. Reason: missing http version\n");
        free(temp_buffer);
        if(request->buf != NULL){
            free(request->buf);
            request->buf = NULL;
        }
        return 1;
	}

	if(strncmp(request->version, "HTTP/" , strlen("HTTP/")) != 0){
        printf("ERROR : Not a valid client request. Reason: http version not supported\n");
        free(temp_buffer);
        if(request->buf != NULL){
            free(request->buf);
            request->buf = NULL;
        }
        return 1;
	}

	request->protocol = strtok_r(address,"://", &str_tok);

	if(request->protocol == NULL){
        printf("ERROR : Not a valid client request. Reason: not secured protocol\n");
        free(temp_buffer);
        if(request->buf != NULL){
            free(request->buf);
            request->buf = NULL;
        }
        return 1;
	}

	const char* url = address + strlen(request->protocol) + strlen("://");
	size_t url_length = strlen(url);

	request->host = strtok_r(NULL, "/", &str_tok);
	if(request->host == NULL){
        printf("ERROR : Not a valid client request. Reason: missing url\n");
        free(temp_buffer);
        if(request->buf != NULL){
            free(request->buf);
            request->buf = NULL;
        }
        return 1;
	}

    if(strlen(request->host) == url_length){
        printf("ERROR : Not a valid client request. Reason: invalid url\n");
        free(temp_buffer);
        if(request->buf != NULL){
            free(request->buf);
            request->buf = NULL;
        }
        return 1;
    }

	request->path = strtok_r(NULL," ",&str_tok);

	if(request->path == NULL){ 
		request->path = (char*)malloc(strlen("/")+1);
		strncpy(request->path, "/", strlen("/")+1);
	}else if(strncmp(request->path, "/" , strlen("/")) == 0){
		printf("ERROR : Not a valid client request. Reason: 2 slash characters encountered, invalid url\n");
        free(temp_buffer);
        if(request->buf != NULL){
            free(request->buf);
            request->buf = NULL;
        }
		request->path = NULL;
        return 1;
	}else{
		char* temp_path = request->path;
		int request_path_len = strlen(request->path);
		request->path = (char*)malloc(strlen("/") + request_path_len + 1);
		strncpy(request->path, "/", strlen("/"));
		strncpy(request->path + strlen("/"), temp_path, request_path_len+1);
	}

	request->host = strtok_r(request->host, ":", &str_tok);

	if(request->host == NULL){
		printf("ERROR : Not a valid client request. Reason: invalid url\n");
        free(temp_buffer);
        if(request->buf != NULL){
            free(request->buf);
            request->buf = NULL;
        }
		if(request->path != NULL){
			free(request->path);
			request->path = NULL;
		}
        return 1;
	}

	request->port = strtok_r(NULL,"/",&str_tok);

	if(request->port != NULL){
		int port = strtol(request->port, (char**)NULL, 10);
		if(port == 0){
			printf("ERROR : Not a valid client request. Reason: invalid url\n");
    	    free(temp_buffer);
	        if(request->buf != NULL){
            	free(request->buf);
        	    request->buf = NULL;
    	    }
	        if(request->path != NULL){
            	free(request->path);
        	    request->path = NULL;
    	    }
	        return 1;
		}
	}

	// Parsing the headers 
	char* curr_header = strstr(temp_buffer, "\r\n") + 2;

	while(curr_header[0] != '\0' && !(curr_header[0] == '\r' && curr_header[1] == '\n')){

		if(parse_headers_for_client_request(request,curr_header))
			break;

		curr_header = strstr(curr_header,"\r\n");
		if(curr_header == NULL || strlen(curr_header) < 2)
			break;
		curr_header += 2;
	}

	free(temp_buffer);
	return 0;
}
