/*
 * \brief  Main program of the RPC server
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

#include <base/component.h>
#include <base/log.h>
#include <base/heap.h>
#include <root/component.h>
#include <rpcplus_session/rpcplus_session.h>
#include <base/rpc_server.h>
#include <base/attached_ram_dataspace.h>
#include <base/session_object.h>
#include <dataspace/capability.h>
#include <dataspace/client.h>
#include <timer_session/connection.h>


const int RPC_BUFFER_LEN = 4096 * 16; // 64KB
const int MAINMEM_STORE_LEN = 4096 * 1024 * 2; // 8 MB
const int STORE_LEN_INT = MAINMEM_STORE_LEN / sizeof(int);


namespace RPCplus {
	struct Session_component;
	struct Root_component;
	struct Main;
}


struct RPCplus::Session_component : Genode::Rpc_object<Session>
{
	//服务端创建一块ram dataspace，用于RPC消息
	Genode::Attached_ram_dataspace rpc_buffer;
	Genode::Attached_ram_dataspace &storage;

	Session_component(Genode::Env &env, Genode::Attached_ram_dataspace &main_store)
	:	rpc_buffer(env.ram(), env.rm(), RPC_BUFFER_LEN),
		storage(main_store)
	{	
		void* storage_ptr = storage.local_addr<int>();
		Genode::log("Server local storage addr: ", storage_ptr);
		int* p = rpc_buffer.local_addr<int>();
		p[0] = 114514;
		Genode::log("RPC buffer init.");
	}

	void pure_function() override {
		return; 
	}

	int add_int(int a, int b) override {
		return (a + b); 
	}

	int set_storehead(int a) override {
		int* buf = rpc_buffer.local_addr<int>();
		int* storage_int = storage.local_addr<int>();
		a = a % STORE_LEN_INT;
		storage_int[a] = buf[0];
		return storage_int[a];
	}

	Genode::Ram_dataspace_capability get_RPCbuffer() override {
		return rpc_buffer.cap();
	}

	int get_storehead(int a) override {
		int* buf = rpc_buffer.local_addr<int>();
		int* storage_int = storage.local_addr<int>();
		a = a % STORE_LEN_INT;
		buf[0] = storage_int[a];
		return 0;
	}

};


class RPCplus::Root_component
:
	public Genode::Root_component<Session_component>
	
{
	private:
		//加个env
		Genode::Env &_env;
	
	protected:

		Session_component *_create_session(const char *) override
		{
			Genode::log("creating rpcplus session");
			//把env作为参数传给Session_component，Session_component才能创建Attached_ram_dataspace
			//we also add the main memory store space to the session
			return new (md_alloc()) Session_component(_env, _main_store);
		}
	
	public:

		//handle the main memory dataspace
		Genode::Attached_ram_dataspace &_main_store;

		Root_component(Genode::Env &env, Genode::Attached_ram_dataspace &main_store,
					Genode::Entrypoint &ep, Genode::Allocator &alloc)
		:	Genode::Root_component<Session_component>(ep, alloc), 
			_env(env), //这里把env传进去
			_main_store(main_store)
		{
			Genode::log("creating root component");
		}
};


struct RPCplus::Main
{
	Genode::Env &env;
	Timer::Connection &_timer;

	Genode::Attached_ram_dataspace mem_store{ env.ram(), env.rm(), MAINMEM_STORE_LEN };

	/*
	 * A sliced heap is used for allocating session objects - thereby we
	 * can release objects separately.
	 */
	Genode::Sliced_heap sliced_heap { env.ram(), env.rm() };
	RPCplus::Root_component root { env, mem_store, env.ep(), sliced_heap };

	Main(Genode::Env &env, Timer::Connection &timer) 
	:	env(env), _timer(timer)
	{
		Genode::log("Time mem allocation ", _timer.curr_time().trunc_to_plain_ms());
		/*
		 * Create a RPC object capability for the root interface and
		 * announce the service to our parent.
		 */
		env.parent().announce(env.ep().manage(root));
	}
};


void Component::construct(Genode::Env &env)
{	
	Timer::Connection _timer(env);
	static RPCplus::Main main(env, _timer);
}
