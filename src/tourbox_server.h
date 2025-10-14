#pragma once

#include <string>
#include <map>
#include <memory>
#include <thread>
#include <mutex>

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #include <windows.h>
    #pragma comment(lib, "ws2_32.lib")
    typedef SOCKET socket_t;
    #define CLOSE_SOCKET closesocket
    #define SOCKET_ERROR_CODE WSAGetLastError()
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #include <cstring>
    typedef int socket_t;
    #define INVALID_SOCKET -1
    #define SOCKET_ERROR -1
    #define CLOSE_SOCKET close
    #define SOCKET_ERROR_CODE errno
#endif

// Forward declarations
class TourBoxClientWrapper;

// Function to emit events to Node.js (defined in tourbox_addon.cc)
extern void EmitToNode(const std::string& eventName, int count);
extern void EmitRawData(const char* buffer, int length);
extern void EmitConnectionEvent(const std::string& eventType, const std::string& ip, int port);

class TourBoxServerWrapper 
{
	private:
		socket_t serverSocket;
		std::atomic<bool> running;
		std::thread serverThread;

	#ifdef _WIN32
		PROCESS_INFORMATION fakeMaxProcess;
		STARTUPINFO startupInfo;
	#endif

	public:
		// Track the held state of buttons globally for all clients
		std::map<int, bool> buttonStates;

		// Mutex to protect buttonStates (accessed from multiple threads)
		std::mutex buttonStatesMutex;

		// Thread-safe accessors
		void SetButtonHeld(int code, bool held);
		bool IsButtonHeld(int code);

		TourBoxServerWrapper();
		~TourBoxServerWrapper();

		bool Initialize();
		bool StartServer(int port = 50500, const std::string& ip = "127.0.0.1");
		void Run();
		void Stop();
		void Cleanup();

	private:
		//bool createFakeMaxProcess();
};