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
#include <base/heap.h>
#include <vector>



const int PING_PONG_NUM = 1<<1;
const int READ_BUF_SIZE = (1 << 16);

// We keep WRITE_QUEUE_DEPTH x WRITE_BUFFER_SIZE = READ_BUF_SIZE 写队列的长度*写缓冲的大小 = 读缓冲的大小
const int WRITE_BUFFER_SIZE = 1<<16; //每个报文的长度
const int WRITE_QUEUE_DEPTH = (READ_BUF_SIZE / WRITE_BUFFER_SIZE);

using namespace Genode;

static inline int wq_pos(int64_t id) {

	return (int)(id % WRITE_QUEUE_DEPTH) * WRITE_BUFFER_SIZE;}

struct Main
{	
	Genode::Env &env;
	Terminal::Connection terminal;
	Signal_handler<Main> read_avail;
	char read_buffer[READ_BUF_SIZE];//读缓冲区的大小
	char write_buffer[WRITE_QUEUE_DEPTH * WRITE_BUFFER_SIZE];//写缓冲区的大小

	float time_slot[PING_PONG_NUM];// 客户端从开始到读完数据总共的时间开销
	float time_write[PING_PONG_NUM];// 客户端写数据的时间
	float time_response[PING_PONG_NUM];//客户端等待响应的时间
	float time_read[PING_PONG_NUM];//客户端读数据的时间

	float time_sever_rw[PING_PONG_NUM];//服务器端读写的时间
	float time_c2s[PING_PONG_NUM];//客户端到服务端端时间
	float time_s2c[PING_PONG_NUM];//服务端到客户端端时间


	//可能会用到的几个时间戳：
	
	float offset = 0;
	float client_write_start=0;//客户端开始读时间戳
	float client_write_end=0;//客户端结束读时间戳

	float client_read_start=0;//客户端开始读时间戳
	float client_read_end=0;//客户端结束读时间戳

	float server_read=0;//服务器端读时间
	float server_write=0;//服务器端写时间戳

	float sever_read_write=0;
	int index=0;
	
	int64_t writeID_head = 0;//   缓冲区中正在读的request id
	int64_t writeID_tail = 0;//  缓冲区中写入的request id
	// int pingpong_time = PING_PONG_NUM;
	Timer::Connection _timer;
	// To verify Tx and Rx are one-to-one, we count them
	int64_t info_cnt_sum = 0;//每个请求的id

	String<128> intro {
"--- Terminal echo test started, now you can type characters to be echoed. ---\n" };

	void handle_read_avail()
	{
		client_read_start = (float)_timer.curr_time().trunc_to_plain_us().value;
		size_t num_bytes = terminal.read(read_buffer, READ_BUF_SIZE);//将io缓冲区中的数据写到
		//log("4 got ", num_bytes, " byte(s)");//从server端发来的数据有多少
		//Genode::log("index = ",index);
		// if(num_bytes!=0){
		log("got ", num_bytes, " byte(s)");
		// }
		int len = (int)num_bytes;
		size_t info_cnt = 0;
		unsigned check[3] ={0};

		if(num_bytes!=0&&read_buffer[len-1]=='\0'){
			int index = len-2;
			for(int i =2;i>=0;i--){
				for(int j = 0;j<4;j++){
					check[i]<<=8;
					check[i] += unsigned(read_buffer[index])&0x000000ff;
					index--;
				}
			}
			info_cnt++;
		}
		int t0= check[0];
		int t2 = check[1];
		int dif = check[2];
		//Genode::log("t0 =",t0,"t2 =",t2,"dif = ",dif);
		server_read = (float)t0;
		server_write = (float)t2;
		sever_read_write =(float)dif;
		
		writeID_tail += info_cnt;
		info_cnt_sum += info_cnt;
		Genode::log("---------");
		return;
	}

	size_t Send2Server(const char* info, size_t size){

		client_write_start = (float)_timer.curr_time().trunc_to_plain_us().value;//计算客户端开始写的时间

		Genode::log("1---------客户端准备发送数据 -------");
		Genode::memcpy(write_buffer + wq_pos(writeID_tail), info, size);
		terminal.write(write_buffer + wq_pos(writeID_tail), size);//把writebuffer中的数据 写入到io缓冲区zz
		
		client_write_end = (float)_timer.curr_time().trunc_to_plain_us().value;
		
		float time_client_write = client_write_end-client_write_start;
		time_write[index] = time_client_write/1000;
		
		Genode::log("------------------2 -------");
		//-----------------------------------监听---------------------------------

		while (writeID_tail==WRITE_QUEUE_DEPTH-1){//当前writer缓冲区刚好有一个数据
			//Write Queue Pending...
			//Genode::log("ddddd");
			//Genode::log("-------循环中 -------");
			handle_read_avail();
		}
		

		client_read_end = (float)_timer.curr_time().trunc_to_plain_us().value;
	 
	 	//Genode::log("c2 = ",(client_read_end-tmp1)/(float)1000);

		time_read[index] = (float)((float)client_read_end-client_read_start)/(float)1000;
		//Genode::log("客户端开始du的时间",client_read_start);
		time_response[index] = (client_read_start-client_write_end)/(float)1000;
		
		
		offset = (  (server_read-client_write_start)+(server_write - client_read_start)      )/(float)2;
		
		
		
		// Genode::log("offset = ",offset);
		// //对齐下时间

		// Genode::log("server_read=",server_read);
		// Genode::log("server_write=",server_write);
		// Genode::log("cha = ",sever_read_write);
		// Genode::log("-------------------------");	
		server_read -= offset;
		server_write-=offset;

		// Genode::log("server_read=",server_read);
		// Genode::log("server_write=",server_write);
		// Genode::log("client_write_start=",client_write_start);
		// Genode::log("client_read_start=",client_read_start);
		time_c2s[index] = (server_read-client_write_start)/(float)1000;
		//Genode::log("c2s = ",time_c2s[index]);
		
		time_s2c[index] = float(client_read_start-server_write)/(float)1000;
		//Genode::log("s2c = ",time_s2c[index]);
		//Genode::log("客户端开始du的时间",client_read_start,"服务器写的时间",server_write_end);
		//Genode::log(time_s2c[index]);
		//Genode::log("客户端读完的时间戳",client_read_end);
		time_sever_rw[index] = sever_read_write/(float)1000;
		writeID_tail  = 0;
		index++;
		return size;
	}

	Main(Env &env)
				: env(env),
				terminal(env),
	            read_avail(env.ep(), *this, &Main::handle_read_avail),
				_timer(env)
	{	
		// Timer::Connection _timer(env);

		terminal.read_avail_sigh(read_avail);
	
	}
};

void Component::construct(Env &env) {
	String<511> str512B {
"111111111111111111111111111111111111111111111111111111111111111\
1111111111111111111111111111111111111111111111111111111111111111\
1111111111111111111111111111111111111111111111111111111111111111\
1111111111111111111111111111111111111111111111111111111111111111\
1111111111111111111111111111111111111111111111111111111111111111\
1111111111111111111111111111111111111111111111111111111111111111\
1111111111111111111111111111111111111111111111111111111111111111\
111111111111111111111111111111111111111111111111111111111111111" };
	String<255> str256B {
"111111111111111111111111111111111111111111111111111111111111111\
1111111111111111111111111111111111111111111111111111111111111111\
1111111111111111111111111111111111111111111111111111111111111111\
111111111111111111111111111111111111111111111111111111111111111" };
	String<255> str128B {
"111111111111111111111111111111111111111111111111111111111111111\
111111111111111111111111111111111111111111111111111111111111111" };
	String<255> str64B {
"11111111111111111111111111111111111111111111111111111111111111" };
	int len=100;
	char s[len];
	for(int i = 0;i<len;i++){
		s[i] = '1';
	}
	s[len-1] = '\0';
	// Genode::Heap heap{env.ram(), env.rm()}; //创建一个heap对象
	static Main main(env); //创建初始化main对象
	
	Timer::Connection _timer(env);
	//Genode::log("t1 = ", main.t1);
	Genode::Microseconds time_beg = _timer.curr_time().trunc_to_plain_us();

	Genode::log("Client test start at ", time_beg);
	// main.Send2Server("1",2);
	// main.Send2Server("12",3);
	// main.Send2Server("123",4);
	// main.Send2Server("1234",5);
	// main.Send2Server("12345",6);
	// main.Send2Server("123456",7);
	// main.Send2Server("12345678",9);
	// main.Send2Server("123456789",10);
	// main.Send2Server("12345678910",12);
	// main.Send2Server("12345678910",12);
	// main.Send2Server("12345678911",12);
	// main.Send2Server("12345678912",12);
	// main.Send2Server("12345678913",12);
	// main.Send2Server("12345678914",12);
	// main.Send2Server("12345678915",12);
	// main.Send2Server("12345678916",12);


	for (int i = 0; i < PING_PONG_NUM; ++i){

		//Genode::Microseconds time_send2server_start = _timer.curr_time().trunc_to_plain_us();//
		Genode::Microseconds time_send2sever_start = _timer.curr_time().trunc_to_plain_us();//客户端将数据准备发送给服务器端的时间
		//main.Send2Server(str512B.string(), str512B.length());
		main.Send2Server(s, len);
		Genode::Microseconds time_send2server_end = _timer.curr_time().trunc_to_plain_us();//客户端接收到服务器端发来数据的时间
		main.time_slot[i] = (float)(time_send2server_end.value-time_send2sever_start.value)/(float)1000;
		Genode::log("total  = " ,main.time_slot[i]);
	}
	
	// these lines drain the RX packets, maybe added to member function 
	Genode::log("Tx cnt: ", PING_PONG_NUM, " Rx cnt: ", main.info_cnt_sum);

	while (PING_PONG_NUM > main.info_cnt_sum){
		main.handle_read_avail();
		// Genode::log("Tx cnt: ", PING_PONG_NUM, " Rx cnt: ", main.info_cnt_sum);
 	}
	main.handle_read_avail();
	Genode::log("Tx cnt: ", PING_PONG_NUM, " Rx cnt: ", main.info_cnt_sum);
	Genode::Microseconds time_end = _timer.curr_time().trunc_to_plain_us();
	Genode::log("Client test finish at ", time_end);

	float sum_total_time = 0;
	float sum_client_write = 0;
	float sum_client_read = 0;
	float sum_client_response = 0;
	float sum_server_read = 0;
	float sum_transport = 0;
	float sum_c2s = 0;
	float sum_s2c = 0;
	float min =main.time_slot[0];
	float max =main.time_slot[0];

	for(int i = 0;i<PING_PONG_NUM;i++){
		if(main.time_slot[i]<min){
			min = main.time_slot[i];
		}
		if(main.time_slot[i]>max){
			max = main.time_slot[i];
		}
		sum_total_time+=main.time_slot[i];
		sum_client_write+=main.time_write[i];
		sum_client_read+=main.time_read[i];
		sum_client_response+=main.time_response[i];
		sum_server_read+=main.time_sever_rw[i];
		sum_c2s+=main.time_c2s[i];
		sum_s2c+=main.time_s2c[i];
		sum_transport+=(main.time_slot[i]-main.time_write[i]-main.time_read[i]-main.time_sever_rw[i])/2;
		//Genode::log(main.time_slot[i]," |",main.time_write[i],"|",main.time_read[i],"|",main.time_slot[i]-main.time_write[i]-main.time_read[i],"|",main.time_response[i],"|",main.time_sever_rw[i]);
	}
	sum_total_time = sum_total_time - min -max;
	float thput = (float)(time_end.value - time_beg.value)/1000;
	thput = (float)PING_PONG_NUM / thput * (float)1000;
	Genode::log("TCP send/recv rate is ", thput, " op/s.");

	Genode::log(" size =  ", len);
	Genode::log(" average total time =  ", sum_total_time/PING_PONG_NUM);
	Genode::log(" max time =  ", max);
	Genode::log(" min time =  ", min);


	Genode::log(" average client write =  ", sum_client_write/PING_PONG_NUM);
	Genode::log(" average client read =  ", sum_client_read/PING_PONG_NUM);
	Genode::log(" average client_response =  ", sum_client_response/PING_PONG_NUM);
	Genode::log(" average server_read =  ", sum_server_read/PING_PONG_NUM);
	Genode::log(" average transport =  ", sum_transport/PING_PONG_NUM);
	Genode::log(" average   sum_c2s ", sum_c2s/PING_PONG_NUM);
	Genode::log(" average sum_s2c =  ", sum_s2c/PING_PONG_NUM);
}
