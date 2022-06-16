#include <unistd.h>
#include <errno.h>

#define BUF_SIZE 8192

// Unbuffered input and output functions
ssize_t readn(int fd, const void * usrbuf, size_t n){
	size_t nleft = n;
	ssize_t nread;
	char * buf = usrbuf;
	while (nleft > 0){
		if ((nread = read(fd, buf, nleft)) < 0){
			if (errno == EINTR){nread = 0;} //  Interrupted by sig handler return -> call read again
			else{return -1;}
		}else if (!nread){break;} // EOF
		nleft -= nread;
		buf += nread;
	}
	return n - nleft;
}

ssize_t writen(int fd, const void * usrbuf, size_t n){
	size_t nleft = n;
	ssize_t nwriten;
	char * buf = userbuf;
	while (nleft > 0){
		if ((nwriten = write(fd, buf, nleft)) <= 0){
			if (errno == EINTR){nwriten = 0;} //  Interrupted by sig handler return -> call write again
			else{return -1;}
		}
		nleft -= nwriten;
		buf += nwriten;
	}
	return n - nwriten;
}

// Buffered input functions
/* The buf_read function copies n bytes from the file descriptor fd to an internal buffer (which is associated with fd by "struct rio"), automatically refill
the buffer when it is empty. Whenever we request bytes from fd, it can be served directly from "struct rio"
*/
struct rio{
	int fd;					// file descriptor
	int cnt;				// number of unread bytes in buf
	char * ptr;				// pointer to next unread byte
	char buf[BUF_SIZE];		// internal buffer
};

void rio_init(rio * rp, int fd){
	rp->fd = fd;
	rp->cnt = 0;
	rp->ptr = rp->buf;
}

ssize_t buf_read(rio * rp, const char * usrbuf, size_t n){
	int count = n;
	
	// refill rp->buf when empty / there is no unread bytes
	while (rp->cnt <= 0){
		if ((rp->cnt = read(rp->fd, rp->buf, sizeof(rp->buf))) < 0){
			if (errno != EINTR){return -1;}
		}else if (!count){return 0;} // EOF
		else{rp->ptr = rp->buf;} // reset rp->ptr
	}
	
	// copy min(n, rp->cnt) to usrbuf
	if (rp->cnt < n){count = rp->cnt;}
	memcpy(usrbuf, rp->ptr, count);
	rp->cnt -= count;
	rp->ptr += count;
	return count;
}

// read at most (len - 1) characters on one line and null-terminate it
ssize_t buf_readline(rio * rp, const char * usrbuf, size_t len){
	int n = 1;
	ssize_t nread;
	char c, *buf = usrbuf;
	for (; n < len; ++n){
		if ((nread = buf_read(rp, &c, 1)) > 0){ // read only one char
			*buf++ = c;
			if (c == '\n'){
				++n;
				break;
			}
		}else if (!nread){
			if (n == 1){return 0;} 	// EOF at the start
			else{break;} 			// already process some data
		}else{return -1;}
	}
	*buf = '\0';
	return n - 1;
}

// consecutively use buf_read to read n bytes from rp to usrbuf
ssize_t buf_readn(rio * rp, const char * usrbuf, size_t n){
	size_t nleft = n;
	ssize_t nread;
	char * buf = usrbuf;
	while (nleft > 0){
		if ((nread = buf_read(rp, buf, nleft)) < 0){return -1;} // since buf_read already detects error
		else if (!nread){break;}
		nleft -= nread;
		buf += nread;
	}
	return n - nleft;
}
