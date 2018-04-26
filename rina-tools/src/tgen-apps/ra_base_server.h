#pragma once

#include <string>
#include <vector>
#include <thread>
#include <mutex>

#include "ra_commons.h"

namespace ra {
	class BaseServer {
	public:
		BaseServer(const std::string& Name, const std::string& Instance,
			   bool verbose);
		virtual ~BaseServer(){};
		void AddToDIF(std::string DIF);

		int Run();

	protected:
		int RegisterTimeoutMs;
		int ListenTimeoutMs;

		std::string MyName;
		std::vector<std::string> DIFs;
		bool CloseOnNextLoop;
		int  NumRunningThreads;
		bool WaitForClose;
		bool verbose;

		virtual int HandleFlow(const int Fd) { return 0; }
		virtual int AfterEnd() { return 0; };
		virtual void AfterEndFlow(const int & Fd, const int ReturnCode) {};

	private:
		std::mutex Mtx;

		void RunThread(const int & Fd);
	};

	BaseServer::BaseServer(const std::string& Name, const std::string& Instance,
			       bool verb) {
		MyName = Name + "|" + Instance;
		RegisterTimeoutMs = -1;
		ListenTimeoutMs = -1;
		WaitForClose = false;
		verbose = verb;
		NumRunningThreads = 0;
		CloseOnNextLoop = false;
	}

	void BaseServer::AddToDIF(const std::string DIF) {
		if (std::find(DIFs.begin(), DIFs.end(), DIF) == DIFs.end()) {
			DIFs.push_back(DIF);
		}
	}

	int BaseServer::Run() {
		if (verbose)
			std::cout << "BaseServer Run()"<< std::endl;

		int Cfd = -1;

		if (DIFs.empty()) {
			if (!RegisterApp(Cfd, MyName.c_str(), RegisterTimeoutMs) ) {
				std::cout << "Cfd = " << Cfd
					  << " | Registered into default DIF failed"
					  << std::endl;
				return -1;
			} else if (verbose) {
				std::cout << "Cfd = " << Cfd
					  << " | Registered into default DIF" << std::endl;
			}
		} else {
			for (auto DIF : DIFs) {
				if (!RegisterApp(Cfd, MyName.c_str(), RegisterTimeoutMs, DIF.c_str() ) ) {
					std::cout << "Cfd = " << Cfd << " | Registered into DIF " << DIF << " failed" << std::endl;
					return -1;
				}
				else if (verbose) {
					std::cout << "Cfd = " << Cfd << " | Registered into DIF " << DIF << std::endl;
				}
			}
		}

		CloseOnNextLoop = false;
		NumRunningThreads = 0;

		if (verbose)
			std::cout << "Start listening loop"<< std::endl;

		for (;;) {
			if (CloseOnNextLoop) {
				if (verbose)
					std::cout << "Close listening loop"<< std::endl;
				break;
			}
			int Fd = ListenFlow(Cfd, ListenTimeoutMs, NULL, NULL);
			if (Fd == 0) {
				continue;
			}
			if (Fd < 0) {
				std::cout << "Bad Fd from ListenFlow : "<< Fd << std::endl;
				break;
			}

			if (verbose)
				std::cout << "Start new flow : "<< Fd << std::endl;

			std::thread t(&BaseServer::RunThread, this, Fd);
			t.detach();
		}

		if (WaitForClose) {
			if (verbose)
				std::cout << "Wait for running flows"<< std::endl;

			while (NumRunningThreads > 0) {
				sleep(1);
			}
		}

		if (verbose)
			std::cout << "Close server"<< std::endl;
		return AfterEnd();
	}

	void  BaseServer::RunThread(const int & Fd) {
		Mtx.lock();
		NumRunningThreads++;
		Mtx.unlock();

		int ReturnCode = HandleFlow(Fd);

		Mtx.lock();
		NumRunningThreads--;
		Mtx.unlock();

		AfterEndFlow(Fd, ReturnCode);

		close(Fd);
	}
}
