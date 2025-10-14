#include "tourbox_server.h"
#include "tourbox_client.h"
#include <iostream>

const bool DEBUG = false; // Disable debug output for Node.js addon

/**
 * Constructor - Initialize TourBox server wrapper
 * Sets up initial state and Windows-specific process structures
 * Initializes socket to invalid state and running flag to false
 * Prepares Windows process info structures for fake Max/MSP process creation
 */
TourBoxServerWrapper::TourBoxServerWrapper() : serverSocket(INVALID_SOCKET), running(false) 
{
#ifdef _WIN32
    ZeroMemory(&fakeMaxProcess, sizeof(fakeMaxProcess));
    ZeroMemory(&startupInfo, sizeof(startupInfo));
    startupInfo.cb = sizeof(startupInfo);
#endif
}

/**
 * Destructor - Clean up server resources
 * Ensures proper shutdown sequence by stopping the server first
 * Then cleans up all allocated resources including sockets and processes
 * Guarantees no resource leaks when server object is destroyed
 */
TourBoxServerWrapper::~TourBoxServerWrapper() 
{
    Stop();
    Cleanup();
}

/**
 * Initialize Server Components
 * @return true if initialization successful, false otherwise
 * 
 * Performs platform-specific initialization:
 * - Windows: Initializes Winsock for network operations
 * - Other platforms: Currently no special initialization needed
 * 
 * Must be called before starting the server to ensure proper network functionality
 */
bool TourBoxServerWrapper::Initialize() 
{
#ifdef _WIN32
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) 
	{
        if (DEBUG) std::cerr << "WSAStartup failed: " << result << std::endl;
        return false;
    }
#endif
    return true;
}

/**
 * Start TourBox TCP Server
 * @param port Port number to listen on (default: 50500)
 * @param ip IP address to bind to (default: "127.0.0.1", use "0.0.0.0" for all interfaces)
 * @return true if server started successfully, false otherwise
 */
bool TourBoxServerWrapper::StartServer(int port, const std::string& ip) 
{
    if (DEBUG) std::cout << "Starting TourBox server on " << ip << ":" << port << "..." << std::endl;

    // Create socket
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == INVALID_SOCKET) 
	{
        if (DEBUG) std::cerr << "Failed to create socket. Error: " << SOCKET_ERROR_CODE << std::endl;		
        return false;
    }

    // Allow socket reuse
    int opt = 1;
#ifdef _WIN32
    setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt));
#else
    setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
#endif

    // Setup address
    sockaddr_in serverAddr;
	memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    serverAddr.sin_addr.s_addr = inet_addr(ip.c_str());

    // Bind socket
    if (bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) 
	{
        if (DEBUG) std::cerr << "Bind failed. Error: " << SOCKET_ERROR_CODE << std::endl;
		closesocket(serverSocket);
		serverSocket = INVALID_SOCKET;
        return false;
    }

    // Listen for connections
    if (listen(serverSocket, 5) == SOCKET_ERROR) 
	{
        if (DEBUG) std::cerr << "Listen failed. Error: " << SOCKET_ERROR_CODE << std::endl;
		closesocket(serverSocket);
		serverSocket = INVALID_SOCKET;		
        return false;
    }

    if (DEBUG) std::cout << "Server listening on " << ip << ":" << port << std::endl;
    if (DEBUG) std::cout << "TourBox Console should connect automatically!" << std::endl;

    running = true;
    
    // Start server thread
    serverThread = std::thread(&TourBoxServerWrapper::Run, this);
    
    return true;
}

/**
 * Main Server Loop - Accept and Handle Client Connections
 * Runs in a separate thread to handle incoming TourBox device connections
 */
void TourBoxServerWrapper::Run() 
{
    while (running) 
    {
        sockaddr_in clientAddr;
#ifdef _WIN32
        int clientAddrSize = sizeof(clientAddr);
#else
        socklen_t clientAddrSize = sizeof(clientAddr);
#endif
        socket_t clientSocket = accept(serverSocket, (sockaddr*)&clientAddr, &clientAddrSize);
        if (clientSocket == INVALID_SOCKET) 
        {
            if (running && DEBUG) 
            {
                std::cerr << "Accept failed. Error: " << SOCKET_ERROR_CODE << std::endl;
            }
            continue;
        }

        if (DEBUG) std::cout << "TourBox Console connected!" << std::endl;
        if (DEBUG) std::cout << "Connection from: " << inet_ntoa(clientAddr.sin_addr) 
                  << ":" << ntohs(clientAddr.sin_port) << std::endl;

        // Emit connection event to Node.js
        std::string clientIP = inet_ntoa(clientAddr.sin_addr);
        int clientPort = ntohs(clientAddr.sin_port);
        EmitConnectionEvent("connect", clientIP, clientPort);

        // Create and run client in a separate thread
        std::thread clientThread([this, clientSocket, clientIP, clientPort]() 
        {
            TourBoxClientWrapper client(clientSocket, this);
            client.Run();
            // Emit disconnect event when client stops
            EmitConnectionEvent("disconnect", clientIP, clientPort);
        });
        clientThread.detach();
    }
}

/**
 * Stop Server and Clean Shutdown
 * Initiates graceful shutdown sequence for the TourBox server
 */
void TourBoxServerWrapper::Stop() 
{
    if (DEBUG) std::cout << "Stopping TourBox server..." << std::endl;
    running = false;
    
    if (serverSocket != INVALID_SOCKET) 
	{
        CLOSE_SOCKET(serverSocket);
        serverSocket = INVALID_SOCKET;
    }
    
    if (serverThread.joinable()) 
	{
        serverThread.join();
    }
}

/**
 * Clean Up All Server Resources
 * Performs final cleanup of all allocated resources and processes
 */
void TourBoxServerWrapper::Cleanup() 
{
#ifdef _WIN32
    if (fakeMaxProcess.hProcess) 
	{
        if (DEBUG) std::cout << "Terminating fake Max process..." << std::endl;
        TerminateProcess(fakeMaxProcess.hProcess, 0);
        CloseHandle(fakeMaxProcess.hProcess);
        CloseHandle(fakeMaxProcess.hThread);
    }
    WSACleanup();
#endif
}

// Thread-safe button state accessors
void TourBoxServerWrapper::SetButtonHeld(int code, bool held)
{
    std::lock_guard<std::mutex> g(buttonStatesMutex);
    buttonStates[code] = held;
}

bool TourBoxServerWrapper::IsButtonHeld(int code)
{
    std::lock_guard<std::mutex> g(buttonStatesMutex);
    auto it = buttonStates.find(code);
    return (it != buttonStates.end() && it->second);
}