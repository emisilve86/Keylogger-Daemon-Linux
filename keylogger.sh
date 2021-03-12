#!/bin/bash

##########################################################################
# Name:        Keylogger-Daemon-Linux                                    #
# Description:	is a keylogger daemon for Linux-based OS that            #
#              silently listens in the background for pressed            #
#              keyboard buttons.                                         #
# Author:      Emiliano Silvestri                                        #
#                                                                        #
# Copyright (C) 2021  Emiliano Silvestri                                 #
#                                                                        #
# This program is free software: you can redistribute it and/or modify   #
# it under the terms of the GNU General Public License as published by   #
# the Free Software Foundation, either version 3 of the License, or      #
# (at your option) any later version.                                    #
#                                                                        #
# This program is distributed in the hope that it will be useful,        #
# but WITHOUT ANY WARRANTY; without even the implied warranty of         #
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          #
# GNU General Public License for more details.                           #
#                                                                        #
# You should have received a copy of the GNU General Public License      #
# along with this program.  If not, see <https://www.gnu.org/licenses/>. #
##########################################################################

if [ $# -eq 0 ]; then

	echo "No arguments supplied."
	echo "Try with one of the following commands:  start | stop | status"
	exit 1

fi

if [ -z "$1" ]; then

	echo "The passed command is empty."
	echo "Try with one of the following commands:  start | stop | status"
	exit 1

fi

if [ "$1" == "start" ]; then

	klpid=`pgrep -x k3yl0gg3r`

	if [ -z "$klpid" ]; then
		sudo bin/k3yl0gg3r
	else
		echo "The daemon is already running."
	fi

elif [ "$1" == "stop" ]; then

	klpid=`pgrep -x k3yl0gg3r`

	if [ -z "$klpid" ]; then
		echo "The daemon is not running."
	else
		sudo kill $klpid
	fi

elif [ "$1" == "status" ]; then

	klpid=`pgrep -x k3yl0gg3r`

	if [ -z "$klpid" ]; then
		echo "The daemon is not running."
	else
		echo "The daemon is running with PID: $klpid"
	fi

else

	echo "Unrecognized command."
	echo "Try with one of the following commands:  start | stop | status"
	exit 1

fi