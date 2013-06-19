//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//

#include <iostream>
#include <math.h>
#include "concurrency.h"
#define NUM_THREADS 5

using namespace rina;

void * doWork(void * arg) {
	int number = (int) arg;
	std::cout << "Thread " << number << " started work\n";
	return (void *) number;
}

class LockableCounter: public Lockable {
public:
	LockableCounter() :
			Lockable() {
		counter = 0;
	}
	void count() {
		lock();
		counter++;
		std::cout << "Incremented counter, current value: " << counter << "\n";
		unlock();
	}

	int getCount() {
		return counter;
	}
private:
	int counter;
};

void * doWorkMutex(void * arg) {
	LockableCounter * counter = (LockableCounter *) arg;
	counter->count();
	return (void *) 0;
}

class ReadWriteLockableCounter: public ReadWriteLockable {
public:
	ReadWriteLockableCounter() :
			ReadWriteLockable() {
		counter = 0;
	}

	void count() {
		writelock();
		counter++;
		std::cout<<"Incremented counter; current value is "<<
				counter<<"\n";
		unlock();
	}

	int getCounter() {
		int result;
		readlock();
		result = counter;
		unlock();
		return result;
	}

private:
	int counter;
};

void * doWorkReadWriteLock(void * arg) {
	ReadWriteLockableCounter * counter = (ReadWriteLockableCounter *) arg;
	counter->count();
	return (void *) 0;
}

int main(int argc, char * argv[]) {
	std::cout << "TESTING CONCURRENCY WRAPPER CLASSES\n";

	/* Test get concurrency */
	std::cout << "Concurrency: " << Thread::getConcurrency() << "\n";

	/* Test self */
	Thread * myself = Thread::self();
	std::cout << "Thread id: " << myself->getThreadType() << "\n";

	/* Should throw an Exception, I cannot join myself */
	try {
		myself->join(NULL);
	} catch (ConcurrentException &e) {
		std::cout << "Caught expected exception. " << e.what() << "\n";
	}

	/* Test equals */
	Thread *myself2 = Thread::self();
	if ((*myself) != (*myself2)) {
		std::cout << "Error, both threads should have been equal\n";
		return -1;
	}

	delete myself;
	delete myself2;

	/* Test Thread creation and joining */
	Thread * threads[NUM_THREADS];
	ThreadAttributes * threadAttributes = new ThreadAttributes();
	threadAttributes->setJoinable();
	for (int i = 0; i < NUM_THREADS; i++) {
		threads[i] = new Thread(threadAttributes, &doWork, (void *) i);
		std::cout << "Created thread " << i << " with id "
				<< threads[i]->getThreadType() << "\n";
	}
	delete threadAttributes;

	void * status;
	for (int i = 0; i < NUM_THREADS; i++) {
		threads[i]->join(&status);
		std::cout << "Completed join with thread " << i
				<< " having a status of " << status << "\n";
		delete threads[i];
	}

	/* Test mutex */
	threadAttributes = new ThreadAttributes();
	threadAttributes->setJoinable();
	LockableCounter * counter = new LockableCounter();
	for (int i = 0; i < NUM_THREADS; i++) {
		threads[i] = new Thread(threadAttributes, &doWorkMutex,
				(void *) counter);
		std::cout << "Created thread " << i << " with id "
				<< threads[i]->getThreadType() << "\n";
	}
	delete threadAttributes;

	for (int i = 0; i < NUM_THREADS; i++) {
		threads[i]->join(&status);
		std::cout << "Completed join with thread " << i
				<< " having a status of " << status << "\n";
		delete threads[i];
	}

	std::cout << "Count value is " << counter->getCount() << "\n";
	if (counter->getCount() != NUM_THREADS){
		return -1;
	}
	delete counter;

	/* Test read-write lock */
	threadAttributes = new ThreadAttributes();
	threadAttributes->setJoinable();
	ReadWriteLockableCounter * counter2 = new ReadWriteLockableCounter();
	for (int i = 0; i < NUM_THREADS; i++) {
		threads[i] = new Thread(threadAttributes, &doWorkReadWriteLock,
				(void *) counter2);
		std::cout << "Created thread " << i << " with id "
				<< threads[i]->getThreadType() << "\n";
	}
	delete threadAttributes;

	for (int i = 0; i < NUM_THREADS; i++) {
		threads[i]->join(&status);
		std::cout << "Completed join with thread " << i
				<< " having a status of " << status
				<< ". Current counter value is " << counter2->getCounter()
				<< "\n";
		delete threads[i];
	}

	std::cout << "Count value is " << counter->getCount() << "\n";
	if (counter2->getCounter() != NUM_THREADS){
		return -1;
	}
	delete counter2;

	/* Test exit */
	Thread::exit(NULL);
	/* This piece of code won't be invoked since it is called after Thread::exit() */
	return -1;
}
