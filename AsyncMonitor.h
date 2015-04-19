#pragma once

//#define _HAS_ITERATOR_DEBUGGING 0
#include <string>
#include <thread>

using namespace std;

/* Asynchronous operation monitor base class. The base class does nothing useful -- override the virtual functions to customize the monitoring functionality. */
class AsyncMonitor {
protected:
	string stateMessage;
	string errorMessage;
	int errorCode;

public:
	std::thread threadHandle;

	AsyncMonitor() {}
	virtual ~AsyncMonitor() {}

	/* Notify the host process that a thread has begun execution. */
	virtual void Begin(const string& startStateMessage) {
		stateMessage = startStateMessage;
	}
	
	/* Update the status of the thread's ongoing execution.  */
	virtual void Update(const string& updateStateMessage) {
		stateMessage = updateStateMessage;
	}

	/* report an error state to the host process. */
	virtual void Error(const string& errorStateMessage, int error) {
		errorMessage = errorStateMessage;
		errorCode = error;
	}

	/* report successful completion of the thread's work, along with an error code.  
		Futher calls to this object should not be done -- derived classes can use this function to 
		clean up thread monitoring data (including this object itself)
	*/
	virtual void End (const string& endStateMessage, int code) {
		stateMessage = endStateMessage;
		errorCode = code;
	}

};

extern AsyncMonitor NoMonitor;

#define NULL_MONITOR &NoMonitor;
