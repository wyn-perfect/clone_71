
#include <base/allocator_avl.h>
#include <base/attached_rom_dataspace.h>
#include <base/component.h>
#include <base/heap.h>
#include <base/log.h>
#include <block_session/connection.h>
#include <os/reporter.h>
#include <timer_session/connection.h>


namespace Block_demo {

	using namespace Genode;
	struct Test;
	struct Main;

	struct Test_failed              : Exception { };
	struct Constructing_test_failed : Exception { };
}


struct Block_demo::Test
{
	protected:

		Env       &_env;
		Allocator &_alloc;


		typedef Block::block_number_t block_number_t;


		Constructible<Timer::Connection> _timer { };


		Allocator_avl _block_alloc { &_alloc };

		struct Job;
		typedef Block::Connection<Job> Block_connection;

		Constructible<Block_connection> _block { };

		struct Job : Block_connection::Job
		{
			unsigned const id;

			Job(Block_connection &connection, Block::Operation operation, unsigned id)
			:
				Block_connection::Job(connection, operation), id(id)
			{ }
		};



		Block::Session::Info _info { };

		uint64_t _start_time { 0 };
		size_t   _bytes      { 0 };
		uint64_t _rx         { 0 };
		uint64_t _tx         { 0 };
		size_t   _triggered  { 0 };  /* number of I/O signals */
		unsigned _completed  { 0 };

		bool _stop_on_error { true };
		bool _finished      { false };
		bool _success       { false };

		uint64_t begin { 0 };
		uint64_t end { 0 };


		size_t job_cnt { 0 };

		size_t loop { 0 };

		size_t loop_cnt { 0 };

		char* _read_buffer;
		char* _write_buffer;


	public:

		/**
		 * Block::Connection::Update_jobs_policy
		 */
		void produce_write_content(Job &job, Block::seek_off_t offset, char *dst, size_t length)
		{
			_tx    += length / _info.block_size;
			_bytes += length;

			log("job ", job.id, ": writing ", length, " bytes at ", offset);
			memcpy(dst, _write_buffer, length);
		}

		/**
		 * Block::Connection::Update_jobs_policy
		 */
		void consume_read_result(Job &job, Block::seek_off_t offset,
		                         char const *src, size_t length)
		{
			_rx    += length / _info.block_size;
			_bytes += length;

			log("job ", job.id, ": got ", length, " bytes at ", offset);
			memcpy(_read_buffer, src, length);
			log(job.id, ": ",_read_buffer[0], " ", _read_buffer[1]);
		}

		/**
		 * Block_connection::Update_jobs_policy
		 */
		void completed(Job &job, bool success)
		{
			if (_completed == 0)
			{
				begin = _timer->curr_time().trunc_to_plain_ms().value;
			}
			
			_completed++;
			log("job ", job.id, ": ", job.operation(), ", completed");
			if (!success)
				error("processing ", job.operation(), " failed");
			destroy(_alloc, &job);
			if (_completed == loop)
			{
				_completed = 0;
				end = _timer->curr_time().trunc_to_plain_ms().value;
				log("Process time: " ,end - begin, "ms", "\nthroughput: ", 1000*loop/(end - begin), " write/s");
			}
			

		}

	protected:

		void _handle_progress_timeout(Duration)
		{
			log("progress: rx:", _rx, " tx:", _tx);
		}

		void _handle_block_io()
		{
			_triggered++;
			_block->update_jobs(*this);
		}

		Signal_handler<Test> _block_io_sigh {
			_env.ep(), *this, &Test::_handle_block_io };

	public:
		Test(Env &env, Allocator &alloc, char* read_buffer, char* write_buffer)
		:
			_env(env), _alloc(alloc), _read_buffer(read_buffer), _write_buffer(write_buffer)
		{
		}


		void _spawn_job(Block::block_number_t block_number, Block::block_count_t block_count, Block::Operation::Type op_type){
			Block::Operation op{.type = op_type, //read or write
								   .block_number = block_number, //开始操作的块的号
								   .count = block_count}; //操作的块的数量
			new (_alloc) Job(*_block, op, job_cnt++);

		}

		void start()
		{
			_timer.construct(_env);
			_block.construct(_env, &_block_alloc);
			_block->sigh(_block_io_sigh);
			_info = _block->info();
			log("block size is ", _info.block_size);
			loop = 100;
			for (int i = 0; i < loop; i++)
			{
				_write_buffer[0] = i;
				_spawn_job(1, 1, Block::Operation::Type::WRITE);
				_handle_block_io();
			}

			// for (int i = 0; i < loop; i++)
			// {
			// 	_spawn_job(i, 1, Block::Operation::Type::READ);
			// 	_handle_block_io();
			// }
			
			_start_time = _timer->elapsed_ms();

		}

};


/* tests */



/*
 * Main
 */
struct Block_demo::Main
{
	Genode::Env  &_env;
	Genode::Heap  _heap { _env.ram(), _env.rm() };


	Main(Genode::Env &env) : _env(env)
	{
		char* read_buffer = (char*)_heap.alloc(4*1024*1024);
		char* write_buffer = (char*)_heap.alloc(4*1024*1024);
		for (int i = 0; i < 1024; i++)
		{
			write_buffer[i] = '1';
		}
		
		Test *t = new (&_heap) Test(_env, _heap, read_buffer, write_buffer);
		log("test start");
		t->start();
	}


};


void Component::construct(Genode::Env &env)
{
	static Block_demo::Main main(env);
}
