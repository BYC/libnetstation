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

// See ftp://ftp.egi.com/pub/documentation/manuals/ges_122106.pdf for more information on structure layout

#ifndef libnetstation_h
#define libnetstation_h

#if defined(_WIN32) || defined(_WIN64)
#include <winsock2.h> // Required for SOCKET type
#pragma comment(lib,"ws2_32.lib") // Automatically link to the winsock 2 library
#endif

namespace NetStation {
    
	#pragma pack(push,1)
    struct Trigger {
        unsigned short size;
        long startTime;
        long duration;
        char code[4];
    };
    
    struct Label {
        unsigned char labelLength;
        char label[256];
    };
    
    struct Description {
        unsigned char descriptionLength;
        char description[256];
    };

    struct Key {
        char key[4];
        char keyDataType[4];
        unsigned short keyDataLength;
        unsigned char keyData[65536];
    };

    struct Event {
        Trigger trigger;
        Label label;
        Description description;
        Key key;
    };
	#pragma pack(pop)
    
    extern const char kMac[4];
    extern const char kUnix[4];
    extern const char kIntel[4];

    class Socket {
    protected:    
	#if defined(_WIN32) || defined(_WIN64)
		SOCKET m_socket;
	#else    
		int m_socket;    
	#endif
	public:
        Socket();
        ~Socket();
    
        bool connect(const char *address, unsigned short port);
        void disconnect();

        size_t sendComplete(const char* data, size_t size) const;
        size_t recvComplete(char* data, size_t size) const;        
    };

    class SocketEx : public Socket {
    protected:
        char m_commandBuffer[ sizeof(Event) ];
    
    public:
        static const char kQuery;
        static const char kExit;
        static const char kBeginRecording;
        static const char kEndRecording;
        static const char kAttention;
        static const char kTimeSynch;
        static const char kEventDataStream;
    
        static const char kQuerySuccess;
        static const char kSuccess;
        static const char kFailure;

        bool sendCommand(const char *command, const size_t commandSize) const;
        bool sendBeginSession(const char systemSpec[4]);
        bool sendEndSession() const;
        bool sendBeginRecording() const;
        bool sendEndRecording() const;
        bool sendAttention() const;
        bool sendTimeSynch(int timeInMilliseconds);
        bool sendTrigger(Trigger& trigger);
        bool sendEvent(Event& event);                
    };
    
    class EGIConnection {
        protected: 
            SocketEx m_socketEx;
        
        public:
            bool connect(const char systemSpec[4], const char* address, unsigned short port);
            bool disconnect();
            bool beginRecording();
            bool endRecording();
            bool sendTrigger(const char *code, long timeStamp, long msDuration);
    };
}

#endif
