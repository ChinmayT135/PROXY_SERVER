#include "headers.h"

/************************************************************************************************
Function Name   : init_cache
Return Type     : void
Purpose         : initiate the cache and allocate memory to the cache for the server
*************************************************************************************************/
void init_cache(){
	head = (lru_cache_element*)malloc(sizeof(lru_cache_element));
	tail = (lru_cache_element*)malloc(sizeof(lru_cache_element));

	head->next = tail;
	head->prev = NULL;
	tail->prev = head;
	tail->next = NULL;
}
/************************************************************************************************
Function Name   : destroy_cache
Return Type     : void
Purpose         : free the memory of cache for the server
*************************************************************************************************/
void destroy_cache(){
	pthread_mutex_lock(&mutex_lock);

	while(tail->prev != head){
		lru_cache_element* temp = tail->prev;
		free(temp->url);
		free(temp->response);
		
		tail->prev = temp->prev;
		temp->prev->next = tail;
		free(temp);
	}

	head->next = NULL;
	tail->prev = NULL;
	free(head);
	free(tail);

	pthread_mutex_unlock(&mutex_lock);
}
/************************************************************************************************
Function Name   : find_in_cache
Return Type     : lru_cache_element*
Purpose         : search the cache for client request
*************************************************************************************************/
lru_cache_element* find_in_cache(char* buffer){
	pthread_mutex_lock(&mutex_lock);
	lru_cache_element* requested = NULL;

	if(head->next != tail){
		requested = head->next;
		while(requested != tail){
			if(strcmp(requested->url,buffer) == 0){
				add_element_to_begin(requested);
				break;
			}
			requested = requested->next;
		}
	}else if(head->next == tail)
		printf("\ncache is empty\n");

	pthread_mutex_unlock(&mutex_lock);
	return requested;
}
/************************************************************************************************
Function Name   : remove_last_element
Return Type     : void
Purpose         : remove the last element from the cache
*************************************************************************************************/
void remove_last_element(){
	pthread_mutex_lock(&mutex_lock);
	lru_cache_element* last = tail->prev;
	
	tail->prev = last->prev;
	last->prev->next = tail;

	free(last->url);
	free(last->response);
	free(last);
	pthread_mutex_unlock(&mutex_lock);
}
/************************************************************************************************
Function Name   : add_element_to_begin
Return Type     : void
Purpose         : add an element to the beginning of the list
*************************************************************************************************/
void add_element_to_begin(lru_cache_element* element){

	lru_cache_element* nxt = element->next;
	lru_cache_element* prv = element->prev;

	prv->next = nxt;
	nxt->prev = prv;

	lru_cache_element* top = head->next;
	element->next = top;
	element->prev = head;
	head->next = element;
	top->prev = element;

}
/************************************************************************************************
Function Name   : add_cache_element
Return Type     : int
Purpose         : add an element to the cache
*************************************************************************************************/
int add_cache_element(char* response, int length, char* url){

	pthread_mutex_lock(&mutex_lock);
	lru_cache_element* new_node = (lru_cache_element*)malloc(sizeof(lru_cache_element));

	new_node->url = (char*)malloc(strlen(url)*sizeof(char*));
	new_node->response = (char*)malloc(strlen(response)*sizeof(char*));
	new_node->length = length;

	memcpy(new_node->url,url,strlen(url));
	memcpy(new_node->response,response,strlen(response));

	lru_cache_element* top = head->next;
	head->next = new_node;
	top->prev = new_node;
	new_node->prev = head;
	new_node->next = top;

	pthread_mutex_unlock(&mutex_lock);
	return 0;
}
