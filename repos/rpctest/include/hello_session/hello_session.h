/*
 * \brief  Interface definition of the Hello service
 * \author Björn Döbel
 * \date   2008-03-20
 */

/*
 * Copyright (C) 2008-2017 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

#ifndef _INCLUDE__HELLO_SESSION__HELLO_SESSION_H_
#define _INCLUDE__HELLO_SESSION__HELLO_SESSION_H_

#include <session/session.h>
#include <base/rpc.h>
#include <base/attached_ram_dataspace.h>
#include <base/capability.h>
#include <base/rpc_server.h>
#include <base/attached_ram_dataspace.h>
#include <base/session_object.h>
#include <dataspace/capability.h>
#include <base/rpc_server.h>
#include <dataspace/client.h>

namespace Hello { struct Session; }


struct Hello::Session : Genode::Session
{
	static const char *service_name() { return "Hello"; }

	enum { CAP_QUOTA = 4 };

	virtual void say_hello() = 0;
	virtual int add(int a, int b) = 0;
	virtual Genode::Ram_dataspace_capability dataspace() = 0;

	/*******************
	 ** RPC interface **
	 *******************/

	GENODE_RPC(Rpc_say_hello, void, say_hello);
	GENODE_RPC(Rpc_add, int, add, int, int);
	GENODE_RPC(Rpc_dataspace, Genode::Ram_dataspace_capability, dataspace);	
	GENODE_RPC_INTERFACE(Rpc_say_hello, Rpc_add, Rpc_dataspace);
};

#endif /* _INCLUDE__HELLO_SESSION__HELLO_SESSION_H_ */
