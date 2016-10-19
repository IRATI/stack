//
// WS connector
//
// This code comes from:
// https://github.com/dhbaird/easywsclient
//
// To get the latest version:
// wget https://raw.github.com/dhbaird/easywsclient/master/easywsclient.hpp
// wget https://raw.github.com/dhbaird/easywsclient/master/easywsclient.cpp
//
// Modified by Micheal Crotty <mcrotty@tssg.org>
// Copyright (C) 2016, PRISTINE, Waterford Insitute of Technology
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

#ifdef _WIN32
    #if defined(_MSC_VER) && !defined(_CRT_SECURE_NO_WARNINGS)
        #define _CRT_SECURE_NO_WARNINGS // _CRT_SECURE_NO_WARNINGS for sscanf errors in MSVC2013 Express
    #endif
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #include <fcntl.h>
    #include <WinSock2.h>
    #include <WS2tcpip.h>
    #pragma comment( lib, "ws2_32" )
    #include <stdio.h>
    #include <stdlib.h>
    #include <string.h>
    #include <sys/types.h>
    #include <io.h>
    #ifndef _SSIZE_T_DEFINED
        typedef int ssize_t;
        #define _SSIZE_T_DEFINED
    #endif
    #ifndef _SOCKET_T_DEFINED
        typedef SOCKET socket_t;
        #define _SOCKET_T_DEFINED
    #endif
    #ifndef snprintf
        #define snprintf _snprintf_s
    #endif
    #if _MSC_VER >=1600
        // vs2010 or later
        #include <stdint.h>
    #else
        typedef __int8 int8_t;
        typedef unsigned __int8 uint8_t;
        typedef __int32 int32_t;
        typedef unsigned __int32 uint32_t;
        typedef __int64 int64_t;
        typedef unsigned __int64 uint64_t;
    #endif
    #define socketerrno WSAGetLastError()
    #define SOCKET_EAGAIN_EINPROGRESS WSAEINPROGRESS
    #define SOCKET_EWOULDBLOCK WSAEWOULDBLOCK
#else
    #include <fcntl.h>
    #include <netdb.h>
    #include <netinet/tcp.h>
    #include <stdio.h>
    #include <stdlib.h>
    #include <string.h>
    #include <sys/socket.h>
    #include <sys/time.h>
    #include <sys/types.h>
    #include <unistd.h>
    #include <stdint.h>
    #ifndef _SOCKET_T_DEFINED
        typedef int socket_t;
        #define _SOCKET_T_DEFINED
    #endif
    #ifndef INVALID_SOCKET
        #define INVALID_SOCKET (-1)
    #endif
    #ifndef SOCKET_ERROR
        #define SOCKET_ERROR   (-1)
    #endif
    #define closesocket(s) ::close(s)
    #include <errno.h>
    #define socketerrno errno
    #define SOCKET_EAGAIN_EINPROGRESS EAGAIN
    #define SOCKET_EWOULDBLOCK EWOULDBLOCK
#endif

#include <vector>
#include <string>
#include <atomic>
#include <iostream>
#include <assert.h>

#include "easywsclient.h"
#define RINA_PREFIX     "ws-client"
#include <librina/logs.h>
#include <librina/concurrency.h>


using easywsclient::Callback_Imp;
using easywsclient::BytesCallback_Imp;

namespace easywsclient { 

socket_t hostname_connect(const std::string& hostname, int port) {
    struct addrinfo hints;
    struct addrinfo *result;
    struct addrinfo *p;
    int ret;
    socket_t sockfd = INVALID_SOCKET;
    char sport[16];
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    snprintf(sport, 16, "%d", port);
    if ((ret = getaddrinfo(hostname.c_str(), sport, &hints, &result)) != 0)
    {
      LOG_ERR("getaddrinfo: %s", gai_strerror(ret));
      return 1;
    }
    for(p = result; p != NULL; p = p->ai_next)
    {
        sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (sockfd == INVALID_SOCKET) { continue; }
        if (connect(sockfd, p->ai_addr, p->ai_addrlen) != SOCKET_ERROR) {
            break;
        }
        closesocket(sockfd);
        sockfd = INVALID_SOCKET;
    }
    freeaddrinfo(result);
    return sockfd;
}


class _DummyWebSocket : public easywsclient::WebSocket
{
  public:
    void poll(int timeout) { }
    void send(const std::string& message) { }
    void sendBinary(const std::string& message) { }
    void sendBinary(const std::vector<uint8_t>& message) { }
    void sendPing() { }
    void close() { } 
    readyStateValues getReadyState() const { return CLOSED; }
    void _dispatch(Callback_Imp & callable) { }
    void _dispatchBinary(BytesCallback_Imp& callable) { }
};


class _RealWebSocket : public easywsclient::WebSocket
{
  public:
    // http://tools.ietf.org/html/rfc6455#section-5.2  Base Framing Protocol
    //
    //  0                   1                   2                   3
    //  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
    // +-+-+-+-+-------+-+-------------+-------------------------------+
    // |F|R|R|R| opcode|M| Payload len |    Extended payload length    |
    // |I|S|S|S|  (4)  |A|     (7)     |             (16/64)           |
    // |N|V|V|V|       |S|             |   (if payload len==126/127)   |
    // | |1|2|3|       |K|             |                               |
    // +-+-+-+-+-------+-+-------------+ - - - - - - - - - - - - - - - +
    // |     Extended payload length continued, if payload len == 127  |
    // + - - - - - - - - - - - - - - - +-------------------------------+
    // |                               |Masking-key, if MASK set to 1  |
    // +-------------------------------+-------------------------------+
    // | Masking-key (continued)       |          Payload Data         |
    // +-------------------------------- - - - - - - - - - - - - - - - +
    // :                     Payload Data continued ...                :
    // + - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - +
    // |                     Payload Data continued ...                |
    // +---------------------------------------------------------------+
    struct wsheader_type {
        unsigned header_size;
        bool fin;
        bool mask;
        enum opcode_type {
            CONTINUATION = 0x0,
            TEXT_FRAME = 0x1,
            BINARY_FRAME = 0x2,
            CLOSE = 8,
            PING = 9,
            PONG = 0xa,
        } opcode;
        int N0;
        uint64_t N;
        uint8_t masking_key[4];
    };

    std::vector<uint8_t> rxbuf;
    //std::vector<uint8_t> txbuf;
    rina::BlockingFIFOQueue<std::vector<uint8_t>> txQ;
    
    std::vector<uint8_t> receivedData;

    socket_t sockfd;
    std::atomic<readyStateValues> readyState;
    bool useMask;

    _RealWebSocket(socket_t sockfd, bool useMask) : sockfd(sockfd), readyState(OPEN), useMask(useMask) {
      rxbuf.reserve(1500);
      rxbuf.clear();
    }

    readyStateValues getReadyState() const {
      return readyState;
    }

    void poll(int timeout) { // timeout in milliseconds
        if (readyState == CLOSED) {
            if (timeout > 0) {
                timeval tv = { timeout/1000, (timeout%1000) * 1000 };
                select(0, NULL, NULL, NULL, &tv);
            }
            return;
        }
        std::vector<uint8_t>* txbuf = txQ.peek();
        
        // if (timeout != 0) {
            fd_set rfds;
            fd_set wfds;
            timeval tv = { timeout/1000, (timeout%1000) * 1000 };
            FD_ZERO(&rfds);
            FD_ZERO(&wfds);
            FD_SET(sockfd, &rfds);
            if (txbuf != nullptr) { FD_SET(sockfd, &wfds); }
            select(sockfd + 1, &rfds, &wfds, 0, timeout > 0 ? &tv : 0);
        // }
        //while (true) {
        if (FD_ISSET(sockfd, &rfds)) {
            // FD_ISSET(0, &rfds) will be true
            int N = rxbuf.size();
            ssize_t ret;
            rxbuf.resize(N + 1500);
            ret = recv(sockfd, (char*)&rxbuf[0] + N, 1500, MSG_DONTWAIT);
            if (ret < 0 && (socketerrno == SOCKET_EWOULDBLOCK || socketerrno == SOCKET_EAGAIN_EINPROGRESS)) {
                rxbuf.resize(N);
                //break;
            }
            else if (ret <= 0) {
                rxbuf.resize(N);
                closesocket(sockfd);
                readyState = CLOSED;
                fputs(ret < 0 ? "Connection error!\n" : "Connection closed!\n", stderr);
                //break;
            }
            else {
            //   if ((N == 0) && (ret == 1500)) {
            //     // Its some form of flush packet
            //     ret = 0; // discard it
            //   }
                rxbuf.resize(N + ret);
                //   std::cout << "N=" << N << ",ret=" << ret << std::endl;
                // }
                //break;
            }
        }
        // Is there something to transmit
        while ((txbuf != nullptr) && FD_ISSET(sockfd, &wfds)) {
            int ret = ::send(sockfd, (char*)&(*txbuf)[0], txbuf->size(), 0);
            if (ret < 0 && (socketerrno == SOCKET_EWOULDBLOCK || socketerrno == SOCKET_EAGAIN_EINPROGRESS)) {
                break;
            } else if (ret <= 0) {
                closesocket(sockfd);
                readyState = CLOSED;
                fputs(ret < 0 ? "Connection error!\n" : "Connection closed!\n", stderr);
                break;
            } else if (((std::vector<uint8_t>::size_type)ret) == txbuf->size()) {
              txQ.take();
              delete txbuf;
              txbuf = nullptr; //txQ.peek();
            } else {
              //std::cout << "Fragmented JSON" << std::endl;
              txbuf->erase(txbuf->begin(), txbuf->begin() + ret);
            }
        }
        
        // Check to see if we should be closing the websocket.
        readyStateValues st = readyState;        
        if (st == CLOSING) {
            closesocket(sockfd);
            readyState = CLOSED;
        }
    }

    // Callable must have signature: void(const std::string & message).
    // Should work with C functions, C++ functors, and C++11 std::function and
    // lambda:
    //template<class Callable>
    //void dispatch(Callable callable)
    virtual void _dispatch(Callback_Imp & callable) {
        struct CallbackAdapter : public BytesCallback_Imp
            // Adapt void(const std::string<uint8_t>&) to void(const std::string&)
        {
            Callback_Imp& callable;
            CallbackAdapter(Callback_Imp& callable) : callable(callable) { }
            void operator()(const std::vector<uint8_t>& message) {
                std::string stringMessage(message.begin(), message.end());
                callable(stringMessage);
            }
        };
        CallbackAdapter bytesCallback(callable);
        _dispatchBinary(bytesCallback);
    }

    virtual void _dispatchBinary(BytesCallback_Imp & callable) {
        // TODO: consider acquiring a lock on rxbuf...
        while (true) {
            wsheader_type ws;
            if (rxbuf.size() < 2) { return; /* Need at least 2 */ }
            const uint8_t * data = (uint8_t *) &rxbuf[0]; // peek, but don't consume
            ws.fin = (data[0] & 0x80) == 0x80;
            ws.opcode = (wsheader_type::opcode_type) (data[0] & 0x0f);
            ws.mask = (data[1] & 0x80) == 0x80;
            ws.N0 = (data[1] & 0x7f);
            ws.header_size = 2 + (ws.N0 == 126? 2 : 0) + (ws.N0 == 127? 8 : 0) + (ws.mask? 4 : 0);
            if (rxbuf.size() < ws.header_size) { return; /* Need: ws.header_size - rxbuf.size() */ }
            int i = 0;
            if (ws.N0 < 126) {
                ws.N = ws.N0;
                i = 2;
            }
            else if (ws.N0 == 126) {
                ws.N = 0;
                ws.N |= ((uint64_t) data[2]) << 8;
                ws.N |= ((uint64_t) data[3]) << 0;
                i = 4;
            }
            else if (ws.N0 == 127) {
                ws.N = 0;
                ws.N |= ((uint64_t) data[2]) << 56;
                ws.N |= ((uint64_t) data[3]) << 48;
                ws.N |= ((uint64_t) data[4]) << 40;
                ws.N |= ((uint64_t) data[5]) << 32;
                ws.N |= ((uint64_t) data[6]) << 24;
                ws.N |= ((uint64_t) data[7]) << 16;
                ws.N |= ((uint64_t) data[8]) << 8;
                ws.N |= ((uint64_t) data[9]) << 0;
                i = 10;
            }
            if (ws.mask) {
                ws.masking_key[0] = ((uint8_t) data[i+0]) << 0;
                ws.masking_key[1] = ((uint8_t) data[i+1]) << 0;
                ws.masking_key[2] = ((uint8_t) data[i+2]) << 0;
                ws.masking_key[3] = ((uint8_t) data[i+3]) << 0;
            }
            else {
                ws.masking_key[0] = 0;
                ws.masking_key[1] = 0;
                ws.masking_key[2] = 0;
                ws.masking_key[3] = 0;
            }
            if (rxbuf.size() < (ws.header_size+ws.N)) { return; /* Need: ws.header_size+ws.N - rxbuf.size() */ }
            
            // if ((rxbuf.size() > 0) && (ws.N == 0)) {
            //     std::cout << "Buffer incomplete at:" << std::endl;
            //     std::cout << "ws header size  =" << ws.header_size << std::endl;
            //     std::cout << "ws data size    =" << ws.N << std::endl;
            //     std::cout << "ws buf complete =" << ws.fin << std::endl;
            //     std::cout << "buffer size     =" << rxbuf.size() << std::endl;
            //     std::cout << "----" << std::endl;
            //     std::cout << "B:";
            //     std::vector<uint8_t>::size_type limit = std::max(((std::vector<uint8_t>::size_type)20), rxbuf.size());
            //     for (std::vector<uint8_t>::size_type q=0;q < limit;q++) {
            //       std::cout << " " << std::hex << rxbuf[q];
            //     } //<<std::hex<< std::string(begin(rxbuf), end(rxbuf)) << std::endl;              
            //     std::cout << "." << std::endl << "----" << std::endl;              
            // }

            // We got a whole message, now do something with it:
            if (ws.opcode == wsheader_type::CLOSE) { 
              close(); 
            } else if (ws.opcode == wsheader_type::PING) {
                if (ws.mask) {
                    LOG_DBG("Demasking");
                    for (size_t i = 0; i != ws.N; ++i) { 
                        rxbuf[i+ws.header_size] ^= ws.masking_key[i&0x3]; 
                    }
                }
              std::cout << "Frame[PING," << ws.fin << ",len=" << ws.N << "]" << std::endl;
              std::cout << "Frame data       =" << std::endl;
              std::vector<uint8_t>::size_type limit = std::max(((std::vector<uint8_t>::size_type)20), rxbuf.size());
              for (std::vector<uint8_t>::size_type q=0;q < limit;q++) {
                std::cout << std::hex << rxbuf[q];
              }             
              std::cout << "." << std::dec << std::endl << "----" << std::endl;
                std::string data(rxbuf.begin()+ws.header_size, rxbuf.begin()+ws.header_size+(size_t)ws.N);
                sendData(wsheader_type::PONG, data.size(), data.begin(), data.end());
            } else if (ws.opcode == wsheader_type::PONG) { 
              // Do nothing
                std::cout << "Frame[PONG," << ws.fin << ",len=" << ws.N << "]" << std::endl;
              
              std::cout << "Frame data       =" << std::endl;
              std::vector<uint8_t>::size_type limit = std::max(((std::vector<uint8_t>::size_type)20), rxbuf.size());
              for (std::vector<uint8_t>::size_type q=0;q < limit;q++) {
                std::cout << std::hex << rxbuf[q];
              }             
              std::cout << "." << std::dec  << std::endl << "----" << std::endl;
            // } else if ( !ws.fin || ws.opcode == wsheader_type::CONTINUATION) {
            //     if (ws.mask) { 
            //       for (size_t i = 0; i != ws.N; ++i) { 
            //         rxbuf[i+ws.header_size] ^= ws.masking_key[i&0x3]; 
            //       } 
            //     }
            //     // Add to the buffer
            //     if (ws.N > 0) {
            //       receivedData.insert(receivedData.end(), rxbuf.begin()+ws.header_size, rxbuf.begin()+ws.header_size+(size_t)ws.N);// just feed
            //     }
            //     std::cout << "Cont[" << ws.opcode << "," << ws.fin << "]" << std::endl;
            //     std::cout << "Dispatching buffer size=" << ws.header_size+(size_t)ws.N << std::endl;
            //     std::cout << "Dispatching data size  =" << receivedData.size() << std::endl;
            //     std::cout << "Dispatching data       =" << std::string(begin(receivedData),end(receivedData)) << std::endl;
                
            //     // Do nothing here a let the buffer increase
            //     //
            //     if (ws.fin) {
            //         callable((const std::vector<uint8_t>) receivedData);
            //         receivedData.erase(receivedData.begin(), receivedData.end());
            //         receivedData.clear();
            //     }
              } else if ( ws.opcode == wsheader_type::BINARY_FRAME ) {
                  // ignore it
                  std::cout << "Frame[BINARY," << ws.fin << ",len=" << ws.N << "]" << std::endl;
              
              } else if ( ws.opcode == wsheader_type::CONTINUATION 
                 || ws.opcode == wsheader_type::TEXT_FRAME 
                 /*|| ws.opcode == wsheader_type::BINARY_FRAME*/) {
                // De mask
                if (ws.mask) { 
                  for (size_t i = 0; i != ws.N; ++i) { 
                    rxbuf[i+ws.header_size] ^= ws.masking_key[i&0x3]; 
                  } 
                }

                std::cout << "Frame[" << ws.opcode << "," << ws.fin << ",len=" << ws.N << "]" << std::endl;

                // Add to the buffer
                if (ws.N > 0) {
                  receivedData.insert(receivedData.end(), rxbuf.begin()+ws.header_size, rxbuf.begin()+ws.header_size+(size_t)ws.N);// just feed
                  //  std::cout << "Dispatching buffer size=" << ws.header_size+(size_t)ws.N << std::endl;
                }
                                    
                if (ws.fin) {
                    std::cout << "Dispatching data size  =" << receivedData.size() << std::endl;
                    std::cout << "Dispatching data       =" << std::string(begin(receivedData),end(receivedData)) << std::endl;
                    callable((const std::vector<uint8_t>) receivedData);
                    //receivedData.erase(receivedData.begin(), receivedData.end());
                    receivedData.clear();
                    //std::vector<uint8_t> ().swap(receivedData);// free memory
                }
            } else { 
              std::cerr << "Warn: Got unexpected WebSocket message." << std::endl;
              std::cerr << "Discarding " << rxbuf.size() << " of buffer." << std::endl;
              std::cerr << "Discarding data       =" << std::endl;
              std::vector<uint8_t>::size_type limit = std::max(((std::vector<uint8_t>::size_type)20), rxbuf.size());
              for (std::vector<uint8_t>::size_type q=0;q < limit;q++) {
                std::cout << " " << std::hex << rxbuf[q];
              }             
              std::cout << "." << std::dec << std::endl << "----" << std::endl;
              // flush the buffer
              ws.N = 100;
              rxbuf.clear();
              //close();
            }
            

            // check for an overrun
            if (rxbuf.size() >= (ws.header_size+(size_t)ws.N)) {
              rxbuf.erase(rxbuf.begin(), rxbuf.begin() + ws.header_size+(size_t)ws.N);
              std::cout << "buffer size     =" << rxbuf.size() << std::endl;
            } else {
            //   std::cout << "Buffer incomplete at:" << std::endl;
            //   std::cout << "ws header size  =" << ws.header_size << std::endl;
            //   std::cout << "ws data size    =" << ws.N << std::endl;
            //   std::cout << "ws buf complete =" << ws.fin << std::endl;
            //   std::cout << "buffer size     =" << rxbuf.size() << std::endl;
            //   std::cout << "----" << std::endl;
            //   std::cout << "B:";
            //   std::vector<uint8_t>::size_type limit = std::max(((std::vector<uint8_t>::size_type)20), rxbuf.size());
            //   for (std::vector<uint8_t>::size_type q=0;q < limit;q++) {
            //     std::cout << " " << std::hex << rxbuf[q];
            //   }             
            //   std::cout << "." << std::endl << "----" << std::endl;
            }
        }
    }

    void sendPing() {
        std::string empty;
        sendData(wsheader_type::PING, empty.size(), empty.begin(), empty.end());
    }

    void send(const std::string& message) {
        sendData(wsheader_type::TEXT_FRAME, message.size(), message.begin(), message.end());
    }

    void sendBinary(const std::string& message) {
        sendData(wsheader_type::BINARY_FRAME, message.size(), message.begin(), message.end());
    }

    void sendBinary(const std::vector<uint8_t>& message) {
        sendData(wsheader_type::BINARY_FRAME, message.size(), message.begin(), message.end());
    }

    template<class Iterator>
    void sendData(wsheader_type::opcode_type type, uint64_t message_size, Iterator message_begin, Iterator message_end) {
        // TODO:
        // Masking key should (must) be derived from a high quality random
        // number generator, to mitigate attacks on non-WebSocket friendly
        // middleware:
        const uint8_t masking_key[4] = { 0x12, 0x34, 0x56, 0x78 };
        // TODO: consider acquiring a lock on txbuf...
        readyStateValues st = readyState;
        if (st == CLOSING || st == CLOSED) { return; }
        std::vector<uint8_t> header;
        header.assign(2 + (message_size >= 126 ? 2 : 0) + (message_size >= 65536 ? 6 : 0) + (useMask ? 4 : 0), 0);
        header[0] = 0x80 | type;
        if (false) { }
        else if (message_size < 126) {
            header[1] = (message_size & 0xff) | (useMask ? 0x80 : 0);
            if (useMask) {
                header[2] = masking_key[0];
                header[3] = masking_key[1];
                header[4] = masking_key[2];
                header[5] = masking_key[3];
            }
        }
        else if (message_size < 65536) {
            header[1] = 126 | (useMask ? 0x80 : 0);
            header[2] = (message_size >> 8) & 0xff;
            header[3] = (message_size >> 0) & 0xff;
            if (useMask) {
                header[4] = masking_key[0];
                header[5] = masking_key[1];
                header[6] = masking_key[2];
                header[7] = masking_key[3];
            }
        }
        else { // TODO: run coverage testing here
            header[1] = 127 | (useMask ? 0x80 : 0);
            header[2] = (message_size >> 56) & 0xff;
            header[3] = (message_size >> 48) & 0xff;
            header[4] = (message_size >> 40) & 0xff;
            header[5] = (message_size >> 32) & 0xff;
            header[6] = (message_size >> 24) & 0xff;
            header[7] = (message_size >> 16) & 0xff;
            header[8] = (message_size >>  8) & 0xff;
            header[9] = (message_size >>  0) & 0xff;
            if (useMask) {
                header[10] = masking_key[0];
                header[11] = masking_key[1];
                header[12] = masking_key[2];
                header[13] = masking_key[3];
            }
        }
        std::vector<uint8_t>* txbuf = new std::vector<uint8_t>();
        // N.B. - txbuf will keep growing until it can be transmitted over the socket:
        txbuf->insert(txbuf->end(), header.begin(), header.end());
        txbuf->insert(txbuf->end(), message_begin, message_end);
        if (useMask) {
            for (size_t i = 0; i != message_size; ++i) { 
              *(txbuf->end() - message_size + i) ^= masking_key[i&0x3];
            }
        }

        // Pop onto queue
        txQ.put(txbuf);
    }

    void close() {
      readyStateValues st = readyState;
      if (st == CLOSING || st == CLOSED) { return; }
      readyState = CLOSING;
      uint8_t closeFrame[6] = {0x88, 0x80, 0x00, 0x00, 0x00, 0x00}; // last 4 bytes are a masking key
      std::vector<uint8_t> header(closeFrame, closeFrame+6);
      std::vector<uint8_t>* txbuf = new std::vector<uint8_t>();
      txbuf->insert(txbuf->end(), header.begin(), header.end());
      // Pop onto queue
      txQ.put(txbuf);
    }

};


easywsclient::WebSocket::pointer from_url(const std::string& url, bool useMask, const std::string& origin) {
    char host[128];
    int port;
    char path[128];
    if (url.size() >= 128) {
      fprintf(stderr, "ERROR: url size limit exceeded: %s\n", url.c_str());
      return NULL;
    }
    if (origin.size() >= 200) {
      fprintf(stderr, "ERROR: origin size limit exceeded: %s\n", origin.c_str());
      return NULL;
    }
    if (false) { }
    else if (sscanf(url.c_str(), "ws://%[^:/]:%d/%s", host, &port, path) == 3) {
    }
    else if (sscanf(url.c_str(), "ws://%[^:/]/%s", host, path) == 2) {
        port = 80;
    }
    else if (sscanf(url.c_str(), "ws://%[^:/]:%d", host, &port) == 2) {
        path[0] = '\0';
    }
    else if (sscanf(url.c_str(), "ws://%[^:/]", host) == 1) {
        port = 80;
        path[0] = '\0';
    }
    else {
        fprintf(stderr, "ERROR: Could not parse WebSocket url: %s\n", url.c_str());
        return NULL;
    }
    fprintf(stderr, "easywsclient: connecting: host=%s port=%d path=/%s\n", host, port, path);
    socket_t sockfd = hostname_connect(host, port);
    if (sockfd == INVALID_SOCKET) {
        fprintf(stderr, "Unable to connect to %s:%d\n", host, port);
        return NULL;
    }
    {
        // XXX: this should be done non-blocking,
        char line[256];
        int status;
        int i;
        snprintf(line, 256, "GET /%s HTTP/1.1\r\n", path); ::send(sockfd, line, strlen(line), 0);
        if (port == 80) {
            snprintf(line, 256, "Host: %s\r\n", host); ::send(sockfd, line, strlen(line), 0);
        }
        else {
            snprintf(line, 256, "Host: %s:%d\r\n", host, port); ::send(sockfd, line, strlen(line), 0);
        }
        snprintf(line, 256, "Upgrade: websocket\r\n"); ::send(sockfd, line, strlen(line), 0);
        snprintf(line, 256, "Connection: Upgrade\r\n"); ::send(sockfd, line, strlen(line), 0);
        if (!origin.empty()) {
            snprintf(line, 256, "Origin: %s\r\n", origin.c_str()); ::send(sockfd, line, strlen(line), 0);
        }
        snprintf(line, 256, "Sec-WebSocket-Key: x3JJHMbDL1EzLkh9GBhXDw==\r\n"); ::send(sockfd, line, strlen(line), 0);
        snprintf(line, 256, "Sec-WebSocket-Version: 13\r\n"); ::send(sockfd, line, strlen(line), 0);
        snprintf(line, 256, "\r\n"); ::send(sockfd, line, strlen(line), 0);
        for (i = 0; i < 2 || (i < 255 && line[i-2] != '\r' && line[i-1] != '\n'); ++i) { if (recv(sockfd, line+i, 1, 0) == 0) { return NULL; } }
        line[i] = 0;
        if (i == 255) { fprintf(stderr, "ERROR: Got invalid status line connecting to: %s\n", url.c_str()); return NULL; }
        if (sscanf(line, "HTTP/1.1 %d", &status) != 1 || status != 101) { fprintf(stderr, "ERROR: Got bad status connecting to %s: %s", url.c_str(), line); return NULL; }
        // TODO: verify response headers,
        while (true) {
            for (i = 0; i < 2 || (i < 255 && line[i-2] != '\r' && line[i-1] != '\n'); ++i) { if (recv(sockfd, line+i, 1, 0) == 0) { return NULL; } }
            if (line[0] == '\r' && line[1] == '\n') { break; }
        }
    }
    int flag = 1;
    setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, (char*) &flag, sizeof(flag)); // Disable Nagle's algorithm
#ifdef _WIN32
    u_long on = 1;
    ioctlsocket(sockfd, FIONBIO, &on);
#else
    fcntl(sockfd, F_SETFL, O_NONBLOCK);
#endif
    fprintf(stderr, "Connected to: %s\n", url.c_str());
    return easywsclient::WebSocket::pointer(new _RealWebSocket(sockfd, useMask));
}

} // end of module-only namespace



namespace easywsclient {

WebSocket::pointer WebSocket::create_dummy() {
    static pointer dummy = pointer(new _DummyWebSocket);
    return dummy;
}


WebSocket::pointer WebSocket::from_url(const std::string& url, const std::string& origin) {
    return easywsclient::from_url(url, true, origin);
}

WebSocket::pointer WebSocket::from_url_no_mask(const std::string& url, const std::string& origin) {
    return easywsclient::from_url(url, false, origin);
}


} // namespace easywsclient
