#include <stdio.h>
#include <ucontext.h>
#include "thread.h"
#include <stdio.h>
#include <queue>
#include <stdlib.h>
#include "interrupt.h"
#include <iostream>
using namespace std;
static int num_writer = 0;
static int num_reader = 0;

static void
readerStart()
{
    thread_lock(0);
    while (num_writer > 0)
        thread_wait(0, 1);
    num_reader+=1;
    thread_unlock(0);
}

static void
readerFinish()
{
    thread_lock(0);
    num_reader-=1;
    thread_broadcast(0, 1);
    thread_unlock(0);
}

static void
writerStart()
{
    thread_lock(0);
    while (num_writer > 0 || num_reader > 0)
        thread_wait(0, 1);
    num_writer+=1;
    thread_unlock(0);
}

static void
writerFinish()
{
    thread_lock(0);
    num_writer-=1;
    thread_broadcast(0, 1);
    thread_unlock(0);
}

static int v= 0;
static void
writer()
{
    writerStart();
    puts("seq1");
    cout<<"hello, writer " << v<<"\n";
    v+= 1;
    thread_yield();
    puts("seq2");
    writerFinish();
}
static void
reader()
{
    readerStart();
    puts("seq3");
    cout<<"hello, reader " <<v<<"\n";
    thread_yield();
    puts("seq4");
    readerFinish();
}
static void
mainthread()
{
    thread_create((thread_startfunc_t)writer, 0);	
	thread_create((thread_startfunc_t)reader, 0);
    thread_create((thread_startfunc_t)writer, 0);   
    thread_create((thread_startfunc_t)reader, 0);
    thread_create((thread_startfunc_t)reader, 0);
}
int
main (void)
{   
    thread_libinit((thread_startfunc_t)mainthread, (void *)0);    
}

/*
main:
    create(writer)
    create reader
    create writer
    create reader *2


writer:
    acquire(writerlock)
    print(seq1)
    yield
    print(seq2)
    release(writerlock)

reader:
    acquire(readerlock)
    print(seq3)
    yield
    print(seq4)
    release(readerlock)
expect:
seq1
   2
   3
   3
   3
   4
   4
   4
   1
   2








*/
