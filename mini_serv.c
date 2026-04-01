#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/select.h>

int extract_message(char **buf, char **msg);
char *str_join(char *buf, char *add);
void fatal();
void broadcast(int sender);

fd_set readfds, exceptfds;
int clients[FD_SETSIZE], count = 0, sockfd, maxfd;
char *msgs[FD_SETSIZE], in_buf[100001], out_buf[100001];


int main(int ac, char **av) {
	if (ac != 2) {
		write(2, "Wrong number of arguments\n", 26);
		exit(1);
	}

	int connfd;
	struct sockaddr_in servaddr, cli; 

	// socket create and verification 
	sockfd = socket(AF_INET, SOCK_STREAM, 0); 
	if (sockfd == -1) 
		fatal();
	bzero(&servaddr, sizeof(servaddr)); 

	// assign IP, PORT 
	servaddr.sin_family = AF_INET; 
	servaddr.sin_addr.s_addr = htonl(2130706433); //127.0.0.1
	servaddr.sin_port = htons(atoi(av[1]));

	// Binding newly created socket to given IP and verification 
	if ((bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr))) != 0)
		fatal();
	if (listen(sockfd, 10) != 0)
		fatal();

	FD_ZERO(&exceptfds);
	FD_SET(sockfd, &exceptfds);

	maxfd = sockfd;

	while(1) {
		readfds = exceptfds;

		if (select(maxfd + 1, &readfds, NULL, NULL, NULL) < 0)
			continue;
		for (int fd = 3; fd <= maxfd; fd++) {
			if (!FD_ISSET(fd, &readfds))
				continue;
			if (fd == sockfd) {
				socklen_t len = sizeof(cli);
				connfd = accept(sockfd, (struct sockaddr *)&cli, &len);
				if (connfd < 0)
					continue;
				if (connfd > maxfd)
					maxfd = connfd;
				FD_SET(connfd, &exceptfds);
				clients[connfd] = count++;
				msgs[connfd] = NULL;
				sprintf(out_buf, "server: client %d just arrived\n", clients[connfd]);
				broadcast(connfd);
				continue;
			}
			else {
				int rec_bytes = recv(fd, in_buf, 100000, 0);
				if (rec_bytes <= 0) {
					sprintf(out_buf, "server: client %d just left\n", clients[fd]);
					broadcast(fd);
					FD_CLR(fd, &exceptfds);
					close(fd);
					free(msgs[fd]);
					msgs[fd] = NULL;
					continue;
				}
				else {
					in_buf[rec_bytes] = '\0';
					char *msg = str_join(msgs[fd], in_buf);
					if (!msg) {
						for (int fd = 4; fd <= maxfd; fd++) {
							if (FD_ISSET(fd, &exceptfds))
								close(fd);
						}
						close(sockfd);
						fatal();
					}
					msgs[fd] = msg;

					int res;
					char *tmp;
					while ((res = extract_message(&(msgs[fd]), &tmp)) == 1) {
						sprintf(out_buf, "client %d: %s", clients[fd], tmp);
						broadcast(fd);
						free(tmp);
						tmp = NULL;
					}
					if (res == -1) {
						for (int i = 4; i <= maxfd; i++) {
							if (FD_ISSET(i, &exceptfds) && i != sockfd)
								close(i);
						}
						close(sockfd);
						fatal();
					}
				}
			}
		}
	}
}

int extract_message(char **buf, char **msg)
{
	char	*newbuf;
	int	i;

	*msg = 0;
	if (*buf == 0)
		return (0);
	i = 0;
	while ((*buf)[i])
	{
		if ((*buf)[i] == '\n')
		{
			newbuf = calloc(1, sizeof(*newbuf) * (strlen(*buf + i + 1) + 1));
			if (newbuf == 0)
				return (-1);
			strcpy(newbuf, *buf + i + 1);
			*msg = *buf;
			(*msg)[i + 1] = 0;
			*buf = newbuf;
			return (1);
		}
		i++;
	}
	return (0);
}

char *str_join(char *buf, char *add)
{
	char	*newbuf;
	int		len;

	if (buf == 0)
		len = 0;
	else
		len = strlen(buf);
	newbuf = malloc(sizeof(*newbuf) * (len + strlen(add) + 1));
	if (newbuf == 0)
		return (0);
	newbuf[0] = 0;
	if (buf != 0)
		strcat(newbuf, buf);
	free(buf);
	strcat(newbuf, add);
	return (newbuf);
}

void fatal() {
	write(2, "Fatal error\n", 12);
	exit(1);
}

void broadcast(int sender) {
	for (int fd = 3; fd <= maxfd; fd++) {
		if (FD_ISSET(fd, &exceptfds) && fd != sender && fd != sockfd)
			if (send(fd, out_buf, strlen(out_buf), 0) < 0)
				continue;
	}
}