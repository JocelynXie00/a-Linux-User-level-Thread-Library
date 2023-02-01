#include <stdio.h>
#include <ucontext.h>
#include "thread.h"
#include <stdio.h>
#include <queue>
#include <stdlib.h>
#include "interrupt.h"
#include <iostream>
using namespace std;
static void producer();
static void consumer();
static int fridge = 0;
static int COMPACITY = 5;

static void
producer ()
{
    cout<<thread_lock(0)<<"\n";
    cout << "fridge from producer before:" << fridge<<"\n";
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
    cout<<thread_libinit((thread_startfunc_t)producer, (void *)0)<<"\n"; 
}


static void
consumer ()
{
    cout<<thread_lock(0);
    cout<<thread_yield();
    cout << "fridge from consumer before:" << fridge<<"\n";
    while (fridge == 0)
    {
        thread_wait(0, 2);
    }
    fridge -= 1;
    cout << "fridge from consumer after:" << fridge<<"\n";
    cout<<thread_signal(0, 1)<<"\n";
    cout<<thread_unlock(0)<<"\n";
}

static void
mainthread()
{
    thread_create((thread_startfunc_t)producer, 0);
    
    thread_create((thread_startfunc_t)producer, 0);

    thread_create((thread_startfunc_t)consumer, 0);
    thread_create((thread_startfunc_t)consumer, 0);
    thread_create((thread_startfunc_t)consumer, 0);
}
int
main (void)
{
    cout<<thread_libinit((thread_startfunc_t)mainthread, (void *)0)<<"\n";    
}
