#include <chrono>
#include <cstring>
#include <string>
#include <iostream>
#include <functional>
#include <thread>
#include <random>
#include <librina/librina.h>

using namespace std;
using namespace rina;

typedef unsigned int uint;

struct Options;

int main(int argc, char** argv);
int runServer(const Options& o);
void runServerFlow(Flow* flow);
int runClient(const Options& o);
void printDuration(const std::chrono::high_resolution_clock::duration& dur);

const uint maxsize = 1024;
const std::chrono::seconds seconde(1);
const std::chrono::seconds dseconde(9);
const std::chrono::milliseconds dmilliseconde(9);
const std::chrono::microseconds dmicroseconde(9);

struct Options
{
	string serverappname;
	string serverinstance;
	string clientappname;
	string clientinstance;
	FlowSpecification qosspec;
	bool server;
	bool checkflowcreationtime;
	ulong echotimes;//-1 is infinite, count
	bool printotherevents;
	bool registerclient;
	uint datasize;
	bool showhelp;
	Options()
	{
		serverappname = "rina.apps.echotime.server";
		serverinstance = "1";
		clientappname = "rina.apps.echotime.client";
		clientinstance = "";

		server = false;
		checkflowcreationtime = false;
		echotimes = static_cast<ulong>(-1);
		printotherevents = false;
		registerclient = false;
		showhelp = false;
		datasize = 64;
	}
};

int main(int argc, char** argv)
{
	initialize("DBG", "testprog.log");
	Options o;
	for(int i = 1; i < argc; i++)
	{
		if(argv[i][0] == '-')
		{
			if(argv[i][1] == '-')
			{
			}
			else if(argv[i][1] == 'c')
			{
				if(i + 1 < argc)
					o.echotimes = strtoul(argv[i + 1], nullptr, 0);
				i++;
			}
			else if(argv[i][1] == 's')
			{
				if(i + 1 < argc)
					o.datasize = min(maxsize, static_cast<uint>(strtoul(argv[i + 1], nullptr, 0)));
				i++;
			}
			else
			{
				uint j = 1;
				while(argv[i][j] != '\0')
				{
					switch(argv[i][j])
					{
						case 'l':
							o.server = true;
							break;
						case 'L':
							o.server = false;
							break;
						case 'x':
							o.printotherevents = true;
							break;
						case 'X':
							o.printotherevents = false;
							break;
						case 'r':
							o.registerclient = true;
							break;
						case 'R':
							o.registerclient = false;
							break;
						case 'f':
							o.checkflowcreationtime = true;
							break;
						case 'F':
							o.checkflowcreationtime = false;
							break;
						case 'h':
							o.showhelp = true;
							break;
						default:
							break;
					}
					j++;
				}
			}
		}
		else
		{
			o.serverinstance = argv[i];
		}
	}
	if(o.showhelp)
	{
		cout << "./rinaechotime [OPTIONS] [SERVER_INSTANCE]" << endl
			 << "Options: \th\tshow help" << endl
			 << "\t\tr\tregister the client" << endl
			 << "\t\tx\tprint debug messages" << endl
			 << "\t\tl\trun this as a server" << endl
			 << "\t\tf\ttime flow creation" << endl;
	}
	else
	{
		if(o.server)
		{
			return runServer(o);
		}
		else
		{
			return runClient(o);
		}
	}
	return 0;
}

int runServer(const Options& o)
{
	{
		//init
		ApplicationRegistrationInformation ari(ApplicationRegistrationType::APPLICATION_REGISTRATION_ANY_DIF);
		ari.setApplicationName(ApplicationProcessNamingInformation(o.serverappname, o.serverinstance));
		try
		{
			ipcManager->requestApplicationRegistration(ari);
		}
		catch(ApplicationRegistrationException e)
		{
			cerr << e.what() << endl;
			return -1;
		}
	}
	for(;;)
	{
		IPCEvent* event = ipcEventProducer->eventWait();
		if(!event)
			return -2;
		switch(event->getType())
		{
			case REGISTER_APPLICATION_RESPONSE_EVENT:
				ipcManager->commitPendingResitration(event->getSequenceNumber(), reinterpret_cast<RegisterApplicationResponseEvent*>(event)->getDIFName());
				break;
			case UNREGISTER_APPLICATION_RESPONSE_EVENT:
				ipcManager->appUnregistrationResult(event->getSequenceNumber(), reinterpret_cast<UnregisterApplicationResponseEvent*>(event)->getResult() == 0);
				break;
			case FLOW_ALLOCATION_REQUESTED_EVENT:
			{
				Flow* flow = ipcManager->allocateFlowResponse(*reinterpret_cast<FlowRequestEvent*>(event), 0, true);
				thread t(runServerFlow, flow);
				t.detach();
				break;
			}
			case FLOW_DEALLOCATED_EVENT:
				ipcManager->requestFlowDeallocation(reinterpret_cast<FlowDeallocatedEvent*>(event)->getPortId());
				break;
			default:
				if(o.printotherevents)
					cerr << "[DEBUG] Server got new event " << event->getType() << endl;
				break;
		}
	}
	return 0;
}

void runServerFlow(Flow* flow)
{
	char buffer[maxsize];
	try
	{
		for(;;)
		{
			int bytesreaded = flow->readSDU(buffer, maxsize);
			flow->writeSDU(buffer, bytesreaded);
		}
	}
	catch(...)
	{
		//flow I/O fail
	}
}

int runClient(const Options& o)
{
	if(o.registerclient)
	{
		//init
		ApplicationRegistrationInformation ari(ApplicationRegistrationType::APPLICATION_REGISTRATION_ANY_DIF);
		ari.setApplicationName(ApplicationProcessNamingInformation(o.clientappname, o.clientinstance));
		try
		{
			ipcManager->requestApplicationRegistration(ari);
		}
		catch(ApplicationRegistrationException e)
		{
			cerr << e.what() << endl;
			return -1;
		}
	}
	Flow* flow = nullptr;
	{
		std::chrono::high_resolution_clock::time_point begintp = std::chrono::high_resolution_clock::now();
		uint handle = ipcManager->requestFlowAllocation(ApplicationProcessNamingInformation(o.clientappname, o.clientinstance), ApplicationProcessNamingInformation(o.serverappname, o.serverinstance), o.qosspec);
		IPCEvent* event = ipcEventProducer->eventWait();
		while(event && event->getType() != ALLOCATE_FLOW_REQUEST_RESULT_EVENT)
		{
			if(o.printotherevents)
				cerr << "[NIET BEHANDELEDE EVENT] " << event->getType() << endl;
			event = ipcEventProducer->eventWait();//todo this can make us wait forever
		}
		AllocateFlowRequestResultEvent* afrrevent =	  reinterpret_cast<AllocateFlowRequestResultEvent*>(event);
		Flow* hulpflow = ipcManager->commitPendingFlow(afrrevent->getSequenceNumber(), afrrevent->getPortId(), afrrevent->getDIFName());
		if(hulpflow->getPortId() == -1)
		{
			cerr << "Host not found" << endl;
			return -3;
		}
		else
		{
			flow = hulpflow;
		}
		std::chrono::high_resolution_clock::time_point eindtp = std::chrono::high_resolution_clock::now();
		std::chrono::high_resolution_clock::duration dur = eindtp - begintp;
		if(o.checkflowcreationtime)
		{
			cout << "flow_creation_time=";
			printDuration(dur);
			cout << endl;
		}
	}
	char buffer[maxsize];
	char buffer2[maxsize];
	ulong n = 0;
	ulong c = o.echotimes;
	random_device rd;
	default_random_engine ran(rd());
	uniform_int_distribution<int> dis(0, 255);
	while(flow)
	{
		IPCEvent* event = ipcEventProducer->eventPoll();
		if(event)
		{
			switch(event->getType())
			{
				case FLOW_DEALLOCATED_EVENT:
					flow = nullptr;
					break;
				default:
					if(o.printotherevents)
						cerr << "[DEBUG] Client got new event " << event->getType() << endl;
					break;
			}
		}
		else
		{
			if(c != 0)
			{
				for(uint i = 0; i < o.datasize; ++i)
					buffer[i] = dis(ran);
				std::chrono::high_resolution_clock::time_point begintp = std::chrono::high_resolution_clock::now();
				int bytesreaded;
				try
				{
					flow->writeSDU(buffer, o.datasize);
					bytesreaded = flow->readSDU(buffer2, o.datasize);
				}
				catch(...)
				{
					//flow I/O fail
					return -4;
				}
				std::chrono::high_resolution_clock::time_point eindtp = std::chrono::high_resolution_clock::now();
				std::chrono::high_resolution_clock::duration dur = eindtp - begintp;
				cout << "sdu_size=" << o.datasize << " seq=" << n  << " time=";
				printDuration(dur);
				if(!((o.datasize == bytesreaded) && (memcmp(buffer, buffer2, bytesreaded) == 0)))
					cout << " bad check";
				cout << endl;
				n++;
				if(c != static_cast<ulong>(-1))
					c--;
				this_thread::sleep_for(seconde);
			}
			else
				flow = nullptr;
		}
	}
	return 0;
}

void printDuration(const std::chrono::high_resolution_clock::duration& dur)
{
	if(dur > dseconde)
		cout << std::chrono::duration_cast<std::chrono::seconds>(dur).count() << "s";
	else if(dur > dmilliseconde)
		cout << std::chrono::duration_cast<std::chrono::milliseconds>(dur).count() << "ms";
	else if(dur > dmicroseconde)
		cout << std::chrono::duration_cast<std::chrono::microseconds>(dur).count() << "us";
	else
		cout << std::chrono::duration_cast<std::chrono::nanoseconds>(dur).count() << "ns";
}
