#pragma once

#include "tourbox_server.h"
#include <map>
#include <string>
#include <functional>

struct ControlAction 
{
    std::string name;
    bool isPress;           // true for press events, false for release events
    int releaseCode;        // corresponding release code for press events (-1 if not applicable)
    std::function<void(int)> action; // lambda function called with count parameter
    
    ControlAction(const std::string& n, bool press = false, int release = -1, std::function<void(int)> func = nullptr) 
        : name(n), isPress(press), releaseCode(release), action(func) {}
};

class TourBoxServerWrapper; // forward declaration

class TourBoxClientWrapper 
{
	private:
		socket_t clientSocket;
		bool running;
		// Pointer to server to access shared state (button states live on server)
		TourBoxServerWrapper* server;
		
		// Control mapping with lambdas that emit to Node.js
		std::map<int, ControlAction> controlMap;

	public:
	TourBoxClientWrapper(socket_t socket, TourBoxServerWrapper* server);
		~TourBoxClientWrapper();
		
		void Run();
		void Stop();

	private:
		void initializeControlMap();
		void processData(char* buffer, int bytesReceived);
		void parseTourBoxData(const std::string& hexData);
		void handleTourBoxInput(int value, int count);
		bool isButtonHeld(int buttonCode);
		bool isButtonHeld(const std::string& buttonName);
};