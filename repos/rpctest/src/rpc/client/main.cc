/*
 * \brief  Test client for our RPC interface
 * \author Björn Döbel
 * \author Norman Feske
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

#include <base/component.h>
#include <base/log.h>
#include <rpcplus_session/connection.h>
#include <base/attached_ram_dataspace.h>


void Component::construct(Genode::Env &env)
{
	RPCplus::Connection rpc(env);

	rpc.say_hello();

	int const sum = rpc.add(2, 5);
	Genode::log("added 2 + 5 = ", sum);

	//获取服务端dataspace
	Genode::Ram_dataspace_capability dscap = rpc.dataspace();
	Genode::log("cap in client is ", dscap);
	//指定dataspace的虚拟内存地址q
	//Now a fixed high addr, free of conflict
	Genode::addr_t q = 0x60000000;
	//绑定
	env.rm().attach_at(dscap, q);
	//addr_t没法直接用，类型转换一下
	int* qq = (int*) q;
	//qq对应的物理地址就是服务端dataspace的地址
	Genode::log("get dataspace head ", qq[0]);
	//Write some reply to the server
	qq[0] = 19;
	qq[19] = 1919810;
	rpc.send2server();

	Genode::log("rpc test completed");
}
