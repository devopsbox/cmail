#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#define LISTEN_QUEUE_LENGTH 		128

typedef struct /*_CONNECTION*/ {
	int fd;
	void* aux_data;
} CONNECTION;
