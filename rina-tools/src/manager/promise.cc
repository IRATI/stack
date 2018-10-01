/*
 * Promise framework (implement sync calls)
 *
 *    Marc Sune             <marc.sune (at) bisdn.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA  02110-1301  USA
 */

#define RINA_PREFIX     "promise"
#include <librina/logs.h>

#include "net-manager.h"

using namespace std;

netman_res_t Promise::wait(void)
{
	unsigned int i;
	// Due to the async nature of the API, notifications (signal)
	// the transaction can well end before the thread is waiting
	// in the condition variable. As apposed to sempahores
	// pthread_cond don't keep the "credit"
	for (i = 0;
			i < PROMISE_TIMEOUT_S * (_PROMISE_1_SEC_NSEC / PROMISE_RETRY_NSEC);
			++i)
	{
		try
		{
			if (ret != NETMAN_PENDING)
				return ret;
			wait_cond.timedwait(0, PROMISE_RETRY_NSEC);
		} catch (...)
		{
		};
	}

	//hard timeout expired
	if (!trans->abort())
		//The transaction ended at the very last second
		return ret;
	ret = NETMAN_FAILURE;
	return ret;
}

netman_res_t Promise::timed_wait(const unsigned int seconds)
{

	if (ret != NETMAN_PENDING)
		return ret;
	try
	{
		wait_cond.timedwait(seconds, 0);
	} catch (rina::ConcurrentException& e)
	{
		if (ret != NETMAN_PENDING)
			return ret;
		return NETMAN_PENDING;
	};
	return ret;
}

//
// Transactions
//
unsigned int NetworkManager::get_next_tid()
{
	return tid_gen.next();
}

int NetworkManager::add_transaction_state(TransactionState * t)
{
	//Lock
	rina::WriteScopedLock writelock(trans_rwlock);

	//Check if exists already
	if (pend_transactions.find(t->tid) != pend_transactions.end()) {
		return -1;
	}

	//Just add and return
	try {
		pend_transactions[t->tid] = t;
	} catch (...) {
		LOG_DBG("Could not add transaction %s. Out of memory?",
			t->tid.c_str());
		return -1;
	}

	return 0;
}

int NetworkManager::remove_transaction_state(const std::string& tid)
{
	TransactionState* t;
	//Lock
	rina::WriteScopedLock writelock(trans_rwlock);

	//Check if it really exists
	if (pend_transactions.find(tid) == pend_transactions.end())
		return -1;

	t = pend_transactions[tid];
	pend_transactions.erase(tid);
	delete t;

	return 0;
}

TransactionState * NetworkManager::get_transaction_state(const std::string& tid)
{
	TransactionState* state;

	//Read lock
	rina::ReadScopedLock readlock(trans_rwlock);

	if ( pend_transactions.find(tid) != pend_transactions.end() ) {
		state = pend_transactions[tid];
		return state;
	}
	return NULL;
}
