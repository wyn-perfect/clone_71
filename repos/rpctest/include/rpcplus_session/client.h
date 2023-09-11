/*
 * \brief  Client-side interface of the RPC service
 * \author Björn Döbel
 * \author Minyi
 * \author Zhenlin
 * \date   2023-09
 */

/*
 * Copyright (C) 2008-2017 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

#ifndef _INCLUDE__RPCPLUS_SESSION_H__CLIENT_H_
#define _INCLUDE__RPCPLUS_SESSION_H__CLIENT_H_

#include <rpcplus_session/rpcplus_session.h>
#include <base/rpc_client.h>
#include <base/log.h>
#include <base/session_object.h>
#include <base/attached_ram_dataspace.h>
#include <dataspace/capability.h>
#include <base/rpc_server.h>
#include <dataspace/client.h>


namespace RPCplus { struct Session_client; }


struct RPCplus::Session_client : Genode::Rpc_client<Session>
{
	Session_client(Genode::Capability<Session> cap)
	: Genode::Rpc_client<Session>(cap) { }

	void say_hello() override
	{
		Genode::log("issue RPC for saying hello");
		call<Rpc_say_hello>();
		Genode::log("returned from 'say_hello' RPC call");
	}

	int add(int a, int b) override
	{
		return call<Rpc_add>(a, b);
	}

	Genode::Ram_dataspace_capability dataspace() override
	{
		return call<Rpc_dataspace>();
	}

	int send2server() override
	{
		return call<Rpc_send2server>();
	}

};

#endif /* _INCLUDE__RPCPLUS_SESSION_H__CLIENT_H_ */
