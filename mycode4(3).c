/*   	Umix thread package
 *
 */

#include <setjmp.h>
#include "aux.h"
#include "umix.h"
#include "mycode4.h"

static int MyInitThreadsCalled = 0;	// 1 if MyInitThreads called, else 0

static struct thread {			// thread table
	int valid;			// 1 if entry is valid, else 0
	jmp_buf env;			// current context
} thread[MAXTHREADS];

static struct queue {			// thread table
	int id;			// 1 if entry is valid, else 0
	int valid;			// current context
} queue[MAXTHREADS];
#define STACKSIZE	65536		// maximum size of thread stack

/* 	MyInitThreads () initializes the thread package. Must be the first
 *  	function called by any user program that uses the thread package.  
 */
int curThread;
int curIndex;
int preThread;
int qSize;
void add(tmp)
	int tmp;
{
	queue[qSize].valid = 1;
	queue[qSize].id = tmp;
	qSize++;
}
void remove(tmp)
	int tmp;
{	int i;
	int test = 0;
	for(i = 0; i < qSize; i++){
		if(queue[i].id == tmp){
			test = 1;
			break;
		}
	}
	if(test == 1){
		for(;i<qSize-1;i++){
			queue[i].id = queue[i+1].id;
		}
		queue[qSize-1].valid = 0;
		qSize--;
	}
}
	
void MyInitThreads ()
{
	char stack[STACKSIZE];	// reserve space for thread 0's stack
	if (((int) &stack[STACKSIZE-1]) - ((int) &stack[0]) + 1 != STACKSIZE) {
		Printf ("Stack space reservation failed\n");
		Exit ();
		}
	int i;

	if (MyInitThreadsCalled) {		// run only once
		Printf ("InitThreads: should be called only once\n");
		Exit ();
	}

	for (i = 0; i < MAXTHREADS; i++) {	// initialize thread table
		thread[i].valid = 0;
		queue[i].valid = 0;
		queue[i].id = -1;
	}
	thread[0].valid = 1;			// initialize thread 0
	curThread = 0;
	curIndex = 0;
	preThread = 0;
	qSize = 0;
//	add(curThread);
	MyInitThreadsCalled = 1;
}

/*  	MyCreateThread (f, p) creates a new thread to execute
 * 	f (p), where f is a function with no return value and
 * 	p is an integer parameter.  The new thread does not begin
 *  	executing until another thread yields to it. 
 */

int MyCreateThread (f, p)
	void (*f)();			// function to be executed
	int p;				// integer parameter
{
	if (! MyInitThreadsCalled) {
		Printf ("CreateThread: Must call InitThreads first\n");
		Exit ();
	}
	int newThread;
	if (setjmp (thread[MyGetThread()].env) == 0) {	// save context of thread 0

		/* The new thread will need stack space. Here we use the
		 * following trick: the new thread simply uses the current
		 * stack.  So, there is no need to allocate space. However,
		 * to ensure that thread 0's stack may grow and (hopefully)
		 * not bump into thread 1's stack, the top of the stack is
		 * effectively extended automatically by declaring a local
		 * variable (a large "dummy" array).  This array is never
		 * actually used.  To prevent an optimizing compiler from
		 * removing it, it should be referenced. 
		 */
		        char stack[STACKSIZE];  // reserve space for thread 0's stack
        if (((int) &stack[STACKSIZE-1]) - ((int) &stack[0]) + 1 != STACKSIZE) {
                Printf ("Stack space reservation failed\n");
                Exit ();
                }

		void (*func)() = f;	// func saves f on top of stack
		int param = p;		// param saves p on top of stack
		int i;
		for(i = 0; i < MAXTHREADS; i++){
			if(thread[(i+curIndex)%MAXTHREADS].valid == 0){
				curIndex = curIndex +  i;
				newThread = (curIndex)%MAXTHREADS;
				thread[newThread].valid = 1;
				break;}
		}
		if(i == MAXTHREADS)
			return -1;
		else
			add(newThread);
		if (setjmp (thread[newThread].env) == 0) {	// save context of 1
			longjmp (thread[MyGetThread()].env, 1);	// back to thread 0
		}

		/* here when thread 1 is scheduled for the first time */

		(*func) (param);		// execute func (param)

		MyExitThread (MyGetThread());		// thread 1 is done - exit
	}
	return newThread;		// done, return new thread ID
}

/*   	MyYieldThread (t) causes the running thread, call it T, to yield to
 * 	thread t. Returns the ID of the thread that yielded to the calling
 *  	thread T, or -1 if t is an invalid ID.  Example: given two threads
 *  	with IDs 1 and 2, if thread 1 calls MyYieldThread (2), then thread 2
 * 	will resume, and if thread 2 then calls MyYieldThread (1), thread 1
 * 	will resume by returning from its call to MyYieldThread (2), which
 *  	will return the value 2.
 */

int MyYieldThread (t)
	int t;				// thread being yielded to
{
	if (! MyInitThreadsCalled) {
		Printf ("YieldThread: Must call InitThreads first\n");
		Exit ();
	}

	if (t < 0 || t >= MAXTHREADS) {
		Printf ("YieldThread: %d is not a valid thread ID\n", t);
		return (-1);
	}
	if (! thread[t].valid) {
		Printf ("YieldThread: Thread %d does not exist\n", t);
		return (-1);
	}

	if (setjmp (thread[MyGetThread()].env) == 0){
		preThread = curThread;
		curThread = t;
		remove(preThread);
		remove(curThread);
		if(preThread>= 0)
		add(preThread);
		longjmp(thread[t].env, 1);
	}else{
		//DPrintf("%d\n", preThread);
		return preThread;}
}

/*   	MyGetThread () returns ID of currently running thread. 
 */

int MyGetThread ()
{
	if (! MyInitThreadsCalled) {
		Printf ("GetThread: Must call InitThreads first\n");
		Exit ();
	}
	return curThread;
}

/* 	MySchedThread () causes the running thread to simply give up the
 *  	CPU and allow another thread to be scheduled.  Selecting which
 *  	thread to run is determined here.  Note that the same thread may
 * 	be chosen (as will be the case if there are no other threads). 
 */

void MySchedThread ()
{
	if (! MyInitThreadsCalled) {
		Printf ("SchedThread: Must call InitThreads first\n");
		Exit ();
	}
	if (qSize > 0 && setjmp (thread[MyGetThread()].env) == 0){
		preThread = -1;
		curThread = queue[0].id;
		remove(curThread);
		remove(preThread);
		if(preThread > 0)
			add(preThread);
		longjmp(thread[curThread].env, 1);
	}
/*	if(qSize > 0){
		curThread = -1;
		int tmp = queue[0].id;
                MyYieldThread (tmp);}*/
}

/* 	MyExitThread () causes the currently running thread to exit. 
 */

void MyExitThread ()
{
	if (! MyInitThreadsCalled) {
		Printf ("ExitThread: Must call InitThreads first\n");
		Exit ();
	}
	thread[curThread].valid = 0;
	//DPrintf("%d    %d \n",qSize,curThread);
	remove(curThread);
//	DPrintf("%d\n",qSize);
	curThread = -1;
	if(qSize > 0){
//		DPrintf("%d\n",qSize);
		MySchedThread ();}
//	DPrintf("end!!!!!!!!!!!!!");
	Exit();
}




