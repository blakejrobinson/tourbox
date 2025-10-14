#include "tourbox_client.h"
#include <iostream>
#include <sstream>
#include <iomanip>

const bool DEBUG = false; // Disable debug output for Node.js addon

/**
 * Constructor - Initialize TourBox client wrapper with socket connection
 * @param socket The socket handle for the connected TourBox device
 * Sets up the client state and initializes the control mapping table
 */
TourBoxClientWrapper::TourBoxClientWrapper(socket_t socket, TourBoxServerWrapper* srv) : clientSocket(socket), running(true), server(srv)
{
    initializeControlMap();
}

/**
 * Destructor - Clean up resources and close socket connection
 * Ensures the socket is properly closed when the client is destroyed
 */
TourBoxClientWrapper::~TourBoxClientWrapper() 
{
    if (clientSocket != INVALID_SOCKET) 
	{
        CLOSE_SOCKET(clientSocket);
    }
}

/**
 * Initialize Control Mapping Table
 * Maps TourBox protocol bytes to control actions with Node.js event emission
 * Creates the complete mapping of all TourBox controls including:
 * - Rotation controls (knob, dial, scroll wheel)
 * - Press/release button pairs with state tracking
 * - All C1-C4, T1-T2, and directional buttons
 */
void TourBoxClientWrapper::initializeControlMap() 
{
    controlMap = 
	{
        // Rotation controls (no press/release) - emit events to Node.js
        {132, ControlAction("Knob CCW",       false, -1, [](int count) { EmitToNode("Knob CCW", count); })},
        {196, ControlAction("Knob CW",        false, -1, [](int count) { EmitToNode("Knob CW", count); })},
        {137, ControlAction("Scroll Down",    false, -1, [](int count) { EmitToNode("Scroll Down", count); })},
        {201, ControlAction("Scroll Up",      false, -1, [](int count) { EmitToNode("Scroll Up", count); })},
        {143, ControlAction("Dial CCW",       false, -1, [](int count) { EmitToNode("Dial CCW", count); })},
        {207, ControlAction("Dial CW",        false, -1, [](int count) { EmitToNode("Dial CW", count); })},
        
        // Knob press/release
        {55,  ControlAction("Knob Press",     true, 183, [](int count) { EmitToNode("Knob Press", count); })},
        {183, ControlAction("Knob Release",   false, -1, [](int count) { EmitToNode("Knob Release", count); })},
        
        // Dial press/release
        {56,  ControlAction("Dial Press",     true, 184, [](int count) { EmitToNode("Dial Press", count); })},
        {184, ControlAction("Dial Release",   false, -1, [](int count) { EmitToNode("Dial Release", count); })},
        
        // Directional buttons press/release
        {16,  ControlAction("Up Press",       true, 144, [](int count) { EmitToNode("Up Press", count); })},
        {144, ControlAction("Up Release",     false, -1, [](int count) { EmitToNode("Up Release", count); })},
        {17,  ControlAction("Down Press",     true, 145, [](int count) { EmitToNode("Down Press", count); })},
        {145, ControlAction("Down Release",   false, -1, [](int count) { EmitToNode("Down Release", count); })},
        {18,  ControlAction("Left Press",     true, 146, [](int count) { EmitToNode("Left Press", count); })},
        {146, ControlAction("Left Release",   false, -1, [](int count) { EmitToNode("Left Release", count); })},
        {19,  ControlAction("Right Press",    true, 147, [](int count) { EmitToNode("Right Press", count); })},
        {147, ControlAction("Right Release",  false, -1, [](int count) { EmitToNode("Right Release", count); })},
        
        // Side buttons press/release
        {0,   ControlAction("Tall Press",     true, 128, [](int count) { EmitToNode("Tall Press", count); })},
        {128, ControlAction("Tall Release",   false, -1, [](int count) { EmitToNode("Tall Release", count); })},
        {1,   ControlAction("Side Press",     true, 129, [](int count) { EmitToNode("Side Press", count); })},
		{129, ControlAction("Side Release",   false, -1, [](int count) { EmitToNode("Side Release", count); })},
        {2,   ControlAction("Top Press",      true, 130, [](int count) { EmitToNode("Top Press", count); })},
        {130, ControlAction("Top Release",    false, -1, [](int count) { EmitToNode("Top Release", count); })},
        {3,   ControlAction("Short Press",    true, 131, [](int count) { EmitToNode("Short Press", count); })},
        {131, ControlAction("Short Release",  false, -1, [](int count) { EmitToNode("Short Release", count); })},
        
        // Tour button press/release
        {42,  ControlAction("Tour Press",     true, 170, [](int count) { EmitToNode("Tour Press", count); })},
        {170, ControlAction("Tour Release",   false, -1, [](int count) { EmitToNode("Tour Release", count); })},
        
        // C1/C2 buttons press/release
        {34,  ControlAction("C1 Press",       true, 162, [](int count) { EmitToNode("C1 Press", count); })},
        {162, ControlAction("C1 Release",     false, -1, [](int count) { EmitToNode("C1 Release", count); })},
        {35,  ControlAction("C2 Press",       true, 163, [](int count) { EmitToNode("C2 Press", count); })},
        {163, ControlAction("C2 Release",     false, -1, [](int count) { EmitToNode("C2 Release", count); })},
        
        // Scroll press/release
        {10,  ControlAction("Scroll Press",   true, 138, [](int count) { EmitToNode("Scroll Press", count); })},
        {138, ControlAction("Scroll Release", false, -1, [](int count) { EmitToNode("Scroll Release", count); })}
    };
}

/**
 * Main Client Loop - Handle incoming TourBox data
 * Continuously receives data from the TourBox device socket and processes it
 * Runs in a separate thread to avoid blocking the main application
 * Handles disconnection and error conditions gracefully
 */
void TourBoxClientWrapper::Run() 
{
    char buffer[1024];
    
    while (running) 
	{
        int bytesReceived = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
        
        if (bytesReceived <= 0) 
		{
            if (bytesReceived == 0) 
			{
                if (DEBUG) std::cout << "TourBox Console disconnected" << std::endl;
            } 
			else 
			{
                if (DEBUG) std::cerr << "Recv failed. Error: " << SOCKET_ERROR_CODE << std::endl;
            }
            break;
        }

        processData(buffer, bytesReceived);
    }
}

/**
 * Stop Client Processing
 * Sets the running flag to false, causing the main loop to exit
 * This allows for graceful shutdown of the client thread
 */
void TourBoxClientWrapper::Stop() 
{
    running = false;
}

/**
 * Process Raw Socket Data
 * @param buffer Raw byte data received from TourBox device
 * @param bytesReceived Number of bytes in the buffer
 */
void TourBoxClientWrapper::processData(char* buffer, int bytesReceived) 
{
    // Emit raw data to Node.js
    EmitRawData(buffer, bytesReceived);
    
    //Debug display
    std::stringstream hexStream;    
    for (int i = 0; i < bytesReceived; i++) 
	{
        hexStream << std::hex << std::setw(2) << std::setfill('0') 
                 << (int)(unsigned char)buffer[i];
    }
    std::string hexData = hexStream.str();
    
    if (DEBUG) 
	{
        std::cout << "Raw hex data: " << hexData << " (length: " << bytesReceived << " bytes)" << std::endl;
        
        // Show decimal values of each byte
        std::cout << "Byte values: ";
        for (int i = 0; i < bytesReceived; i++) 
		{
            std::cout << (int)(unsigned char)buffer[i] << " ";
        }
        std::cout << std::endl;
    }

    parseTourBoxData(hexData);
}

/**
 * Parse TourBox Protocol Data
 * @param hexData Hex string representation of the received bytes
 * 
 * Implements sequential processing to group consecutive identical bytes
 * This preserves timing information for rapid control actions
 */
void TourBoxClientWrapper::parseTourBoxData(const std::string& hexData) 
{
    try 
	{
        // Sequential processing - group consecutive identical bytes
        std::vector<int> bytes;
        
        // Convert hex string to byte values
        for (size_t i = 0; i < hexData.length(); i += 2) 
		{
            if (i + 1 < hexData.length()) 
			{
                std::string byteHex = hexData.substr(i, 2);
                
                int value = std::stoi(byteHex, nullptr, 16);
                bytes.push_back(value);
            }
        }
        
        // Process bytes sequentially, grouping consecutive identical values
        if (!bytes.empty()) 
		{
            int currentValue = bytes[0];
            int count = 1;
            
            for (size_t i = 1; i < bytes.size(); i++) 
			{
                if (bytes[i] == currentValue) 
				{
                    count++;
                } 
				else 
				{
                    // Different value, process the previous group
                    if (DEBUG) 
					{
                        std::cout << "Sequential group: " << currentValue << " (count: " << count << ")" << std::endl;
                        
                        // Map to control name
                        auto it = controlMap.find(currentValue);
                        std::string controlName = (it != controlMap.end()) ? it->second.name : "Unknown (" + std::to_string(currentValue) + ")";
                        std::cout << "Action: " << controlName << " x" << count << std::endl;
                    }
                    
                    // Call handler with the count for this group
                    handleTourBoxInput(currentValue, count);
                    
                    // Start new group
                    currentValue = bytes[i];
                    count = 1;
                }
            }
            
            // Process the final group
            if (DEBUG) 
			{
                std::cout << "Sequential group: " << currentValue << " (count: " << count << ")" << std::endl;
                
                // Map to control name
                auto it = controlMap.find(currentValue);
                std::string controlName = (it != controlMap.end()) ? it->second.name : "Unknown (" + std::to_string(currentValue) + ")";
                std::cout << "Action: " << controlName << " x" << count << std::endl;
            }
            
            // Call handler with the count for the final group
            handleTourBoxInput(currentValue, count);
        }
    } 
	catch (const std::exception& e) 
	{
        if (DEBUG) std::cerr << "Parse error: " << e.what() << std::endl;
    }
}

/**
 * Handle Individual TourBox Control Input
 * @param value The byte value representing a specific TourBox control
 * @param count Number of times this control was triggered consecutively
 * 
 * Looks up the control in the mapping table and executes its action
 * Handles button state tracking for press/release pairs
 * Manages held button states to prevent duplicate press events
 * Emits appropriate events to Node.js with count information
 */
void TourBoxClientWrapper::handleTourBoxInput(int value, int count) 
{
    auto it = controlMap.find(value);
    if (it == controlMap.end()) 
	{
        if (DEBUG) std::cout << "Unhandled control (" << value << ")" << std::endl;
        return;
    }
    
    const ControlAction& action = it->second;
    
    // Update button state tracking (stored on server)
    if (action.isPress) 
    {
        // Press event: mark the press code as held
        if (server) server->SetButtonHeld(value, true);
        if (DEBUG) std::cout << action.name << " - HELD" << std::endl;
    } 
    else
    {
        // Release event: find the corresponding press code (press.releaseCode == this release value)
        if (server) {
            int releaseValue = value;
            int pressCodeToClear = -1;
            for (const auto& pair : controlMap) {
                const int code = pair.first;
                const ControlAction& ca = pair.second;
                if (ca.isPress && ca.releaseCode == releaseValue) {
                    pressCodeToClear = code;
                    break;
                }
            }

            if (pressCodeToClear != -1 && server->IsButtonHeld(pressCodeToClear)) {
                server->SetButtonHeld(pressCodeToClear, false);
                if (DEBUG) std::cout << action.name << " - RELEASED (cleared press code " << pressCodeToClear << ")" << std::endl;
            }
        }
    }
    
    if (DEBUG) std::cout << "Custom action: " << action.name << "!" << std::endl;
    
    // Execute the lambda action if it exists
    if (action.action != nullptr) 
	{
        action.action(count);
    }
}

/**
 * Check Button Hold State
 * @param buttonCode The byte value of the button to check
 * @return true if the button is currently being held down, false otherwise
 * 
 * Used to prevent duplicate press events when a button is held down
 * Tracks button states in the buttonStates map for all press/release pairs
 */
bool TourBoxClientWrapper::isButtonHeld(int buttonCode) 
{
    if (!server) return false;
    return server->IsButtonHeld(buttonCode);
}
bool TourBoxClientWrapper::isButtonHeld(const std::string& buttonName) 
{
    for (const auto& pair : controlMap) 
    {
        if (pair.second.name == buttonName) 
        {
            return isButtonHeld(pair.first);
        }
    }
    return false;
}