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
#include <timer_session/connection.h>

using namespace Genode;

const int READ_BUF_SIZE = (1 << 16);//表示服务器端读缓冲区的大小

//定义main结构
struct Main
{
	Terminal::Connection terminal; //包含了终端连接对象 terminal
	Signal_handler<Main> read_avail;//创建一个信号处理对象
	Timer::Connection _timer;
	char read_buffer[READ_BUF_SIZE];// 服务器端的一个读缓冲区的大小

	int t0 = 0;
	int dif = 0;
	size_t rec_bytes = 0;
	int m = 0;

	String<128> intro {
"--- Terminal echo test started, now you can type characters to be echoed. ---\n" };

	//用于于处理读取可用信号的回调函数 handle_read_avail
	void handle_read_avail()
	{	
		Genode::Microseconds server_read_start = _timer.curr_time().trunc_to_plain_us();
		int t1 = (int)server_read_start.value;
		if(t0==0){
			t0=t1;
		}
		size_t num_bytes = terminal.read(read_buffer+rec_bytes, READ_BUF_SIZE-rec_bytes);
		//size_t num_bytes = terminal.read(read_buffer, READ_BUF_SIZE);
		rec_bytes+=num_bytes;
		log("got ", num_bytes, " byte(s)");

		
		if (read_buffer[rec_bytes-1] == '\0') {//找到某个请求包的结束位置
			
			Genode::Microseconds server_write_start = _timer.curr_time().trunc_to_plain_us();
			int t2 =(int)server_write_start.value;
			t1=t0;
			t0=0;
			dif  = t2 - t1;
			
			int check[3] = {t1,t2,dif};
			for(int i = 0;i<3;i++){
				for(int j =0 ;j<4;j++){
					read_buffer[rec_bytes]=char(check[i]);
					check[i]>>=8;
					rec_bytes+=1;
				}
			}
			read_buffer[rec_bytes]='\0';
			terminal.write(read_buffer, rec_bytes+1);
			rec_bytes = 0;
		}
	}
	Main(Env &env) : terminal(env),
	                 read_avail(env.ep(), *this, &Main::handle_read_avail),
					 _timer(env)
	{
		terminal.read_avail_sigh(read_avail);
	}
};

void Component::construct(Env &env) 
{ 
	static Main main(env); 
	
}