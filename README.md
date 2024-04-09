<h1>Implemented Proxy Server in C language</h1>
<h2>Working on implementation of cache using doubly-linked-list</h2>

##### Introduction
I was always curious about how a server is built and how the data is tranfered from server to client and vice versa. This project was built with intention to support multiple clients at once and my personal motive being understanding the functionalities that goes into implementing the server and specifically using Least Recently Used cache (LRU) with memory management to serve faster responses.

##### Implementation of multi-threading
- Used pthread_create function with combination of sem_wait and sem_post functions to avoid thread clash

##### Motivation for Project
- To Understand how multithreaded-server is built and sustains multiple concurrent client connection
- Understanding implementation of semaphore with multithreads
- To build a custom cache using doubly-linked list and manage cache space storage

 
##### OS Component Used ​
- Implemented:
	- Threading
	- Semaphore
- Implementation ongoing:
	- Cache (LRU cache using doubly-linked-list)
	- Locks for cache
	- Cache size management

##### Limitations ​
- This implementation does not support 'CONNECT' request which is rejected, hence fetching sites fails for large websites
- There is no database so storing large 'GET' requests is not possible

##### How this project can be extended? ​
- Handling CONNECT operation and then following up with GET requests
- Handling POST operation through a proxy server


## How to Run

```bash
$ (clone the repository)
$ cd PROXY_SERVER
$ make
$ ./util_server
```
`Open any site which has only GET request or manually perform a GET request with required URL parameters and header parameters`

# Note:
- This code will only run on LINUX machine.

I have referred to <a href = "https://github.com/Lovepreet-Singh-LPSK/MultiThreadedProxyServerClient">Project</a> for my base code and further implementation.

#### Reviews if any from all the coding enthusiats are always welcomed !!
