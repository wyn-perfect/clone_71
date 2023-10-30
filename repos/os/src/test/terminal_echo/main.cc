/*
 * \brief  Terminal echo program
 * \author Norman Feske
 * \author Martin Stein
 * \date   2009-10-16
 */

/*
 * Copyright (C) 2009-2017 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

/* Genode includes */
#include <base/component.h>
#include <terminal_session/connection.h>

using namespace Genode;

struct Main
{
	Terminal::Connection terminal;
	Signal_handler<Main> read_avail;
	char                 read_buffer[256];

	String<128> intro {
		"--- Terminal echo test started - now you can type characters to be echoed. ---\n" };

	void handle_read_avail()
	{
		size_t num_bytes = terminal.read(read_buffer, sizeof(read_buffer));
		// log("got ", num_bytes, " byte(s)");
		size_t carriage_index = 0;
		for (size_t i = 0; i < num_bytes; i++) {
			// Genode::log((int)read_buffer[i]);
			if (read_buffer[i] == '\r' || read_buffer[i] == '\n') {
				carriage_index = i;
				// terminal.write("\n", 1); 
				break;
			}
			// terminal.write(&read_buffer[i], 1);
		}
		read_buffer[carriage_index] = '\n';
		read_buffer[carriage_index + 1] = '\0';
		terminal.write(read_buffer, carriage_index + 1);
		// Genode::log(carriage_index);
		// terminal.write("\n", 2);
	}

	Main(Env &env) : terminal(env),
	                 read_avail(env.ep(), *this, &Main::handle_read_avail)
	{
		terminal.read_avail_sigh(read_avail);
		// terminal.write(intro.string(), intro.length() + 1);
		// terminal.write("123456789\n", 10);
	}
};

void Component::construct(Env &env) { static Main main(env); }
