/*
* Copyright 2008 Free Software Foundation, Inc.
*
* This software is distributed under the terms of the GNU Public License.
* See the COPYING file in the main directory for details.
*
* This use of this software may be subject to additional restrictions.
* See the LEGAL file in the main directory for details.

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/


#ifndef INTERTHREAD_H
#define INTERTHREAD_H

#include "Timeval.h"
#include "Threads.h"
#include "LinkedLists.h"
#include <map>
#include <vector>
#include <queue>





/**@defgroup Templates for interthread mechanisms. */
//@{


/** Pointer FIFO for interthread operations.  */
template <class T> class InterthreadQueue {

	protected:

	PointerFIFO mQ;	
	mutable Mutex mLock;
	mutable Signal mWriteSignal;


	public:

	/** Delete contents. */
	void clear()
	{
		mLock.lock();
		while (mQ.size()>0) delete (T*)mQ.get();
		mLock.unlock();
	}



	~InterthreadQueue()
		{ clear(); }


	size_t size() const
	{
		mLock.lock();
		size_t retVal = mQ.size();
		mLock.unlock();
		return retVal;
	}

	/**
		Blocking read.
		@return Pointer to object (will not be NULL).
	*/
	T* read()
	{
		mLock.lock();
		T* retVal = (T*)mQ.get();
		while (retVal==NULL) {
			mWriteSignal.wait(mLock);
			retVal = (T*)mQ.get();
		}
		mLock.unlock();
		return retVal;
	}

	/**
		Blocking read with a timeout.
		@param timeout The read timeout in ms.
		@return Pointer to object or NULL on timeout.
	*/
	T* read(unsigned timeout)
	{
		if (timeout==0) return readNoBlock();
		mLock.lock();
		Timeval waitTime(timeout);
		while ((mQ.size()==0) && (!waitTime.passed()))
			mWriteSignal.wait(mLock,waitTime.remaining());
		T* retVal = (T*)mQ.get();
		mLock.unlock();
		return retVal;
	}

	/**
		Non-blocking read.
		@return Pointer to object or NULL if FIFO is empty.
	*/
	T* readNoBlock()
	{
		mLock.lock();
		T* retVal = (T*)mQ.get();
		mLock.unlock();
		return retVal;
	}

	/** Non-blocking write. */
	void write(T* val)
	{
		mLock.lock();
		mQ.put(val);
		mWriteSignal.signal();
		mLock.unlock();
	}


};



/** Pointer FIFO for interthread operations.  */
template <class T> class InterthreadQueueWithWait {

	protected:

	PointerFIFO mQ;	
	mutable Mutex mLock;
	mutable Signal mWriteSignal;
	mutable Signal mReadSignal;

	virtual void freeElement(T* element) const { delete element; };

	public:

	/** Delete contents. */
	void clear()
	{
		mLock.lock();
		while (mQ.size()>0) freeElement((T*)mQ.get());
		mReadSignal.signal();
		mLock.unlock();
	}



	~InterthreadQueueWithWait()
		{ clear(); }


	size_t size() const
	{
		mLock.lock();
		size_t retVal = mQ.size();
		mLock.unlock();
		return retVal;
	}

	/**
		Blocking read.
		@return Pointer to object (will not be NULL).
	*/
	T* read()
	{
		mLock.lock();
		T* retVal = (T*)mQ.get();
		while (retVal==NULL) {
			mWriteSignal.wait(mLock);
			retVal = (T*)mQ.get();
		}
		mReadSignal.signal();
		mLock.unlock();
		return retVal;
	}

	/**
		Blocking read with a timeout.
		@param timeout The read timeout in ms.
		@return Pointer to object or NULL on timeout.
	*/
	T* read(unsigned timeout)
	{
		if (timeout==0) return readNoBlock();
		mLock.lock();
		Timeval waitTime(timeout);
		while ((mQ.size()==0) && (!waitTime.passed()))
			mWriteSignal.wait(mLock,waitTime.remaining());
		T* retVal = (T*)mQ.get();
		if (retVal!=NULL) mReadSignal.signal();
		mLock.unlock();
		return retVal;
	}

	/**
		Non-blocking read.
		@return Pointer to object or NULL if FIFO is empty.
	*/
	T* readNoBlock()
	{
		mLock.lock();
		T* retVal = (T*)mQ.get();
		if (retVal!=NULL) mReadSignal.signal();
		mLock.unlock();
		return retVal;
	}

	/** Non-blocking write. */
	void write(T* val)
	{
		mLock.lock();
		mQ.put(val);
		mWriteSignal.signal();
		mLock.unlock();
	}

	/** Wait until the queue falls below a low water mark. */
	void wait(size_t sz=0)
	{
		mLock.lock();
		while (mQ.size()>sz) mReadSignal.wait(mLock);
		mLock.unlock();
	}

};





/** Thread-safe map of pointers to class D, keyed by class K. */
template <class K, class D > class InterthreadMap {

protected:

	typedef std::map<K,D*> Map;
	Map mMap;
	mutable Mutex mLock;
	Signal mWriteSignal;

public:

	void clear()
	{
		mLock.lock();
		// Delete everything in the map.
		typename Map::iterator iter = mMap.begin();
		while (iter != mMap.end()) {
			delete iter->second;
			++iter;
		}
		mMap.clear();
		mLock.unlock();
	}

	~InterthreadMap() { clear(); }

	/**
		Non-blocking write.
		@param key The index to write to.
		@param wData Pointer to data, not to be deleted until removed from the map.
	*/
	void write(const K &key, D * wData)
	{
		mLock.lock();
		typename Map::iterator iter = mMap.find(key);
		if (iter!=mMap.end()) {
			delete iter->second;
			iter->second = wData;
		} else {
			mMap[key] = wData;
		}
		mWriteSignal.broadcast();
		mLock.unlock();
	}

	/**
		Non-blocking read with element removal.
		@param key Key to read from.
		@return Pointer at key or NULL if key not found, to be deleted by caller.
	*/
	D* getNoBlock(const K& key)
	{
		mLock.lock();
		typename Map::iterator iter = mMap.find(key);
		if (iter==mMap.end()) {
			mLock.unlock();
			return NULL;
		}
		D* retVal = iter->second;
		mMap.erase(iter);
		mLock.unlock();
		return retVal;
	}

	/**
		Blocking read with a timeout and element removal.
		@param key The key to read from.
		@param timeout The blocking timeout in ms.
		@return Pointer at key or NULL on timeout, to be deleted by caller.
	*/
	D* get(const K &key, unsigned timeout)
	{
		if (timeout==0) return getNoBlock(key);
		mLock.lock();
		Timeval waitTime(timeout);
		typename Map::iterator iter = mMap.find(key);
		while ((iter==mMap.end()) && (!waitTime.passed())) {
			mWriteSignal.wait(mLock,waitTime.remaining());
			iter = mMap.find(key);
		}
		if (iter==mMap.end()) {
			mLock.unlock();
			return NULL;
		}
		D* retVal = iter->second;
		mMap.erase(iter);
		mLock.unlock();
		return retVal;
	}

	/**
		Blocking read with and element removal.
		@param key The key to read from.
		@return Pointer at key, to be deleted by caller.
	*/
	D* get(const K &key)
	{
		mLock.lock();
		typename Map::iterator iter = mMap.find(key);
		while (iter==mMap.end()) {
			mWriteSignal.wait(mLock);
			iter = mMap.find(key);
		}
		D* retVal = iter->second;
		mMap.erase(iter);
		mLock.unlock();
		return retVal;
	}


	/**
		Remove an entry and delete it.
		@param key The key of the entry to delete.
		@return True if it was actually found and deleted.
	*/
	bool remove(const  K &key )
	{
		D* val = getNoBlock(key);
		if (!val) return false;
		delete val;
		return true;
	}


	/**
		Non-blocking read.
		@param key Key to read from.
		@return Pointer at key or NULL if key not found.
	*/
	D* readNoBlock(const K& key) const
	{
		D* retVal=NULL;
		mLock.lock();
		typename Map::const_iterator iter = mMap.find(key);
		if (iter!=mMap.end()) retVal = iter->second;
		mLock.unlock();
		return retVal;
	}

	/**
		Blocking read with a timeout.
		@param key The key to read from.
		@param timeout The blocking timeout in ms.
		@return Pointer at key or NULL on timeout.
	*/
	D* read(const K &key, unsigned timeout) const
	{
		if (timeout==0) return readNoBlock(key);
		mLock.lock();
		Timeval waitTime(timeout);
		typename Map::const_iterator iter = mMap.find(key);
		while ((iter==mMap.end()) && (!waitTime.passed())) {
			mWriteSignal.wait(mLock,waitTime.remaining());
			iter = mMap.find(key);
		}
		if (iter==mMap.end()) {
			mLock.unlock();
			return NULL;
		}
		D* retVal = iter->second;
		mLock.unlock();
		return retVal;
	}

	/**
		Blocking read.
		@param key The key to read from.
		@return Pointer at key.
	*/
	D* read(const K &key) const
	{
		mLock.lock();
		typename Map::const_iterator iter = mMap.find(key);
		while (iter==mMap.end()) {
			mWriteSignal.wait(mLock);
			iter = mMap.find(key);
		}
		D* retVal = iter->second;
		mLock.unlock();
		return retVal;
	}

};







/** This class is used to provide pointer-based comparison in priority_queues. */
template <class T> class PointerCompare {

	public:

	/** Compare the objects pointed to, not the pointers themselves. */
	bool operator()(const T *v1, const T *v2)
		{ return (*v1)>(*v2); }

};



/**
	Priority queue for interthread operations.
	Passes pointers to objects.
*/
template <class T, class C = std::vector<T*>, class Cmp = PointerCompare<T> > class InterthreadPriorityQueue {

	protected:

	std::priority_queue<T*,C,Cmp> mQ;
	mutable Mutex mLock;
	mutable Signal mWriteSignal;

	public:


	/** Clear the FIFO. */
	void clear()
	{
		mLock.lock();
		while (mQ.size()>0)	{
			T* ptr = mQ.top();
			mQ.pop();
			delete ptr;
		}
		mLock.unlock();
	}


	~InterthreadPriorityQueue()
	{
		clear();
	}

	size_t size() const
	{
		mLock.lock();
		size_t retVal = mQ.size();
		mLock.unlock();
		return retVal;
	}


	/** Non-blocking read. */
	T* readNoBlock()
	{
		T* retVal = NULL;
		mLock.lock();
		if (mQ.size()!=0) {
			retVal = mQ.top();
			mQ.pop();
		}
		mLock.unlock();
		return retVal;
	}

	/** Blocking read. */
	T* read()
	{
		T* retVal;
		mLock.lock();
		while (mQ.size()==0) mWriteSignal.wait(mLock);
		retVal = mQ.top();
		mQ.pop();
		mLock.unlock();
		return retVal;
	}

	/** Non-blocking write. */
	void write(T* val)
	{
		mLock.lock();
		mQ.push(val);
		mWriteSignal.signal();
		mLock.unlock();
	}

};





class Semaphore {

	private:

	bool mFlag;
	Signal mSignal;
	mutable Mutex mLock;

	public:

	Semaphore()
		:mFlag(false)
	{ }

	void post()
	{
		mLock.lock();
		mFlag=true;
		mSignal.signal();
		mLock.unlock();
	}

	void get()
	{
		mLock.lock();
		while (!mFlag) mSignal.wait(mLock);
		mFlag=false;
		mLock.unlock();
	}

	bool semtry()
	{
		mLock.lock();
		bool retVal = mFlag;
		mFlag = false;
		mLock.unlock();
		return retVal;
	}

};





//@}




#endif
// vim: ts=4 sw=4
