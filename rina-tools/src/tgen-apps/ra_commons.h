#pragma once

#include <cstdint>
#include <cstring>
#include <unistd.h>
#include <iostream>
#include <fstream>
#include <string>
#include <poll.h>

#include <rina/api.h>

#define TIMEOUT_MS 5000

namespace ra {

	typedef uint8_t	byte_t;

	/*
	ReadData and ReadDataTimeout read from Fd to Buffer.
	- Return values:
		*  0 -> No data read
		* <0 -> Failure reading data
		* >0 -> Number of bytes read
	- Size specifies the amount of data to read, otherwise packet size is specified by the first bytes read.
	- Timeout variants return 0 if no data is available before the specified time. Timeout time is specified in milliseconds
	*/
	int ReadData(const int Fd, byte_t * Buffer);
	int ReadData(const int Fd, byte_t * Buffer, const size_t Size);
	int ReadDataTimeout(const int Fd, byte_t * Buffer, const int mSec = 0);
	int ReadDataTimeout(const int Fd, byte_t * Buffer, const size_t  Size, const int mSec = 0);

	/*
	WriteData and WriteDataTimeout send data from  Buffer to Fd.
	- Return values:
	*  0 -> No data writed
	* <0 -> Failure writing data
	* >0 -> Number of bytes wrote, can be less than Size.
	- Size specifies the amount of data to write.
	- Timeout variants return 0 if no data is available before the specified time. Timeout time is specified in milliseconds
	*/
	int WriteData(const int Fd, byte_t * Buffer, const size_t Size,
		      size_t max_pdu_size);
	int WriteDataTimeout(const int Fd, byte_t * Buffer, const size_t Size,
			     int max_pdu_size, const int mSec = 0);

	/*
	AllocFlow allocates a flow to AppName.
	- Return values:
		*  0 -> Failure creating flow
		* >0 -> File Descriptor of the created flow
	- Parameter DIFName = NULL -> system decides best DIF
	- If used, parameters mSec specifies the timeout for the call in milliseconds.
		If mSec not used or mSec < 0 :> blocking call
	*/
	int AllocFlow(const char * MyName, const char * AppName, const struct rina_flow_spec * FlowSpec, const char * DIFName = NULL);
	int AllocFlow(const char * MyName, const char * AppName, const struct rina_flow_spec * FlowSpec, const int mSec, const char * DIFName = NULL);

	/*
	Register the MyApp in DIFName
	- Return values:
	* false -> Failure creating flow
	* true	-> Success creating flow
	- Parameter Cfd saves the result of rina_open(). if Cfd < 0, rina_open() will be called and Cfd overwrited.
	- Parameter DIFName = NULL -> system decides best DIF
	- Parameters mSec specifies the timeout for the call. Timeout time is specified in milliseconds. mSec < 0 :> blocking call
	*/
	bool RegisterApp(int & Cfd, const char * MyName, const char * DIFName = NULL);
	bool RegisterApp(int & Cfd, const char * MyName, const int mSec, const char * DIFName = NULL);

	/*
	Listen for flow connection
	- Return values:
	* 0	 -> Timeout
	* >0 -> Flow Descriptor of new flow
	* <0 -> Error
	- Parameter Cfd has the result of rina_open().
	- Parameters mSec specifies the timeout for the call. Timeout time is specified in milliseconds. mSec < 0 :> blocking call
	*/
	int ListenFlow(const int Cfd, char **RemoteApp = NULL, struct rina_flow_spec * FlowSpec = NULL);
	int ListenFlow(const int Cfd, const int mSec, char **RemoteApp = NULL, struct rina_flow_spec * FlowSpec = NULL);

	/*
	Update FlowSpec with QoS requirements read from File "Filename"
	- Return values:
	* true	 -> File read and all parameters valid
	* false  -> Something failed
	- If the file is correctly read but some parameters are not valid, valid values are still updated.
	*/
	bool ParseQoSRequirementsFile(struct rina_flow_spec * FlowSpec, char * Filename);

	//Implementation
	int ReadData(const int Fd, byte_t * Buffer) {
		size_t  Rem = sizeof(size_t);
		ssize_t Ret = read(Fd, Buffer, Rem);
		if (Ret <= 0) {
			if (Ret == 0) {
				std::cout << "ReadData () failed: return 0" << std::endl;
			}
			else {
				std::cout << "ReadData () failed: " << strerror(errno) << std::endl;
			}

			return Ret;
		}

		return ReadData(Fd, Buffer + Rem, (*(size_t *)Buffer) - Rem) + Rem;
	}

	int ReadData(const int Fd, byte_t * Buffer, const size_t Size) {
		int DataSize = 0;
		size_t  Rem = Size;
		ssize_t Ret;

		do {
			Ret = read(Fd, Buffer, Rem);
			if (Ret <= 0) {
				if (Ret == 0) {
					std::cout << "ReadData () failed: return 0" << std::endl;
				}
				else {
					std::cout << "ReadData () failed: " << strerror(errno) << std::endl;
				}

				return Ret;
			}
			Rem -= Ret;
			Buffer += Ret;
			DataSize += Ret;
		} while (Rem > 0);

		return DataSize;
	}

	int ReadDataTimeout(const int Fd, byte_t * Buffer, const int mSec) {
		struct pollfd Fds = { .fd = Fd,.events = POLLIN };
		int PollRet = poll(&Fds, 1, mSec);

		if (PollRet != 1) {
			return PollRet;
		}

		if (Fds.revents & POLLIN == 0) {
			return -1;
		}

		return ReadData(Fd, Buffer);
	}

	int ReadDataTimeout(const int Fd, byte_t * Buffer, const size_t  Size, const int mSec) {
		struct pollfd Fds = { .fd = Fd,.events = POLLIN };
		int PollRet = poll(&Fds, 1, mSec);

		if (PollRet != 1) {
			return PollRet;
		}
		if (Fds.revents & POLLIN == 0) {
			return -1;
		}

		return ReadData(Fd, Buffer, Size);
	}

	int WriteData(const int Fd, byte_t * Buffer, const size_t Size,
		      size_t max_pdu_size) {
		try {
			int W = 0;
			size_t tSize = Size;
			while(tSize > max_pdu_size){
			 	ssize_t Wt = write(Fd, Buffer, max_pdu_size);
				if(Wt < 0){
				 	return -1;
				}
				tSize -= max_pdu_size;
				Buffer += max_pdu_size;
				W += Wt;
			}
			ssize_t Wt = write(Fd, Buffer, tSize);
			if(Wt < 0){
				return -1;
			}
			return W + Wt;
		} catch (...) {
			return -1;
		}
	}

	int WriteDataTimeout(const int Fd, byte_t * Buffer, const size_t Size,
			     int max_pdu_size, const int mSec ) {
		struct pollfd Fds = { .fd = Fd,.events = POLLOUT };
		int PollRet = poll(&Fds, 1, mSec);

		if (PollRet != 1) {
			return PollRet;
		}
		if (Fds.revents & POLLOUT == 0) {
			return -1;
		}

		return WriteData(Fd, Buffer, Size, max_pdu_size);
	}

	int AllocFlow(const char * MyName, const char * AppName, const struct rina_flow_spec * FlowSpec,
		      const char * DIFName) {
		int Fd = rina_flow_alloc(DIFName, MyName, AppName, FlowSpec, 0);
		if (Fd < 0) {
			std::cout << "rina_flow_alloc () failed: " << strerror(errno) << std::endl;
		}

		return Fd;
	}

	int AllocFlow(const char * MyName, const char * AppName, const struct rina_flow_spec * FlowSpec,
		      const int mSec, const char * DIFName) {
		if (mSec < 0) {
			return AllocFlow(MyName, AppName, FlowSpec, DIFName);
		}

		int Fd = rina_flow_alloc(DIFName, MyName, AppName, FlowSpec, RINA_F_NOWAIT);
		if (Fd < 0) {
			std::cout << "rina_flow_alloc () failed: " << strerror(errno) << std::endl;
		}

		struct pollfd Fds = { .fd = Fd,.events = POLLIN };
		int PollRet = poll(&Fds, 1, mSec);

		if (PollRet != 1 || Fds.revents & POLLIN == 0) {
			std::cout << "rina_flow_alloc () failed: timeout | "<< mSec << " milliseconds"  << std::endl;
			return -1;
		}
		Fd = rina_flow_alloc_wait(Fd);
		if (Fd < 0) {
			std::cout << "rina_flow_alloc_wait () failed: " << strerror(errno) << std::endl;
		}

		return Fd;
	}

	bool RegisterApp(int & Cfd, const char * MyName, const char * DIFName) {
		if (Cfd < 0) {
			Cfd = rina_open();
			if (Cfd < 0) {
				std::cout << "rina_open () failed: return " << Cfd << std::endl;
				return false;
			}
		}

		if (rina_register(Cfd, DIFName, MyName, 0) < 0) {
			std::cout << "rina_register () failed: " << strerror(errno) << std::endl;
			return false;
		}

		return true;
	}

	bool RegisterApp(int & Cfd, const char * MyName, const int mSec, const char * DIFName) {
		if (mSec < 0) {
			return RegisterApp(Cfd, MyName, DIFName);
		}

		if (Cfd < 0) {
			Cfd = rina_open();
			if (Cfd < 0) {
				std::cout << "rina_open () failed: return " << Cfd << std::endl;
				return false;
			}
		}

		int Fd = rina_register(Cfd, DIFName, MyName, RINA_F_NOWAIT);
		if (Fd < 0) {
			std::cerr << "rina_register () failed: " << strerror(errno) << std::endl;
			return false;
		}

		struct pollfd Fds = { .fd = Fd,.events = POLLIN };
		int PollRet = poll(&Fds, 1, mSec);

		if (PollRet != 1 || Fds.revents & POLLIN == 0) {
			std::cout << "rina_register () failed: timeout" << std::endl;
			return false;
		}

		if (rina_register_wait(Cfd, Fd) < 0) {
			std::cout << "rina_register_wait () failed: " << strerror(errno) << std::endl;
			return false;
		}

		return true;
	}

	int ListenFlow(const int Cfd, char **RemoteApp, struct rina_flow_spec * FlowSpec) {
		int Fd = rina_flow_accept(Cfd, RemoteApp, FlowSpec, 0);
		if (Fd < 0) {
			std::cout << "rina_flow_accept () failed: " << strerror(errno) << std::endl;
		}

		return Fd;
	}

	int ListenFlow(const int Cfd, const int mSec, char **RemoteApp, struct rina_flow_spec * FlowSpec) {
		if (mSec < 0) {
			return ListenFlow(Cfd, RemoteApp, FlowSpec);
		}

		struct pollfd Fds = { .fd = Cfd,.events = POLLIN };
		int PollRet = poll(&Fds, 1, mSec);
		if (PollRet <= 1 || Fds.revents & POLLIN == 0) {
			std::cout << "ListenFlow () failed: timeout" << std::endl;
			return 0;
		}

		int Fd = rina_flow_accept(Cfd, RemoteApp, FlowSpec, RINA_F_NORESP);
		if (Fd < 0) {
			std::cout << "rina_flow_accept () failed: " << strerror(errno) << std::endl;
		}

		Fd = rina_flow_respond(Cfd, Fd, 0);
		if (Fd < 0) {
			std::cout << "rina_flow_respond () failed: " << strerror(errno) << std::endl;
		}

		return Fd;
	}
}
