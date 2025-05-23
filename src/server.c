#include <fcntl.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>

int main() {

  // Create socket with socket().
  int s = socket(AF_INET, SOCK_STREAM, 0);
  if (s == -1) {
    perror("socket failure");
    exit(1);
  }

  struct sockaddr_in addr = {
      .sin_family = AF_INET,
      .sin_port = htons(3111),
      .sin_addr.s_addr = 0,
  };

  // bind() to assign scoket to a port and IP address.
  if (bind(s, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
    perror("binding error");
    close(s);
    exit(1);
  }

  // listen() to make the socket as passive, ready to accept connections
  // 10 is the backlog queue size, could be any number.
  if (listen(s, 10) < 0) {
    perror("listen error");
    close(s);
    exit(1);
  }

  // accept() to wait for a client to connect.
  int client_fd = accept(s, NULL, NULL);
  if (client_fd < 0) {
    perror("accept error");
    close(s);
    exit(1);
  }

  char buffer[256] = {0};

  // recv() or read() to read data from the client socket into a buffer.
  ssize_t received = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
  if (received <= 0) {
    perror("recv error or connection closed");
    close(client_fd);
    close(s);
    exit(1);
  }

  // Assuming the buffer contains something like "GET /filename HTTP/1.1"
  //
  // Parse that line to extract HTTP method (i.e. GET), the requested resource
  // path and HTTP version.
  char *f = buffer + 5;
  char *space = strchr(f, ' ');
  if (space) {
    *space = '\0';
  } else {
    fprintf(stderr, "Invalid request format\n");
    close(client_fd);
    close(s);
    exit(1);
  }

  // if file exists, read its contents and send it back over the client socket.
  int opened_fd = open(f, O_RDONLY);
  if (opened_fd < 0) {
    perror("open file error");
    close(client_fd);
    close(s);
    exit(1);
  }

  off_t offset = 0;
  off_t bytes_to_send = 256; // or better: get file size or limit bytes to send

  int result = sendfile(opened_fd, client_fd, offset, &bytes_to_send, NULL, 0);
  if (result == -1) {
    perror("sendfile error");
  }

  // Close the client socket after serving the request.
  close(opened_fd);
  close(client_fd);
  close(s);

  return 0;
}
