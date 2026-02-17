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

static const char *prog_name;
static int quiet = 0;

void print_error_and_exit(const char *format, ...) {
  va_list args;
  va_start(args, format);
  vfprintf(stderr, format, args);
  va_end(args);
  exit(EXIT_FAILURE);
}

void print_usage_and_exit(const char *msg) {
  if (msg)
    fprintf(stderr, "%s\n", msg);

  print_error_and_exit("Usage: %s [-q] PID1 [PID2, PID3, ...]\n", prog_name);
}

void parse_args(int argc, char **argv) {
  int opt;

  while((opt = getopt(argc, argv, "q")) != -1) {
    switch(opt) {
      case 'q':
        quiet = 1;
        break;
      default:
        print_usage_and_exit(NULL);
    }
  }
}

pid_t parse_pid(const char *s) {
  long pid;

  errno = 0;
  pid = strtol(s, NULL, 10);

  if (errno == EINVAL || errno == ERANGE || pid < 0 || pid > INT_MAX)
    print_error_and_exit("Error: invalid PID: %s\n", s);

  return (pid_t)pid;
}

void print_pids(int n_pids, pid_t *pids) {
  int i;

  printf("Waiting for PID%s ", n_pids == 1 ? "" : "s");

  for (i = 0; i < n_pids; ++i)
    printf("%d ", pids[i]);

  puts("to exit...");
}

void kq_waitpids(int n_pids, pid_t *pids) {
  struct kevent *pid_list;
  struct kevent *event_list;
  int i, kq, n, n_events;

  pid_list = malloc(n_pids * sizeof(struct kevent));
  if (!pid_list)
    print_error_and_exit("malloc() failed\n");

  event_list = malloc(n_pids * sizeof(struct kevent));
  if (!event_list)
    print_error_and_exit("malloc() failed\n");

  if (!quiet)
    print_pids(n_pids, pids);

  kq = kqueue();

  if (kq == -1) {
    perror("kqueue");
    exit(EXIT_FAILURE);
  }

  for (i = 0; i < n_pids; ++i)
    EV_SET(&pid_list[i], pids[i], EVFILT_PROC, EV_ADD | EV_ENABLE, NOTE_EXIT, 0,
           NULL);

  errno = 0;
  if (kevent(kq, pid_list, n_pids, NULL, 0, NULL) == -1) {
    perror("kevent");
    exit(EXIT_FAILURE);
  }

  for (i = 0; i < n_pids;) {
    errno = 0;
    n_events = kevent(kq, NULL, 0, event_list, n_pids, NULL);

    if (n_events == -1) {
      perror("kevent");
      exit(EXIT_FAILURE);
    } else if (n_events > 0) {
      i += n_events;

      for (n = 0; n < n_events; ++n) {
        if (event_list[n].flags & EV_ERROR) {
          print_error_and_exit("Event error: %s\n",
                               strerror(event_list[n].data));
        } else {
          if (!quiet)
            printf("PID %lu has exited...\n", event_list[n].ident);
        }
      }
    }
  }

  free(pid_list);
  free(event_list);
  close(kq);
}

int main(int argc, char **argv) {
  pid_t *pids;
  int i, n_pids;

  prog_name = argv[0];

  parse_args(argc, argv);

  n_pids = argc - optind;

  if (n_pids < 1)
    print_usage_and_exit("Error: no PIDs provided\n");

  pids = malloc((sizeof(pid_t)) * n_pids);
  if (!pids)
    print_error_and_exit("malloc() failed\n");

  for (i = 0; optind < argc; ++optind, ++i)
    pids[i] = parse_pid(argv[optind]);

  kq_waitpids(n_pids, pids);

  free(pids);
  return EXIT_SUCCESS;
}
