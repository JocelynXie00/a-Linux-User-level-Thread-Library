#include "thread.h"
#include <stdio.h>
#include <queue>
#include <stdlib.h>
#include <iostream>
#include "interrupt.h"
#include <string>

//#include "thread.h"
//#include "interrupt.h"
using namespace std;
void child_thread(void *);
void main_thread(void *);
void pressure_thread(void *);
void main_thread(void* arg){
   if (thread_libinit(main_thread, 0)){
      cout << "success, we cannot libinit twice\n";
   }
   if (thread_lock(1)){
      cout << "lock failed\n";
   }
   if (thread_lock(1)){
      cout << "success, a thread cannot lock twice\n";
   }
   if (thread_wait(10,1)){
      cout << "success, we cannot wait without holding a lock\n";
   }
   if (thread_unlock(2)){
      cout << "success, we cannot unlock a lock we did not hold or not exists\n";
   }
   thread_create(child_thread, 0);
   // test three times yield, should output seq1seq2seq3seq4\n
   cout << "seq1";
   thread_yield();
   cout << "seq3";
   thread_yield();
   cout << "seq1\n";
   thread_yield(); // will come back to this thread
   cout << "seq2\n";
   thread_unlock(1);
   thread_yield();
   cout << "seq4\n";
   // pressure test
   while (1){
      if (thread_create(pressure_thread, 0)){
         cout << "memory used up\n";
         break;
      }
   }
}
void child_thread(void *arg){
   cout << "seq2";
   thread_yield();
   cout << "seq4\n";
   if (thread_unlock(1)){
      cout << "success, we cannot unlock a lock we did not hold\n";
   }
   thread_lock(1); // should next print seq1
   cout << "seq3\n";
}

void pressure_thread(void *arg){
}

int main(int argc, char const *argv[])
{
   // test 
   if (thread_create(child_thread, 0)){
      cout << "success, we cannot create thread before libinit\n";
   }
   if (thread_lock(1)){
      cout << "success, we cannot lock before libinit\n";
   }
   if (thread_unlock(2) and thread_unlock(1)){
      cout << "success, we cannot unlock before libinit\n";
   }
   if (thread_wait(2,3) and thread_wait(1,3)){
      cout << "success, we cannot wait before libinit\n";
   }
   if (thread_broadcast(2,3) and thread_broadcast(1,3)){
      cout << "success, we cannot broadcast before libinit\n";
   }
   if (thread_signal(2,3) and thread_signal(1,3)){
      cout << "success, we cannot signal before libinit\n";
   }
   if (thread_yield()){
      cout << "success, we cannot yield before libinit\n";
   }
   thread_libinit(main_thread, 0);

   // libinit should return 0;
   // here should be output
   cout << "here is unreachable\n";



}