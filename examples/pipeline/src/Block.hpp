#ifndef BLOCK_HPP_
#define BLOCK_HPP_

#include <map>
#include <thread>
#include <aff3ct.hpp>
#include "Buffered_Socket.hpp"

class Block
{
public:
	std::string name = "";

	Block(aff3ct::module::Task* task, int buffered_socket_size);
	virtual ~Block();

	int bind     (std::string start_sck_name, Block &dest_block, std::string dest_sck_name);
	int bind_cpy (std::string start_sck_name, Block &dest_block, std::string dest_sck_name);
	void execute_task(bool const * isDone);
	void run(bool const * isDone) {this->th = std::thread{&Block::execute_task,this,isDone};};
	void join() {this->th.join();};
	void reset();
	Buffered_Socket<int8_t >* get_buffered_socket_int8_in   (std::string name);
	Buffered_Socket<int8_t >* get_buffered_socket_int8_out  (std::string name);
	Buffered_Socket<int16_t>* get_buffered_socket_int16_in  (std::string name);
	Buffered_Socket<int16_t>* get_buffered_socket_int16_out (std::string name);
	Buffered_Socket<int32_t>* get_buffered_socket_int32_in  (std::string name);
	Buffered_Socket<int32_t>* get_buffered_socket_int32_out (std::string name);
	Buffered_Socket<int64_t>* get_buffered_socket_int64_in  (std::string name);
	Buffered_Socket<int64_t>* get_buffered_socket_int64_out (std::string name);
	Buffered_Socket<float  >* get_buffered_socket_float_in  (std::string name);
	Buffered_Socket<float  >* get_buffered_socket_float_out (std::string name);
	Buffered_Socket<double >* get_buffered_socket_double_in (std::string name);
	Buffered_Socket<double >* get_buffered_socket_double_out(std::string name);	
	
protected:
	aff3ct::module::Task* task;
	unsigned int buffer_size;
	std::thread th;

	std::map<std::string, Buffered_Socket<int8_t >*> buffered_sockets_int8_in  ;
	std::map<std::string, Buffered_Socket<int16_t>*> buffered_sockets_int16_in ;
	std::map<std::string, Buffered_Socket<int32_t>*> buffered_sockets_int32_in ;
	std::map<std::string, Buffered_Socket<int64_t>*> buffered_sockets_int64_in ;
	std::map<std::string, Buffered_Socket<float  >*> buffered_sockets_float_in ;
	std::map<std::string, Buffered_Socket<double >*> buffered_sockets_double_in;

	std::map<std::string, Buffered_Socket<int8_t >*> buffered_sockets_int8_out  ;
	std::map<std::string, Buffered_Socket<int16_t>*> buffered_sockets_int16_out ;
	std::map<std::string, Buffered_Socket<int32_t>*> buffered_sockets_int32_out ;
	std::map<std::string, Buffered_Socket<int64_t>*> buffered_sockets_int64_out ;
	std::map<std::string, Buffered_Socket<float  >*> buffered_sockets_float_out ;
	std::map<std::string, Buffered_Socket<double >*> buffered_sockets_double_out;
};
#endif /* BLOCK_HPP_ */