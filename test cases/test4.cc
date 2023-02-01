#include "thread.h"
#include <stdio.h>
#include <queue>
#include <stdlib.h>
#include <iostream>
#include "interrupt.h"
#include <string>
#include <fstream>

using namespace std;


string disk0[3] = {"510", "671", "0"};
string disk1[3] = {"644", "682", "668"};
string disk2[3] = {"687", "697", "606"};
string disk3[3] = {"677", "570", "828"};
string disk4[3] = {"622", "688", "229"};
string *file_list[5] = {disk0,disk1,disk2,disk3,disk4};
struct queue_item
{
	unsigned int id;
	int num;
};
class Sstf_queue
{
	vector<struct queue_item> queue;
	int last_pop;

public:
	void push(struct queue_item item){
		queue.push_back(item);
	}
	struct queue_item pop(){
		int min_distance = 1000;
		int candidate;
		struct queue_item item;
		for (int i = 0; i < queue.size(); i++){
			if (abs(queue[i].num - last_pop) < min_distance){
				candidate = i;
				min_distance = abs(queue[i].num - last_pop);
			}
		}
		item = queue[candidate];
		last_pop = item.num;
		queue.erase(queue.begin()+candidate);
		return item;
	}
	int size(){
		return queue.size();
	}
	
	Sstf_queue(){
		last_pop = 0;
	}
	
}request_queue;


//queue<struct queue_item> request_queue;
int capacity;
unsigned int num_requester_alive;
unsigned int queue_full_cond;
unsigned int serve_cond;

// request thread:
// wait until an issue is served
// wait unitl the queue is not full
static void
request_thread(void *xa)
{
	
	unsigned int id  = (unsigned int)(intptr_t)xa;
	// ifstream input_file(file_list[id]);
	string *file_name_list = file_list[id];
	for (int i = 0; i < 3; i++) {
		struct queue_item item = {id, atoi(file_name_list[i].c_str())};
		thread_lock(0);
		while (request_queue.size() == capacity) {
			thread_wait(0, queue_full_cond);
		}
		request_queue.push(item);
		cout << "requester " << id << " track " << atoi(file_name_list[i].c_str()) << endl;
		thread_signal(0, serve_cond);
		thread_wait(0, id);
		thread_unlock(0);
	}

	thread_lock(0);
	num_requester_alive -= 1;
	thread_signal(0, serve_cond);
	thread_unlock(0);

}

// serve when queue is as full as possible
// as full as possible
//   request_queue is full or
//   num running_request_threads == request_queue.size()
// queue.size() == num_running_request_threads
// when to terminate?
// queue.size() == num_running_request_threads == 0
static void
serve_thread(void *xa)
{
	while(1)
	{
		thread_lock(0);
		while (request_queue.size()!= capacity and request_queue.size() != num_requester_alive) {
			thread_wait(0, serve_cond);
		}
		if (num_requester_alive == 0){
			thread_unlock(0);
			return;
		}
		struct queue_item item = request_queue.pop();
		
		cout << "service requester " << item.id << " track " << item.num << endl;
		
		thread_signal(0, item.id);
		thread_signal(0, queue_full_cond);
		thread_unlock(0);
	}
}
static void
main_thread(void *xa)
{
	int num_requester = (int)(intptr_t) xa;
	// must set num_requester_alive here before any thread cause sub thread would change/use it
	num_requester_alive = num_requester;
	// set 2 cond variables the integer right after num_requester
	queue_full_cond = num_requester;
	serve_cond = num_requester + 1;
	// create requester threads
	for (int i = 0; i < num_requester; i++) {
		// pass thread id
		thread_create(request_thread, (void *)(intptr_t)i);
	}
	// create server thread
	thread_create(serve_thread, 0);
	cout << "hello" <<'\n';
}

// entrance of program but won't return...
int
main(int argc, char const *argv[])
{
	capacity = 3;
	// pass #files(#requesters to main thread)
	thread_libinit(main_thread, (void *)(intptr_t) 5);
	return 0;
}



/*
main_thread(argv):
	num_requester_alive = argv[0]
	for i in range(argv[0]):
		create_thread(requester_thread, i)
	create_thread(server_thread, 0)

requester_thread(id,request_file):
	for request in request_file:
		lock(BIGLOCK)
		while request_queue.size() == capcity:
			wait(BIGLOCK, COND_QUEUE_FULL)
		push_queue([request, id])
		signal(BIGLOCK,COND_SERVE_WAIT)
		wait(BIGLOCK, id)
		unlock(BIGLOCK)
	lock(BIGLOCK)
	num_requester_alive -= 1
	signal(BIGLOCK,COND_SERVE_WAIT)
	unlock(BIGLOCK)

server_thread():
	while TRUE:
		lock(BIGLOCK)
		while request_queue.size()!= capacity and request_queue.size() != num_requester_alive:
			wait(BIGLOCK, COND_SERVE_WAIT)
		if RUNNING THREAD == 0:
			unlock(BIGLOCK)
			return 0
		item, requester_id = pop_queue()
		signal(BIGLOCK, requester_id)
		signal(BIGLOCK, COND_QUEUE_FULL)
		unclock(BIGLOCK)

*/