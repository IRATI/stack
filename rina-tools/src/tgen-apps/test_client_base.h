#pragma once

#include <thread>
#include <chrono>
#include <time.h>

#include "ra_commons.h"
#include "ra_base_client.h"
#include "test_commons.h"

class TestClientBase : public ra::BaseClient {
public:
	TestClientBase(const std::string Name, const std::string Instance, const std::string Servername,
		       const std::string ServerInstance, const std::string DIF,
		       int TestDuration, bool verbose,
		       size_t mpdus, unsigned int loss, unsigned int delay, int FlowIdent, int QoSIdent) :
		BaseClient(Name, Instance, Servername, ServerInstance, DIF, verbose, loss, delay) {
		_TestDuration = TestDuration;
        _FlowIdent = FlowIdent;
        _QoSIdent = QoSIdent;

		AllocTimeoutMs = TIMEOUT_MS;

		max_sdu_size = mpdus;
		_Fd = 0;
	}

	virtual ~TestClientBase(){};

protected:
	std::chrono::time_point<std::chrono::system_clock> Endtime;

	virtual int HandleFlow(const int Fd) {
		_Fd = Fd;
		srand(time(0));
		int reties = 2;

		InitData.QoSId = _QoSIdent;
		InitData.FlowId = _FlowIdent;

		InitData.Size = sizeof(initSDU);
		InitData.Flags = SDU_FLAG_INIT;
		InitData.SeqId = 0;
		InitData.SendTime = 0;

		int res = SendSpecialSDU(Fd,sizeof(initSDU),reties);
		if (res == -1){
			std::cerr << "Failure sending initial SDU" << std::endl;
			return -1;
		}else if (res == -2){
			std::cerr << "No data echo received for the initial SDU" << std::endl;
			return -1;
		}

		Data.Flags = 0;
		Data.SeqId = 0;

		Endtime = std::chrono::system_clock::now() + std::chrono::seconds(_TestDuration);
		int ReturnCode = RunFlow();

		Data.Flags = SDU_FLAG_FIN;
		res = SendSpecialSDU(Fd,sizeof(dataSDU), reties);
		if (res == -1){
			std::cerr << "Failure sending fin SDU" << std::endl;
			return -1;
		}else if (res == -2){
			std::cerr << "No data echo received for the fin SDU" << std::endl;
			return -1;
		}

		return ReturnCode;
	}

	int SendSpecialSDU(const int Fd,size_t size, int retries) {
		retries --;
		if (SendData(size, TIMEOUT_MS) != size) {
			std::cerr << "Failure sending control SDU" << std::endl;
			return -1;
		}

		if (ra::ReadDataTimeout(Fd, readBuffer, TIMEOUT_MS) <= 0) {
			if  (retries > 0){
				std::cerr << "Trying to send again SDU" << std::endl;
				return SendSpecialSDU(Fd, size, retries);
			}else{
				std::cerr << "No data echo received for the fin SDU" << std::endl;
				return -2;
			}
		}
		return 0;
	}

	int SendData(size_t Size, int mSec = -1) {
		Data.Size = Size;
		Data.SeqId++;
		Data.SendTime =
			std::chrono::duration_cast<std::chrono::microseconds>(
				std::chrono::high_resolution_clock::now().time_since_epoch()).count();
		if (mSec < 0) {
			return ra::WriteData(_Fd, Buffer, Size, max_sdu_size);
		} else {
			return ra::WriteDataTimeout(_Fd, Buffer, Size, max_sdu_size, mSec);
		}
	}

	virtual int RunFlow() { return 0;  };

private:
	int _TestDuration;
    int _FlowIdent;
    int _QoSIdent;
	int _Fd;
	union {
		ra::byte_t Buffer[BUFF_SIZE];
		dataSDU Data;
		initSDU InitData;
	};
	ra::byte_t readBuffer[BUFF_SIZE];
	size_t max_sdu_size;
};
