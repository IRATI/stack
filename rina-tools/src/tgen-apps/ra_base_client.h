#pragma once


#include <string>
#include <vector>
#include <thread>
#include <mutex>

#include "ra_commons.h"

namespace ra {
	class BaseClient {
	public:
		BaseClient(const std::string Name, const std::string Instance, const std::string Servername,
			   const std::string ServerInstance, const std::string DIF, bool verb,
			   unsigned int loss, unsigned int delay);
		virtual ~BaseClient(){};

		int Run();

	protected:
		int AllocTimeoutMs;

		std::string MyName;
		std::string DstName;
		std::string DIFName;
		struct rina_flow_spec FlowSpec;
		bool verbose;

		virtual int HandleFlow(const int Fd) { return 0; }
		virtual int AllocFailed(const int ReturnCode) { return -1; };
		virtual int AfterEndFlow(const int ReturnCode) { return ReturnCode; };
	};

	BaseClient::BaseClient(const std::string Name, const std::string Instance, const std::string ServerName,
			       const std::string ServerInstance, const std::string DIF, bool verb,
			       unsigned int loss, unsigned int delay) {
		MyName = Name + "|" + Instance;
		DstName = ServerName + "|" + ServerInstance;
		rina_flow_spec_unreliable(&FlowSpec);
		verbose = verb;
		DIFName = DIF;

		FlowSpec.max_delay = delay;
		FlowSpec.max_loss = loss;

		AllocTimeoutMs = -1;
	}

	int BaseClient::Run() {
		if (verbose)
			std::cout << "BaseClient Run()"<< std::endl;

		int Fd = AllocFlow(
			MyName.c_str(),
			DstName.c_str(),
			&FlowSpec,
			AllocTimeoutMs,
			(DIFName == "" ? NULL : DIFName.c_str())
		);

		if (Fd <= 0) {
			std::cout << "Failure allocating flow"<< std::endl;
			return AllocFailed(Fd);
		}

		if (verbose)
			std::cout << "AllocFlow() success : " << Fd
				  << " | start flow"<< std::endl;

		int ReturnCode = HandleFlow(Fd);
		close(Fd);
		return AfterEndFlow(ReturnCode);
	}
}
