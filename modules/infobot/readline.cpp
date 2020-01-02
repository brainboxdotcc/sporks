/************************************************************************************
 * 
 * Sporks, the learning, scriptable Discord bot!
 *
 * Copyright 2019 Craig Edwards <support@sporks.gg>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * Read characters from 'fd' until a newline is encountered. If a newline character
 * is not encountered in the first (n - 1) bytes, then the excess characters are
 * discarded. The returned string placed in 'buf' is null-terminated and includes
 * the newline character if it was read in the first (n - 1) bytes. The function
 * return value is the number of bytes placed in buffer (which includes the newline
 * character if encountered, but excludes the terminating null byte).
 *
 ************************************************************************************/

#include <cstddef>
#include <iostream>
#include <sporks/includes.h>
#include "infobot.h"

using namespace std;

/* Read line from socket, blocking */
size_t InfobotModule::readLine(int fd, char *buffer, size_t n)
{
	ssize_t numRead;				/* # of bytes fetched by last read() */
	size_t totRead;					/* Total bytes read so far */
	char *buf;
	char ch;
	buf = buffer;					/* No pointer arithmetic on "void *" */

	totRead = 0;
	for (;;) {
		numRead = read(fd, &ch, 1);

		if (numRead == -1) {
			if (errno == EINTR)		 /* Interrupted --> restart read() */
				continue;
			else {
				throw std::exception();
		}

		} else if (numRead == 0) {		/* EOF */
			if (totRead == 0)		/* No bytes read; return 0 */
				return 0;
			else				/* Some bytes read; add '\0' */
				break;

		} else {				/* 'numRead' must be 1 if we get here */
			if (totRead < n - 1) {	  	/* Discard > (n - 1) bytes */
				if (ch != '\n' && ch != '\r') {
					totRead++;
					*buf++ = ch;
				}
			}

			if (ch == '\n')
				break;
		}
	}

	*buf = '\0';
	return totRead;
}

/* Write line to socket, blocking */
bool InfobotModule::writeLine(int fd, const std::string &str)
{
	if (write(fd, str.data(), str.length()) < 0) {
		throw std::exception();
	} else {
		if (write(fd, "\r\n", 2) < 0) {
			throw std::exception();
		}
	}
	return true;
}

