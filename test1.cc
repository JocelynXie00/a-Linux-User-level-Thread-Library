#include <stdio.h>
#include <ucontext.h>
#include "thread.h"
#include <stdio.h>
#include <queue>
#include <stdlib.h>
#include "interrupt.h"
#include <iostream>
using namespace std;
static void baduse0();
static void consumer();
static int fridge = 0;
static int COMPACITY = 5;

static void
baduse0 ()//libinit more than once
{
    puts("baduse0");
    cout<<thread_lock(0)<<"\n";
    cout<<thread_yield()<<"\n";
    while (fridge == COMPACITY)
    {
    	cout<<thread_yield()<<"\n";
    	cout<<thread_wait(0, 1)<<"\n";
    }
    cout<<thread_yield()<<"\n";
    fridge += 1;
    cout << "fridge from producer after:" << fridge<<"\n";
    cout<<thread_signal(0, 2)<<"\n";
    cout<<thread_yield()<<"\n";
    cout<<thread_unlock(0)<<"\n";
}

static void
baduse1()//misuse of lock, lock twice, lock without hold
{
	puts("baduse1");
	cout<<thread_lock(1)<<"\n";
	cout<<thread_lock(1)<<"\n";
    cout<<thread_yield()<<"\n";
    while (fridge == COMPACITY)
    {
    	thread_yield();
    	thread_wait(0, 1);
    }
    cout<<thread_yield()<<"\n";
    fridge += 1;
    cout << "fridge from producer after:" << fridge<<"\n";
    cout<<thread_signal(0, 2)<<"\n";
    cout<<thread_yield()<<"\n";
    cout<<thread_unlock(0)<<"\n";
}
static void
baduse2()//wait without lock
{
    puts("baduse2");
    cout<<thread_yield()<<"\n";
    while (fridge == COMPACITY)
    {
    	cout<<thread_yield()<<"\n";
    	cout<<thread_wait(0, 1)<<"\n";
    }
    cout<<thread_yield()<<"\n";
    fridge += 1;
    cout << "fridge from producer after:" << fridge<<"\n";
    cout<<thread_signal(0, 2)<<"\n";
    cout<<thread_yield()<<"\n";
    cout<<thread_unlock(1)<<"\n";
}
static void
baduse3()//keep creating threads
{
	puts("baduse3");
	while(1)
	{
		if(thread_create((thread_startfunc_t)baduse0, 0) == -1)
		{
			puts("-1");
			break;
		}
		puts("0");
	}

}
static void
consumer ()
{
    puts("consumer");
    cout<<thread_lock(0)<<"\n";
    cout<<thread_yield()<<"\n";
    while (fridge == 0)
    {
    	cout<<thread_wait(0, 2)<<"\n";
    }
    fridge -= 1;
    cout << "fridge from consumer after:" << fridge<<"\n";
    cout<<thread_signal(0, 1)<<"\n";
    cout<<thread_unlock(0)<<"\n";
}

static void
mainthread()
{
	thread_create((thread_startfunc_t)baduse0, 0);
	thread_create((thread_startfunc_t)baduse1, 0);
	
	thread_create((thread_startfunc_t)baduse0, 0);

	thread_create((thread_startfunc_t)consumer, 0);
	thread_create((thread_startfunc_t)consumer, 0);
	thread_create((thread_startfunc_t)baduse1, 0);
	thread_create((thread_startfunc_t)consumer, 0);
}
int
main (void)
{
    thread_libinit((thread_startfunc_t)mainthread, (void *)0);    
}