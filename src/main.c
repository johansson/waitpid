/*
 * Copyright (c) 2026 Will Johansson
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.

 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#include <errno.h>
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/event.h>
#include <unistd.h>

void print_error_and_exit(const char *format, ...) {
  va_list args;
  va_start(args, format);
  vfprintf(stderr, format, args);
  va_end(args);
  exit(EXIT_FAILURE);
}

void kq_waitpid(pid_t pid) {
  struct kevent event;
  struct kevent tevent;
  int kq, ret;

  printf("Waiting for PID %d to exit...\n", pid);

  kq = kqueue();

  if (kq == -1) {
    perror("kqueue");
    exit(EXIT_FAILURE);
  }

  EV_SET(&event, pid, EVFILT_PROC, EV_ADD | EV_ENABLE, NOTE_EXIT, 0, NULL);

  errno = 0;
  if (kevent(kq, &event, 1, NULL, 0, NULL) == -1) {
    perror("kevent");
    exit(EXIT_FAILURE);
  }

  errno = 0;
  ret = kevent(kq, NULL, 0, &tevent, 1, NULL);

  if (ret == -1) {
    perror("kevent");
    exit(EXIT_FAILURE);
  } else if (ret > 0) {
    if (tevent.flags & EV_ERROR) {
      print_error_and_exit("Event error: %s\n", strerror(event.data));
    } else {
      printf("PID %d has exited...\n", pid);
    }
  }

  close(kq);
}

int main(int argc, char **argv) {
  long _pid;
  pid_t pid;

  if (argc != 2)
    print_error_and_exit("Usage: %s PID\n", argv[0]);

  errno = 0;
  _pid = strtol(argv[1], NULL, 10);

  if (errno == EINVAL || errno == ERANGE || _pid < 0 || _pid > INT_MAX)
    print_error_and_exit("Error: invalid PID %s\n", argv[1]);

  pid = (pid_t)_pid;

  kq_waitpid(pid);

  return EXIT_SUCCESS;
}
