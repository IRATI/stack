//
// Echo Client
//
// Addy Bombeke <addy.bombeke@ugent.be>
// Vincenzo Maffione <v.maffione@nextworks.it>
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
//   1. Redistributions of source code must retain the above copyright
//      notice, this list of conditions and the following disclaimer.
//   2. Redistributions in binary form must reproduce the above copyright
//      notice, this list of conditions and the following disclaimer in the
//      documentation and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
// OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
// HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
// LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
// OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
// SUCH DAMAGE.
//

#include <csignal>
#include <cstring>
#include <cmath>
#include <deque>
#include <iostream>
#include <sstream>
#include <cassert>
#include <time.h>
#include <unistd.h>
#include <limits.h>
#include <iomanip>
#include <cerrno>

#define RINA_PREFIX "rina-echo-time"
#include <librina/ipc-api.h>
#include <librina/logs.h>

#include "et-client.h"

using namespace std;
using namespace rina;

void get_current_time(timespec& time) {
	clock_gettime(CLOCK_REALTIME, &time);
}

double time_difference_in_ms(timespec start, timespec end) {
	 double mtime, seconds, nseconds;

	 seconds  = (double)end.tv_sec  - (double)start.tv_sec;
	 nseconds = (double)end.tv_nsec - (double)start.tv_nsec;

	 mtime = ((seconds) * 1000 + nseconds/1000000.0);

	 return mtime;
}

class Signal {
	int signum;
	struct sigaction oldact;
public:
	Signal(int signum, sighandler_t handler) : signum(signum) {
		struct sigaction act;
		act.sa_handler = handler;
		sigemptyset(&act.sa_mask);
		act.sa_flags = 0;
		sigaction(signum, &act, &oldact);
	}
	~Signal() {
		sigaction(signum, &oldact, NULL);
	}
	static void dummy_handler(int) {}
};

class ReadSDUWithTimeout {
	int fd;
public:
	ReadSDUWithTimeout(int fd, unsigned timeout) {
		if (timeout) {
			long timeout2 = timeout;
			this->fd = -1;
			/* Timer with an interval of 1ms after the timeout is
			 * elapsed, in case that the first signal happens
			 * outside the syscall. */
			itimerval it = {
				{0, 1000},
				{timeout2 / 1000, timeout2 % 1000 * 1000}
			};
			setitimer(ITIMER_REAL, &it, NULL);
		} else {
                        int ret;
			this->fd = fd;
                        ret = fcntl(fd, F_SETFL, O_NONBLOCK);
                        if (ret) {
                                LOG_ERR("Failed to set O_NONBLOCK flag");
                        }
		}
	}
	~ReadSDUWithTimeout() {
		if (fd < 0) {
			itimerval it = { {0, 0}, {0, 0} };
			setitimer(ITIMER_REAL, &it, NULL);
		} else {
                        int ret;
                        ret = fcntl(fd, F_SETFL, 0);
                        if (ret) {
                                LOG_ERR("Failed to clear O_NONBLOCK flag");
                        }
		}
	}
};

Client::Client(const string& t_type,
               const list<string>& dif_nms, const string& apn, const string& api,
               const string& server_apn, const string& server_api,
               bool q, unsigned long count,
               bool registration, unsigned int size,
               int w, int g, int dw, unsigned int lw, int rt,
	       unsigned int delay_, unsigned int loss_) :
        Application(dif_nms, apn, api), test_type(t_type), dif_name(dif_nms.front()),
        server_name(server_apn), server_instance(server_api),
        quiet(q), echo_times(count),
        client_app_reg(registration), data_size(size), wait(w), gap(g),
        dealloc_wait(dw), lost_wait(lw), rate(rt),  delay(delay_), loss(loss_),
	snd(0), nsdus(0), m2(0), sdus_received(0), min_rtt(LONG_MAX), max_rtt(0),
	average_rtt(0), port_id(-1), fd(-1)
{
}

void Client::run()
{
        int ret;

	cout << setprecision(5);

        if (client_app_reg) {
                applicationRegister();
        }
        if (createFlow()) {
        	LOG_ERR("Problems creating flow, exiting");
        	return;
        }

	Signal alarm(SIGALRM, Signal::dummy_handler);

        ret = fcntl(fd, F_SETFL, 0);
        if (ret) {
                LOG_ERR("Failed to clear O_NONBLOCK flag");
        }

        if (test_type == "ping")
        	pingFlow();
        else if (test_type == "perf")
        	perfFlow();
        else if (test_type == "flood")
                floodFlow();
        else
        	LOG_ERR("Unknown test type '%s'", test_type.c_str());

        if (dealloc_wait > 0) {
        	sleep(dealloc_wait);
        }
        destroyFlow();
}

int Client::createFlow()
{
        timespec begintp, endtp;
        AllocateFlowRequestResultEvent* afrrevent;
        FlowSpecification qosspec;
        IPCEvent* event;
        uint seqnum;

        if (gap >= 0)
                qosspec.maxAllowableGap = gap;

        qosspec.msg_boundaries = true;
        qosspec.delay = delay;
        qosspec.loss = loss;

        get_current_time(begintp);

        if (dif_name != string()) {
                seqnum = ipcManager->requestFlowAllocationInDIF(
                        ApplicationProcessNamingInformation(app_name, app_instance),
                        ApplicationProcessNamingInformation(server_name, server_instance),
                        ApplicationProcessNamingInformation(dif_name, string()),
                        qosspec);
        } else {
                seqnum = ipcManager->requestFlowAllocation(
                        ApplicationProcessNamingInformation(app_name, app_instance),
                        ApplicationProcessNamingInformation(server_name, server_instance),
                        qosspec);
        }

        for (;;) {
                event = ipcEventProducer->eventWait();
                if (event && event->eventType == ALLOCATE_FLOW_REQUEST_RESULT_EVENT
                                && event->sequenceNumber == seqnum) {
                        break;
                }
                LOG_DBG("Client got new event %d", event->eventType);
        }

        afrrevent = dynamic_cast<AllocateFlowRequestResultEvent*>(event);

        rina::FlowInformation flow = ipcManager->commitPendingFlow(afrrevent->sequenceNumber,
                                              	      	           afrrevent->portId,
                                              	      	           afrrevent->difName);
        if (flow.portId < 0) {
                LOG_ERR("Failed to allocate a flow");
                return -1;
        } else {
                LOG_DBG("[DEBUG] Port id = %d", flow.portId);
        }

        get_current_time(endtp);

        if (!quiet) {
                cout << "Flow allocation time = "
                     << time_difference_in_ms(begintp, endtp) << " ms" << endl;
        }

        fd = flow.fd;
        port_id = flow.portId;

        return 0;
}

struct Ping {
	timespec begintp;
	unsigned long seq;
	void fill(unsigned char *buffer, unsigned data_size) {
		unsigned long n = seq;
		unsigned i = 0;
		do {
			if (i == data_size)
				return;
			buffer[i++] = (unsigned char) n;
		} while (n >>= 8);
		unsigned char c = (unsigned char) seq;
		while (i < data_size)
			buffer[i++] = ++c;
	}
	bool test(unsigned char *buffer, unsigned data_size) {
		unsigned long n = seq;
		unsigned i = 0;
		do {
			if (i == data_size)
				return true;
			if (buffer[i++] != (unsigned char) n)
				return false;
		} while (n >>= 8);
		unsigned char c = (unsigned char) seq;
		while (i < data_size) {
			if (buffer[i++] != ++c)
				return false;
		}
		return true;
	}
};

void Client::pingFlow()
{
	Ping ping;
	deque<Ping> sent;
	unsigned char *buffer = new unsigned char[data_size];
	unsigned int sdus_sent = 0;
	unsigned int sdus_received = 0;
	timespec endtp;
	//unsigned char counter = 0;
	double min_rtt = LONG_MAX;
	double max_rtt = 0;
	double average_rtt = 0;
	double m2 = 0;
	double delta = 0;
	double variance = 0;
	double stdev = 0;
	double current_rtt = 0;

	for (ping.seq = 0; ping.seq < echo_times; ping.seq++) {
		bool bad_response;
		int bytes_read = 0;
		Ping ping_aux;
		unsigned int index;
		int ret;

		ping.fill(buffer, data_size);
		get_current_time(ping.begintp);
		sent.push_front(ping);

		ret = write(fd, buffer, data_size);
		if (ret != (int)data_size) {
			if (errno == EAGAIN) {
				continue;
			}
			ostringstream oss;

			oss << "write() error: ";
			if (ret < 0) {
				oss << strerror(errno);
			} else {
				oss << "partial write " << ret <<
						"/" << data_size;
			}
			LOG_ERR("%s", oss.str().c_str());
			break;
		}

		sdus_sent ++;

		bool read_late = false;
read:
		bytes_read = readTimeout(buffer, data_size,
				read_late ? wait : lost_wait);

		if (bytes_read <= 0 && errno == EAGAIN) {
			if (!read_late) {
				LOG_WARN("Timeout waiting for reply, SDU maybe lost");
			}
			continue;
		}


		get_current_time(endtp);

		/* In case that SDUs are late (i.e. not lost), we kept track
		 * of all unanswered pings. This is important because the
		 * payload depends on the sequence number, and if we don't
		 * check for late pongs, we'd always get bad responses.
		 * However, in order not to complicate things too much, we
		 * assume replies are always ordered.
		 */
		index = 0;
		do{
			ping_aux = sent[index];
			if (!(bad_response = !ping_aux.test(buffer, data_size))){
				break;
			}
			++index;
		}while (sent.size() > index);

		if (bad_response){
			cout << "SDU [bad response]" << endl;
			if (ping.seq == echo_times - 1)
				break;
			read_late = true;
			goto read;
		}

		sdus_received ++;

		current_rtt = time_difference_in_ms(
				ping_aux.begintp, endtp);

		if (current_rtt < min_rtt) {
			min_rtt = current_rtt;
		}
		if (current_rtt > max_rtt) {
			max_rtt = current_rtt;
		}

		delta = current_rtt - average_rtt;
		average_rtt = average_rtt + delta/(double)sdus_received;
		m2 = m2 + delta*(current_rtt - average_rtt);

		cout << "SDU size = " << data_size << ", seq = "
				<< ping_aux.seq << ", RTT = " << current_rtt << " ms";
		if (index != 0)
			cout << " [REORDERED] "  << index << "   " << sent.size();
		cout << endl;

		if (ping.seq == echo_times - 1)
			break;

		sent.erase(sent.begin()+index);
		read_late = true;
		goto read;
	}

	variance = m2/((double)sdus_received -1);
	stdev = sqrt(variance);

	cout << "SDUs sent: "<< sdus_sent << "; SDUs received: " << sdus_received
			<< "; " << ((sdus_sent - sdus_received)*100/sdus_sent) << "% SDU loss"
			<< "; Minimum RTT: " << min_rtt << " ms; Maximum RTT: " << max_rtt
			<< " ms; Average RTT:" << average_rtt
			<< " ms; Standard deviation: " << stdev<<" ms"<<endl;

	delete [] buffer;
}

void Client::floodFlow()
{
	unsigned long sdus_sent = 0;
	double variance = 0, stdev = 0;
	timespec endtp, begintp, mtp; //, mintp
	unsigned long sn;
	double delta = 0;
	double current_rtt = 0;
	unsigned char *buffer2 = new unsigned char[max_buffer_size];

	snd = startSender();
	cout << "Sender started" << endl;
	while(true) {
		int bytes_read = 0;

		bytes_read = read(fd, buffer2, max_buffer_size);
                if (bytes_read < 0) {
                        if (errno == EAGAIN) {
                                continue;
                        }
                        ostringstream oss;

                        oss << "read() error: " << strerror(errno);
                        LOG_ERR("%s", oss.str().c_str());
                        break;
                }

		get_current_time(endtp);

		lock.lock();
		mtp = maxtp;
		lock.unlock();
		if (bytes_read <= 0) {
			LOG_WARN("Returned 0 bytes, SDU considered lost");
			double mtime = time_difference_in_ms(mtp, endtp);
			lock.lock();
			sdus_sent = nsdus;
			lock.unlock();
			if (mtime > lost_wait && (sdus_sent == echo_times)) {
				cout << "Experiment finished: " << mtime << endl;
				break;
			}

			continue;
		}

		memcpy(&sn, buffer2, sizeof(sn));
		lock.lock();
		sdus_received++;
		if (m.find(sn) != m.end()) {
			begintp = m[sn];
			m.erase(sn);
		}
		lock.unlock();
		current_rtt = time_difference_in_ms(begintp, endtp);
		if (current_rtt < min_rtt) {
			min_rtt = current_rtt;
		}
		if (current_rtt > max_rtt) {
			max_rtt = current_rtt;
		}

		delta = current_rtt - average_rtt;
		average_rtt = average_rtt + delta/(double)sdus_received;
		m2 = m2 + delta*(current_rtt - average_rtt);

		double mtime = time_difference_in_ms(mtp, endtp);
		lock.lock();
		sdus_sent = nsdus;
		lock.unlock();
		if (mtime > lost_wait ||
				(sdus_sent == echo_times && sdus_sent == sdus_received)) {
			break;
		}

	}

	lock.lock();
	sdus_sent = nsdus;
	lock.unlock();

	variance = m2/((double)sdus_received -1);
	stdev = sqrt(variance);

	unsigned long rt = 0;
	if (sdus_sent > 0) rt = ((sdus_sent - sdus_received)*100/sdus_sent);
	cout << "MAX delay: "<< delay << " ;SDUs sent: "<< sdus_sent << "; SDUs received: " << sdus_received
	     << "; " << rt << "% SDU loss"
	     << "Minimum RTT: " << min_rtt << " ms; Maximum RTT: " << max_rtt
			<< " ms; Average RTT:" << average_rtt
			<< " ms; Standard deviation: " << stdev<<" ms"<<endl;

	delete [] buffer2;
}

void Client::perfFlow()
{
        char *buffer;
        unsigned long n = 0;

        buffer = new char[data_size];
        if (!buffer) {
                LOG_ERR("Allocation of the send buffer failed");

                return;
        }

        for (unsigned int i = 0; i < data_size; i++) {
                buffer[i] = static_cast<char>(i * i);
        }
        memcpy(buffer, &delay, sizeof(delay));

        while (n < echo_times) {
        	int ret = write(fd, buffer, data_size);
                if (ret != (int)data_size) {
                        if (errno == EAGAIN) {
                                continue;
                        }
                        ostringstream oss;

                        oss << "write() error: ";
                        if (ret < 0) {
                                oss << strerror(errno);
                        } else {
                                oss << "partial write " << ret <<
                                        "/" << data_size;
                        }
                        LOG_ERR("%s", oss.str().c_str());
                }
                n++;
        }

        delete [] buffer;
}

void Client::destroyFlow()
{
        ipcManager->deallocate_flow(port_id);
        if (snd) delete snd;
}

int Client::readTimeout(void * sdu, int maxBytes, unsigned int timeout)
{
	timespec begintp, endtp;
	get_current_time(begintp);
	ReadSDUWithTimeout _(fd, timeout);
	do {
		int bytes_read = read(fd, sdu, maxBytes);
                if (bytes_read > 0 || errno == EAGAIN) {
                        return bytes_read;
                }
                if (errno != EINTR) {
                        ostringstream oss;

                        oss << "read() error: " << strerror(errno);
                        LOG_ERR("%s", oss.str().c_str());
                        return bytes_read;
                }
                /* XXX The ping application relies on the kernel returning EINTR,
                 * and changes the EINTR in EAGAIN (see below). I cannot understand
                 * what is the point of this. XXX THIS MUST CHANGE XXX. Why should
                 * the kernel return EINTR if there is no packet to read ???
                 * Apparently, for each write(), the ping application does a first
                 * read() that reads the response, and then another read() that
                 * returns EINTR, which is the used as an "indication" to proceed
                 * with the next write(). XXX XXX XXX
                 */
		get_current_time(endtp);
	} while ((unsigned) time_difference_in_ms(begintp, endtp) < timeout);

	errno = EAGAIN;
	return -1;
}

Sender * Client::startSender()
{
        Sender * sender = new Sender(echo_times,
                                     data_size,
                                     dealloc_wait,
                                     lost_wait,
                                     rate,
                                     port_id,
                                     fd,
                                     this);

        sender->start();
        sender->detach();

        return sender;
}

void Client::map_push(unsigned long sn, timespec tp)
{
	ScopedLock g(lock);
        m[sn] = tp;

}

void Client::set_sdus(unsigned long n)
{
	ScopedLock g(lock);
        nsdus = n;
}

void Client::set_maxTP(timespec tp)
{
	ScopedLock g(lock);
        maxtp = tp;
}

void Client::cancelFloodFlow()
{
        try {
        	ipcManager->deallocate_flow(port_id);
        } catch(rina::Exception &e) {
        	//Ignore, flow was already deallocated
        }
}

void Client::startCancelFloodFlowTask()
{
	cflood_task = new CFloodCancelFlowTimerTask(port_id, this);
	timer.scheduleTask(cflood_task, lost_wait);
}

Client::~Client()
{
}

CFloodCancelFlowTimerTask::CFloodCancelFlowTimerTask(int pid, Client * cl)
{
	port_id = pid;
	client = cl;
}

void CFloodCancelFlowTimerTask::run()
{
	client->cancelFloodFlow();
}

Sender::Sender(unsigned long echo_times,
                   unsigned int data_size,
                   int dealloc_wait,
                   unsigned int lost_wait,
                   int rt, int port_id,
                   int fd,
                   Client * client) : SimpleThread(std::string("sender"), false),
           echo_times(echo_times), data_size(data_size),
           dealloc_wait(dealloc_wait), lost_wait(lost_wait), rate(rt),
           port_id(port_id), fd(fd), client(client)
{
}

#ifndef MILLION
#define MILLION  1000000
#endif

#ifndef BILLION
#define BILLION  1000000000
#endif

void busy_wait_until(const struct timespec &deadline)
{
        timespec now;
        clock_gettime(CLOCK_REALTIME, &now);
        while (now.tv_sec < deadline.tv_sec)
                clock_gettime(CLOCK_REALTIME, &now);
        while (now.tv_sec == deadline.tv_sec && now.tv_nsec<deadline.tv_nsec)
                clock_gettime(CLOCK_REALTIME, &now);
}

/* add intv to t and store it in res*/
void ts_add(const timespec *t,
            const timespec *intv,
            timespec       *res)
{
        long nanos = 0;

        if (!(t && intv && res))
                return;

        nanos = t->tv_nsec + intv->tv_nsec;

        res->tv_sec = t->tv_sec + intv->tv_sec;
        while (nanos > BILLION) {
                nanos -= BILLION;
                ++(res->tv_sec);
        }
        res->tv_nsec = nanos;
}

int Sender::run(void)
{
	char buffer[data_size];
	unsigned int sdus_sent = 0;
	timespec begintp; //unused , endtp;
	timespec mintp, maxtp;
	// unused unsigned char counter = 0;
	double interval_time = 0;
	// unused bool out = false;

	if (rate) { /* wait in kB/s */
		interval_time = ((double) data_size) / ((double) rate); /* ms */
	}

	cout << "Experiment params" << endl;
	cout << "RATE: " << rate << endl;
	cout << "Interval: " << interval_time << endl;

	get_current_time(mintp);
	timespec next = mintp;
	for (unsigned long n = 0; n < echo_times; n++) {
                int ret;
		memcpy(buffer, &n, sizeof(n));

		get_current_time(begintp);
		ret = write(fd, buffer, data_size);
                if (ret != (int)data_size) {
                        if (errno == EAGAIN) {
                                continue;
                        }
                        ostringstream oss;

                        oss << "write() error: ";
                        if (ret < 0) {
                                oss << strerror(errno);
                        } else {
                                oss << "partial write " << ret <<
                                        "/" << data_size;
                        }
                        LOG_ERR("%s", oss.str().c_str());
                        break;
                }

		client->map_push(n, begintp);
		sdus_sent++;
		get_current_time(maxtp);
		client->set_maxTP(maxtp);
		client->set_sdus(sdus_sent);
		if (rate) {
			long nanos = interval_time * MILLION;
			timespec interval = {nanos / BILLION, nanos % BILLION};
			ts_add(&next,&interval,&next);
			busy_wait_until(next);
		}
	}
	cout << "ENDED sending. SDUs sent: "<< sdus_sent << "; in TIME: " <<
			time_difference_in_ms(mintp, maxtp) << " ms" <<endl;

	client->startCancelFloodFlowTask();

	return 0;
}
