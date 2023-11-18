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


// const int RPC_BUFFER_LEN = 4096 * 16; // 64KB
// const int MAINMEM_STORE_LEN = 4096 * 1024 * 8; // 32 MB
const int RPC_BUFFER_LEN = 64;
const int MAINMEM_STORE_LEN = 64;
const int CLIENT_NUM = 4;
const int STORE_LEN_INT = MAINMEM_STORE_LEN / sizeof(int) / CLIENT_NUM;
const int MAINMEM_STORE_BEGS[CLIENT_NUM] = {STORE_LEN_INT*0,
											STORE_LEN_INT*1,
											STORE_LEN_INT*2,
											STORE_LEN_INT*3};


namespace RPCplus {
	struct Session_component;
	struct Root_component;
	struct Main;
}


struct RPCplus::Session_component : Genode::Rpc_object<Session>
{
	//服务端创建一块ram dataspace，用于RPC消息
	Genode::Attached_ram_dataspace &rpc_buffer0;
	Genode::Attached_ram_dataspace &rpc_buffer1;
	Genode::Attached_ram_dataspace &rpc_buffer2;
	Genode::Attached_ram_dataspace &rpc_buffer3;
	Genode::Attached_ram_dataspace &storage;

	Session_component(Genode::Attached_ram_dataspace &main_store,
				Genode::Attached_ram_dataspace &rpc_buf0,
				Genode::Attached_ram_dataspace &rpc_buf1,
				Genode::Attached_ram_dataspace &rpc_buf2,
				Genode::Attached_ram_dataspace &rpc_buf3)
	:	rpc_buffer0(rpc_buf0),
		rpc_buffer1(rpc_buf1),
		rpc_buffer2(rpc_buf2),
		rpc_buffer3(rpc_buf3),
		storage(main_store)
	{	
		void* storage_ptr = storage.local_addr<int>();
		Genode::log("Server local storage addr: ", storage_ptr);
		int* p;
		p = rpc_buffer0.local_addr<int>();
		p[0] = 114514;
		p = rpc_buffer1.local_addr<int>();
		p[0] = 114514;
		p = rpc_buffer2.local_addr<int>();
		p[0] = 114514;
		p = rpc_buffer3.local_addr<int>();
		p[0] = 114514;
		Genode::log("RPC buffer init.");
	}

	void pure_function() override {
		return; 
	}

	int add_int(int a, int b) override {
		return (a + b); 
	}

	int set_storehead(int clientID, int a) override {
		int* buf;
		if (clientID == 0) buf = rpc_buffer0.local_addr<int>();
		if (clientID == 1) buf = rpc_buffer1.local_addr<int>();
		if (clientID == 2) buf = rpc_buffer2.local_addr<int>();
		if (clientID == 3) buf = rpc_buffer3.local_addr<int>();
		if (clientID < 0 || clientID > 3) return -1;
		int* storage_int = storage.local_addr<int>();
		storage_int = storage_int + MAINMEM_STORE_BEGS[clientID];
		a = a % STORE_LEN_INT;
		storage_int[a] = buf[0];
		return storage_int[a];
	}

	Genode::Ram_dataspace_capability get_RPCbuffer(int clientID) override {
		if (clientID == 0) return rpc_buffer0.cap();
		if (clientID == 1) return rpc_buffer1.cap();
		if (clientID == 2) return rpc_buffer2.cap();
		if (clientID == 3) return rpc_buffer3.cap();
		if (clientID < 0 || clientID > 3) Genode::log("get_RPCbuffer clientID ERR.");
		return rpc_buffer0.cap();
	}

	int get_storehead(int clientID, int a) override {
		int* buf;
		if (clientID == 0) buf = rpc_buffer0.local_addr<int>();
		if (clientID == 1) buf = rpc_buffer1.local_addr<int>();
		if (clientID == 2) buf = rpc_buffer2.local_addr<int>();
		if (clientID == 3) buf = rpc_buffer3.local_addr<int>();
		if (clientID < 0 || clientID > 3) return -1;
		int* storage_int = storage.local_addr<int>();
		storage_int = storage_int + MAINMEM_STORE_BEGS[clientID];
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
			return new (md_alloc()) Session_component(_main_store, _rpc_buffer0,
													_rpc_buffer1, _rpc_buffer2, _rpc_buffer3);
		}
	
	public:

		//handle the main memory dataspace
		Genode::Attached_ram_dataspace &_main_store;
		Genode::Attached_ram_dataspace &_rpc_buffer0;
		Genode::Attached_ram_dataspace &_rpc_buffer1;
		Genode::Attached_ram_dataspace &_rpc_buffer2;
		Genode::Attached_ram_dataspace &_rpc_buffer3;

		Root_component(Genode::Env &env, 
					Genode::Attached_ram_dataspace &main_store,
					Genode::Attached_ram_dataspace &rpc_buffer0,
					Genode::Attached_ram_dataspace &rpc_buffer1,
					Genode::Attached_ram_dataspace &rpc_buffer2,
					Genode::Attached_ram_dataspace &rpc_buffer3,
					Genode::Entrypoint &ep, Genode::Allocator &alloc)
		:	Genode::Root_component<Session_component>(ep, alloc), 
			_env(env), //这里把env传进去
			_main_store(main_store),
			_rpc_buffer0(rpc_buffer0),
			_rpc_buffer1(rpc_buffer1),
			_rpc_buffer2(rpc_buffer2),
			_rpc_buffer3(rpc_buffer3)
		{
			Genode::log("creating root component");
		}
};


struct RPCplus::Main
{
	Genode::Env &env;
	Timer::Connection &_timer;

	Genode::Attached_ram_dataspace mem_store{ env.ram(), env.rm(), MAINMEM_STORE_LEN };
	Genode::Attached_ram_dataspace rpc_buf0{ env.ram(), env.rm(), RPC_BUFFER_LEN };
	Genode::Attached_ram_dataspace rpc_buf1{ env.ram(), env.rm(), RPC_BUFFER_LEN };
	Genode::Attached_ram_dataspace rpc_buf2{ env.ram(), env.rm(), RPC_BUFFER_LEN };
	Genode::Attached_ram_dataspace rpc_buf3{ env.ram(), env.rm(), RPC_BUFFER_LEN };

	/*
	 * A sliced heap is used for allocating session objects - thereby we
	 * can release objects separately.
	 */
	Genode::Sliced_heap sliced_heap { env.ram(), env.rm() };
	RPCplus::Root_component root { env, mem_store, rpc_buf0, 
								rpc_buf1, rpc_buf2, rpc_buf3,
								env.ep(), sliced_heap };

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

	Genode::Heap heap{env.ram(), env.rm()};
	Genode::log("Time malloc test start: ", _timer.curr_time().trunc_to_plain_ms());

	void* buf[6000];
	int break_num = 0;

	for (int i = 0; i < 5000; ++i){
		buf[i] = heap.alloc(1<<12);
		// Genode::log("malloc result: ", i, "@", buf[i]);
		if (i > 0 && (genode_uint64_t)(buf[i]) - (genode_uint64_t)(buf[i-1]) != (1<<12)){
			// Genode::log("not continous at: ", i, "'th addr.");
			break_num += 1;
		}
	}
	Genode::log("ADDR break number: ", break_num);

	Genode::log("Time malloc test end: ", _timer.curr_time().trunc_to_plain_ms());

	for (int i = 0; i < 5000; ++i){
		heap.free(buf[i], 1<<12);
	}
}
