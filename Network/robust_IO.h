#ifndef ROBUST_IO_H_
#define ROBUST_IO_H_

#define BUF_SIZE 8192
typedef struct rio rio;
ssize_t readn(int, const void *, size_t);
ssize_t writen(int, const void *, size_t);

struct rio{
	int fd;					// file descriptor
	int cnt;				// number of unread bytes in buf
	char * ptr;				// pointer to next unread byte
	char buf[BUF_SIZE];		// internal buffer
};

void rio_init(rio *, int);
ssize_t buf_read(rio *, const void *, size_t);
ssize_t buf_readline(rio *, const void *, size_t);
ssize_t buf_readn(rio *, const void *, size_t);

#endif
