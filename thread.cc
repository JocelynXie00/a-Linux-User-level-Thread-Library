#include <stdio.h>
#include <ucontext.h>
#include <stdlib.h>
#include "thread.h"
#include "interrupt.h"
#include <queue>
#include <map>
#include <utility>
#include <iostream>

using namespace std;

queue<ucontext_t*> ready_queue;
ucontext_t *current_ctx_ptr;
ucontext_t scheduler_ctx;
class LockInfo
{
public:
	queue<ucontext_t*> *wait_queue;
	ucontext_t* holder;
	LockInfo() {
		holder = 0;
		wait_queue = new queue<ucontext_t*>;
	}
	
};
map<unsigned int, LockInfo*> lock_info;
map<pair<unsigned int, unsigned int>, queue<ucontext_t*>* > monitor_info;

int finish;
int has_init = 0;

/*
we need a start function to start "func"
because we need to free the stack and the context
*/
int 
thread_start(thread_startfunc_t func, void *arg)
{
	interrupt_enable();
	func(arg);
	interrupt_disable();
	finish = 1;
	return 0;
}

/*
create a thread remember in class:
alloc TCB (context)
alloc stack
bind stack and pc to TCB (...ss_sp = ...makecontext...)
*/
int 
thread_create(thread_startfunc_t func, void *arg)
{
	// ensure never use this function before libinit
	if (has_init == 0){
		return -1;
	}
	ucontext_t *ctx_ptr = (ucontext_t *)malloc(sizeof (ucontext_t));
	if (ctx_ptr == 0)
		return -1;
	getcontext(ctx_ptr);
	ctx_ptr->uc_stack.ss_sp = malloc(STACK_SIZE);
	if (ctx_ptr->uc_stack.ss_sp == 0)
		return -1;
    ctx_ptr->uc_stack.ss_size = STACK_SIZE;

    // when finish return back to scheduler
    ctx_ptr->uc_link = &scheduler_ctx;
    makecontext(ctx_ptr, (void (*)())thread_start, 2, func, arg);
    
    interrupt_disable();
    ready_queue.push(ctx_ptr);
    interrupt_enable();
    return 0;
}
// what to do? create a thread and make a scheduler thread
int 
thread_libinit(thread_startfunc_t func, void *arg)
{
	
	// ensure never init twice
	if (has_init == 1){
		return -1;
	}
	has_init = 1;
	if (thread_create(func, arg) == -1)
		return -1;
	interrupt_disable();
	while (!ready_queue.empty()) {
		current_ctx_ptr = ready_queue.front();
		ready_queue.pop();
		swapcontext(&scheduler_ctx, current_ctx_ptr);
		if (finish){
			free(current_ctx_ptr->uc_stack.ss_sp);
			free(current_ctx_ptr);
			finish = 0;
		}
	}
	/*
	cout << "lock wait "<< lock_info[0]->wait_queue->size() << '\n';
	for (int i = 0; i < 7; ++i)
	{

		cout << "cond"<<i<<"wait"<<monitor_info[pair<int,int>(0,i)]->size()<<'\n';
	}
	*/
	
	printf("Thread library exiting.\n");
	exit(0);
}



/**********
return control to scheduler
before that we need to add the current thread to ready queue

Question here: could call thread_yield in thread_yield?
maybe yes...
**********/
int 
thread_yield()
{
	// ensure never use this function before libinit
	if (has_init == 0){
		return -1;
	}
	interrupt_disable();
	// add current thread (context) to ready queue
	ready_queue.push(current_ctx_ptr);
	// switch to scheduler
	swapcontext(current_ctx_ptr, &scheduler_ctx);
	interrupt_enable();
	return 0;
}

int 
thread_lock(unsigned int lock)
{
	if (has_init == 0){
		return -1;
	}
	interrupt_disable();
	// if not exist create one
	if (lock_info.find(lock) == lock_info.end()) {
		LockInfo *info = new LockInfo;
		lock_info[lock] = info;
	}

	LockInfo *info = lock_info[lock];
	// if holder is current thread return -1 error
	if (current_ctx_ptr == info->holder) {
		interrupt_enable();
		return -1;
	}
	// no one holds
	if (info->holder == 0){
		info->holder = current_ctx_ptr;
	} else {
		// add this thread into waiting list
		info->wait_queue->push(current_ctx_ptr);
		// switch to a ready thread
		swapcontext(current_ctx_ptr, &scheduler_ctx);
	}
	interrupt_enable();
	return 0;
}

int
thread_unlock(unsigned int lock)
{
	if (has_init == 0){
		return -1;
	}
	interrupt_disable();
	// if not exist return -1
	if (lock_info.find(lock) == lock_info.end()) {
		interrupt_enable();
		return -1;
	}
	LockInfo *info = lock_info[lock];
	if (info->holder != current_ctx_ptr){
		interrupt_enable();
		return -1;
	}
	info->holder = 0;
	if (info->wait_queue->size() > 0) {
		ucontext_t *next_ready = info->wait_queue->front();
		info->wait_queue->pop();
		ready_queue.push(next_ready);
		info->holder = next_ready;
	}
	interrupt_enable();
	return 0;
}
// what does wait() do?
// unlock and sleep atomically
// but how?
int 
thread_wait(unsigned int lock, unsigned int cond)
{
	if (has_init == 0){
		return -1;
	}
	// not elegant..... unlock
	interrupt_disable();
	// if not exist return -1
	if (lock_info.find(lock) == lock_info.end()) {
		interrupt_enable();
		return -1;
	}
	LockInfo *info = lock_info[lock];
	if (info->holder != current_ctx_ptr){
		interrupt_enable();
		return -1;
	}
	info->holder = 0;
	if (info->wait_queue->size() > 0){
		ucontext_t *next_ready = info->wait_queue->front();
		info->wait_queue->pop();
		ready_queue.push(next_ready);
		info->holder = next_ready;
	}
	// sleep ... add this thread into monitor wait queue
	pair<unsigned int, unsigned int> p = pair<unsigned int, unsigned int>(lock, cond);
	if (monitor_info.find(p) == monitor_info.end()) {
		queue<ucontext_t*> *info = new queue<ucontext_t*>;
		monitor_info[p] = info;
	}
	monitor_info[p]->push(current_ctx_ptr);
	swapcontext(current_ctx_ptr, &scheduler_ctx);
	interrupt_enable();
	thread_lock(lock);
	return 0;
}

int thread_signal(unsigned int lock, unsigned int cond)
{
	if (has_init == 0){
		return -1;
	}
	interrupt_disable();
	pair<unsigned int, unsigned int> p = pair<unsigned int, unsigned int>(lock, cond);
	if (monitor_info.find(p) == monitor_info.end()) {
		interrupt_enable();
		return 0;
	}
	if (monitor_info[p]->size() > 0){
		ucontext_t *next_ready = monitor_info[p]->front();
		monitor_info[p]->pop();
		ready_queue.push(next_ready);
	}

	interrupt_enable();
	return 0;
}
int thread_broadcast(unsigned int lock, unsigned int cond)
{
	if (has_init == 0){
		return -1;
	}
	interrupt_disable();
	pair<unsigned int, unsigned int> p = pair<unsigned int, unsigned int>(lock, cond);
	if (monitor_info.find(p) == monitor_info.end()) {
		interrupt_enable();
		return 0;
	}
	while (monitor_info[p]->size() > 0){
		ucontext_t *next_ready = monitor_info[p]->front();
		monitor_info[p]->pop();
		ready_queue.push(next_ready);
	}

	interrupt_enable();
	return 0;
}
/*
int shared_value = 0;
void parent(void *arg)
{
	thread_lock(0);
	shared_value += 5;
	thread_yield();
	shared_value *= 8;
	thread_unlock(0);
}
void child(void *arg)
{
	puts("4");
	thread_lock(0);
	puts("5");
	shared_value +=5;
	thread_yield();
	puts("6");
	shared_value *= 8;
	thread_unlock(0);
	puts("7");
}


void
main_thread(void *arg)
{

	thread_create(parent, 0);
	thread_create(parent, 0);
	thread_create(parent, 0);
	thread_create(parent, 0);
	//printf("%d\n",ready_queue.size());
	printf("finish creating...\n");
}
int
main (void)
{
    thread_libinit(main_thread, 0);
    printf("%d\n", shared_value);
    printf("finish main\n");
    return 0;
}
*/