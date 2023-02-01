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
static int COMPACITY = 3;

static void
baduse0 ()
{
    thread_lock(0);
    while (fridge == COMPACITY)
    {
        thread_wait(0, 1);
    }
    fridge += 1;
    thread_signal(0, 2);
    thread_unlock(0);
}

static void
baduse1()//lock twice
{
    thread_lock(1);
    cout<<"lock twice:"<< thread_lock(1)<<"\n";
    while (fridge == COMPACITY)
    {
        thread_wait(0, 1);
    }
    fridge += 1;
    thread_signal(0, 2);
    thread_unlock(1);
}

static void
baduse2()//unlock without hold
{
    while (fridge == COMPACITY)
    {
        thread_wait(0, 1);
    }
    fridge += 1;
    thread_signal(0, 2);
    cout<<"unlock without hold:"<<thread_unlock(1)<<"\n";
    cout<<"unlock nonexsiting lock:"<<thread_unlock(9)<<"\n";
}
static void
baduse3()//wait without lock
{
    while (fridge == COMPACITY)
    {
        if (thread_wait(0,1) == -1)
        {
            puts("wait without lock:-1");
            thread_yield();
        }
    }
    fridge += 1;
    thread_signal(0, 2);
}
static void
baduse4()//wait with nonexisting lock
{
    
    while (fridge == COMPACITY)
    {
        if (thread_wait(10,1) == -1)
        {
            puts("wait with nonexsiting lock:-1");
            thread_yield();
        }
    }
    fridge += 1;
    thread_signal(0, 2);
}



static void
consumer ()
{
    thread_lock(0);
    while (fridge == 0)
    {
        thread_wait(0, 2);
    }
    fridge -= 1;
    thread_signal(0, 1);
    thread_unlock(0);
}

static void
mainthread()
{
    thread_create((thread_startfunc_t)baduse0, 0);
    thread_create((thread_startfunc_t)baduse1, 0);
    thread_create((thread_startfunc_t)baduse2, 0);
    thread_create((thread_startfunc_t)baduse3, 0);
    thread_create((thread_startfunc_t)baduse4, 0);
    thread_create((thread_startfunc_t)consumer, 0);
    thread_create((thread_startfunc_t)consumer, 0);
    thread_yield();
    thread_yield();
    thread_yield();
    thread_yield();
    thread_yield();
    thread_yield();
    thread_yield();
    thread_yield();
}
int
main (void)
{
    //create before libinit
    cout<<"create before libinit: "<<thread_create((thread_startfunc_t)baduse0, 0)<<"\n";
    //yield before libinit
    cout<<"yield before libinit: "<<thread_yield()<<"\n";
    //lock before libinit
    cout<<"lock before libinit: "<<thread_lock(1)<<"\n";
    cout<<"unlock before libinit: "<<thread_unlock(1)<<"\n";
    //wait signal broadcast before libinit
    cout<<"wait before libinit: "<<thread_wait(0,0)<<"\n";
    cout<<"signal before libinit: "<<thread_signal(0,0)<<"\n";
    cout<<"broadcast before libinit: "<<thread_broadcast(0,0)<<"\n";
    //try libinit
    thread_libinit((thread_startfunc_t)mainthread, (void *)0);    
}