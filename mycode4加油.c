/*   	Umix thread package
 *
 */

#include <setjmp.h>
#include "aux.h"
#include "umix.h"
#include "mycode4.h"
#include "string.h"

static int MyInitThreadsCalled = 0;	

static struct thread {			
	int valid;			
	jmp_buf env;			
	jmp_buf iniEnv;
	void (*f)();
	int p;	
} thread[MAXTHREADS];

static struct queue {			
	int id;			
	int valid;			
} queue[MAXTHREADS];
#define STACKSIZE	65536		
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
	char stack[STACKSIZE];	
	if (((int) &stack[STACKSIZE-1]) - ((int) &stack[0]) + 1 != STACKSIZE) {
		Printf ("Stack space reservation failed\n");
		Exit ();
		}
	int i;

	if (MyInitThreadsCalled) {		
		Printf ("InitThreads: should be called only once\n");
		Exit ();
	}

	for (i = 0; i < MAXTHREADS; i++) {	
		thread[i].valid = 0;
		queue[i].valid = 0;
		queue[i].id = -1;
	}
	thread[0].valid = 1;			
	curThread = 0;
	curIndex = 0;
	preThread = 0;
	qSize = 0;
	MyInitThreadsCalled = 1;
	for(i = 0; i < MAXTHREADS; i++){
		char stack[STACKSIZE*i]; 
		if (((int) &stack[STACKSIZE*i-1]) - ((int) &stack[0]) + 1 != STACKSIZE*i) {
			Printf ("Stack space reservation failed\n");
			Exit ();
		}
		curThread = i;
		if(setjmp(thread[curThread].env) == 0){
			memcpy(thread[i].iniEnv, thread[i].env, sizeof(jmp_buf));}
		else{
			thread[curThread].f (thread[curThread].p);
			MyExitThread (MyGetThread());
		}
	}
	curThread = 0;	
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
		memcpy(thread[newThread].env, thread[newThread].iniEnv, sizeof(jmp_buf));
		thread[newThread].f = f;
		thread[newThread].p = p;
		return newThread;		
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
	remove(curThread);
	curThread = -1;
	if(qSize > 0){
		MySchedThread ();}
	Exit();
}


