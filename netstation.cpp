/* 
 The MIT License
 
 Copyright (c) 2008 Andrew Butcher
 
 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:
 
 The above copyright notice and this permission notice shall be included in
 all copies or substantial portions of the Software.
 
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 THE SOFTWARE. 
 */

#include <iostream>
#include <stdexcept>
#include <cstring>
#include <cassert>

#if defined(_WIN32) || defined(_WIN64)
// #include <winsock2.h> // Already included from header for SOCKET type
#else
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#endif

#include "netstation.h"

namespace NetStation {

	// NetStation supports "NTEL", "UNIX" and "MAC-", but "UNIX" and "MAC-" are both indicators for big endian
	const char kLittleEndian[4]	= {'N','T','E','L'};	// Inform netstation that data is in little endian format
	const char kBigEndian[4]	= {'U','N','I','X'};	// Inform netstation that data is in big endian format

    // Start of with a disconnected socket
    Socket::Socket() : m_socket(0) { 
    }
    
    // And close our socket when we go out of scope
    Socket::~Socket() { 
        this->disconnect(); 
    }
    
    // Handle the connection sequence to NetStation
    bool Socket::connect(const char *address, unsigned short port) {
		int noDelay = 1;
        bool didConnect = false;
        struct sockaddr_in destination;

        // If we're already connected, disconnect cleanly first
        this->disconnect();

		#if defined(_WIN32) || defined(_WIN64)
		WSADATA wsaData;	
		if(WSAStartup(MAKEWORD(1,1),&wsaData) != 0){
			std::cerr << "WSAStartup failed." << std::endl;
			exit(1);
		}
		#endif

        try {
            // Try to obtain a socket from the OS, if we fail, throw an exception.
            m_socket = socket(PF_INET, SOCK_STREAM, 0);
            if (m_socket == 0) throw std::runtime_error("socket");
		
			// Disable Nagleing for faster transmission... Increases amount of data being sent per moment in time, in exchange for lower latency
			// If this fails, who cares?
			setsockopt(m_socket, IPPROTO_TCP, TCP_NODELAY, (char*)&noDelay, sizeof(int));
		
            // Setup the destination for our socket
            destination.sin_family = AF_INET;
            destination.sin_port = htons(port);
            destination.sin_addr.s_addr = inet_addr(address);
            memset(destination.sin_zero, '\0', sizeof(destination.sin_zero));
        
            // Try to connect... if there is no one "listen"ing on the other end this will fail.
            int result = ::connect(m_socket, (struct sockaddr*) &destination, sizeof(destination));
            if (result != 0) throw std::runtime_error("connect");
        
            // We connected!
            didConnect = true;
			
        }
        // Cleanup after ourselves if an error occurred
        catch (std::runtime_error& problem) {
            std::cerr << problem.what() << std::endl;
            this->disconnect();
            didConnect = false;
        }
            
        return didConnect;
    }
        
    // Cleanup after ourselves. The destructor will call this for us if we forget to, but we should do it.
    void Socket::disconnect() {
        if (m_socket != 0) {
			#if defined(_WIN32) || defined(_WIN64)
			closesocket(m_socket);
			WSACleanup();
			#else
            close(m_socket);
			#endif
			m_socket = 0;
        }
    }

    
    // The C function "send" may not send all of the data passed to it. Loop until everything is away or an error occurs.
    size_t Socket::sendComplete(const char *data, size_t dataSize) const {
        int dataSent = 0;
        int errorCode = 0;
        
        while (size_t(dataSent) < dataSize) {
            errorCode = ::send(m_socket, &data[dataSent], int(dataSize - dataSent), 0);
            if (errorCode > 0) dataSent += errorCode;
            else break;
        }
        
        return dataSent;
    }
    

    // The C function "recv" may not recv all of the data sent to it. Loop until everything is here or an error occurs.
    size_t Socket::recvComplete(char *data, size_t dataSize) const {
        int dataRecv = 0;
        int errorCode = 0;
    
        while (size_t(dataRecv) < dataSize) {
            errorCode = ::recv(m_socket, &data[dataRecv], int(dataSize - dataRecv), 0);
            if (errorCode > 0) dataRecv += errorCode;
            else break;
        }
        
        return dataRecv;
    }


    /////////////////////////////////////////////////////////////////////////////////////////////////
    
    
    // SocketEx
    const char SocketEx::kQuery           = 'Q';
    const char SocketEx::kExit            = 'X';
    const char SocketEx::kBeginRecording  = 'B';
    const char SocketEx::kEndRecording    = 'E';
    const char SocketEx::kAttention       = 'A';
    const char SocketEx::kTimeSynch       = 'T';
    const char SocketEx::kEventDataStream = 'D';
    
    const char SocketEx::kQuerySuccess    = 'I';
    const char SocketEx::kSuccess         = 'Z';
    const char SocketEx::kFailure         = 'F';

    bool SocketEx::sendCommand(const char *command, const size_t commandSize) const {
        // Assume the command didn't get through
        bool didSendCommand = false;
        
        // The response message we receive from netstation
        char responseCode = 0;
        
        // Query commands respond with a one-byte version of the protocol
        char responseVersion = 0;
        
        // Failure response with an error code
        short responseError = 0;

        try {
            // 2) Send any accompanying data, if any
            if (command != 0) {
                if (this->sendComplete(command, commandSize) != commandSize) {
                    throw std::runtime_error("sendComplete");
                }
            }
            
            // 3) Receive the response code
			if (this->recvComplete(&responseCode, sizeof(responseCode)) != sizeof(responseCode)) {
                throw std::runtime_error("recvComplete: responseCode");
            }

            // 4) Receive any additional response data
            if (responseCode == kQuerySuccess) {
                if (this->recvComplete((char*)&responseVersion, sizeof(responseVersion)) != sizeof(responseVersion)) {
                    throw std::runtime_error("recvComplete: responseVersion");
                }
            }
            else if (responseCode == kFailure) {
				if (this->recvComplete((char*)&responseError, sizeof(responseError)) != sizeof(responseError)) {
                    throw std::runtime_error("recvComplete: responseError");
                }
            }
			else if (responseCode != kSuccess) {
				throw std::runtime_error("responseCode invalid"); // We should only see this if the NetStation protocol changes....
			}
            didSendCommand = true;
        }
        catch (std::runtime_error& problem) {
			std::cerr << problem.what() << std::endl;
        }
        
        return didSendCommand;
    }

    bool SocketEx::sendBeginSession(const char systemSpec[4]) {
        size_t offset = 0;
        
        this->m_commandBuffer[offset] = kQuery;
        offset += sizeof(kQuery);
        
        memcpy(&this->m_commandBuffer[offset], systemSpec, sizeof(systemSpec));
        offset += sizeof(systemSpec);
        
        return this->sendCommand(&this->m_commandBuffer[0], offset);
    }
    
    bool SocketEx::sendEndSession() const {
        return this->sendCommand(&kExit, sizeof(kExit));
    }

    bool SocketEx::sendBeginRecording() const {
        return this->sendCommand(&kBeginRecording, sizeof(kBeginRecording));
    }
    
    bool SocketEx::sendEndRecording() const {
        return this->sendCommand(&kEndRecording, sizeof(kEndRecording));
    }
    
    bool SocketEx::sendAttention() const {
        return this->sendCommand(&kAttention, sizeof(kAttention));
    }
    
    bool SocketEx::sendSynch(long timeStamp) {
        size_t offset = 0;
		        
        this->m_commandBuffer[offset] = kTimeSynch;
        offset += sizeof(kTimeSynch);
   
        memcpy(&this->m_commandBuffer[offset], &timeStamp, sizeof(timeStamp));
        offset += sizeof(timeStamp);
		
        return this->sendCommand(&this->m_commandBuffer[0], offset);
    }

    bool SocketEx::sendTrigger(const char* code, long timeStamp, long msDuration) {
        size_t offset = 0;
		
        this->m_commandBuffer[offset] = kEventDataStream;
        offset += sizeof(kEventDataStream);
	
		unsigned short dataSize = 25;
		memcpy(&this->m_commandBuffer[offset], &dataSize, sizeof(dataSize));
		offset += sizeof(dataSize);

		memcpy(&this->m_commandBuffer[offset], &timeStamp, sizeof(timeStamp));
		offset += sizeof(timeStamp);

		memcpy(&this->m_commandBuffer[offset], &msDuration, sizeof(msDuration));
		offset += sizeof(msDuration);

		memcpy(&this->m_commandBuffer[offset], code, 4);
		offset += 4;
		 
		memset(&this->m_commandBuffer[offset], 0, 13);
		offset += 13;
		
        return this->sendCommand(&this->m_commandBuffer[0], offset); 		
    }    
    
    //////////////////////////////////////////////////////////////////////////
    
    bool EGIConnection::connect(const char systemSpec[4], const char* address, unsigned short port) {
        bool didConnect = false;
        try {
            if (!this->m_socketEx.connect(address, port)) {
                throw std::runtime_error("this->m_socketEx.connect()");
            }

			if (!this->m_socketEx.sendBeginSession( systemSpec )) {
                throw std::runtime_error("this->m_socketEx.sendBeginSession()");
            }

			didConnect = true;
        }
        catch (std::runtime_error& problem) {
			std::cerr << problem.what() << std::endl;
        }
        return didConnect;
    }
    
    bool EGIConnection::disconnect() {
        bool didDisconnect = false;
        try {
            if (!this->m_socketEx.sendEndSession()) {
                throw std::runtime_error("this->m_socketEx.sendEndSession()");
            }
            this->m_socketEx.disconnect();
            didDisconnect = true;
        }
        catch (std::runtime_error& problem) {  
			std::cerr << problem.what() << std::endl;
        }
        
        return didDisconnect;
    }

    bool EGIConnection::beginRecording() {
        bool didBeginRecording = false;
        try {
            if (!this->m_socketEx.sendBeginRecording()) {
                throw std::runtime_error("this->m_socketEx.sendBeginRecording()");
            }
            didBeginRecording = true;
        }
        catch(std::runtime_error& problem) {
            std::cerr << problem.what() << std::endl;
        }
        return didBeginRecording;
    }
    
    bool EGIConnection::endRecording() {
        bool didEndRecording = false;
        try {
            if (!this->m_socketEx.sendEndRecording()) {
                throw std::runtime_error("this->m_socketEx.sendEndRecording()");
            }
            didEndRecording = true;
        }
        catch(std::runtime_error& problem) {
            std::cerr << problem.what() << std::endl;
        }
        return didEndRecording;
    }
 
	bool EGIConnection::sendAttention() {
		return this->m_socketEx.sendAttention();
	}
	
	bool EGIConnection::sendSynch(long timeStamp) {
		return this->m_socketEx.sendSynch(timeStamp);
	}

	// Sends a four character code, a time stamp marking the elapsed time from the beginning of the experiment
	// and a duration flag that specifies how long the event lasts
	// EGI Documentation states that timestamps must be unique, and all durations must be at least one millisecond 
    bool EGIConnection::sendTrigger(const char* code, long timeStamp, long msDuration) {				
		return this->m_socketEx.sendTrigger(code, timeStamp, msDuration);
	}
}
