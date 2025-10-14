#include <napi.h>
#include "tourbox_server.h"
#include <memory>
#include <map>

static std::map<int, std::shared_ptr<TourBoxServerWrapper>> g_servers;
static int g_nextServerId = 1;
static Napi::ThreadSafeFunction g_eventCallback;
static Napi::ThreadSafeFunction g_rawCallback;

// Map control names to their byte codes (must match client's controlMap names)
static std::map<std::string, int> g_nameToCode = {
    {"Knob CCW", 132}, {"Knob CW", 196}, {"Scroll Down", 137}, {"Scroll Up", 201},
    {"Dial CCW", 143}, {"Dial CW", 207}, {"Knob Press", 55}, {"Knob Release", 183},
    {"Dial Press", 56}, {"Dial Release", 184}, {"Up Press", 16}, {"Up Release", 144},
    {"Down Press", 17}, {"Down Release", 145}, {"Left Press", 18}, {"Left Release", 146},
    {"Right Press", 19}, {"Right Release", 147}, {"Tall Press", 0}, {"Tall Release", 128},
    {"Side Press", 1}, {"Side Release", 129}, {"Top Press", 2}, {"Top Release", 130},
    {"Short Press", 3}, {"Short Release", 131}, {"Tour Press", 42}, {"Tour Release", 170},
    {"C1 Press", 34}, {"C1 Release", 162}, {"C2 Press", 35}, {"C2 Release", 163},
    {"Scroll Press", 10}, {"Scroll Release", 138}
};

// buttonState(serverId, controlName)
Napi::Value ButtonState(const Napi::CallbackInfo& info)
{
    Napi::Env env = info.Env();

    std::string name;
    bool checkAll = false;
    int serverId = -1;

    if (info.Length() == 1 && info[0].IsString()) {
        name = info[0].As<Napi::String>().Utf8Value();
        checkAll = true;
    } else if (info.Length() == 2) {
        if (info[0].IsNumber() && info[1].IsString()) {
            serverId = info[0].As<Napi::Number>().Int32Value();
            name = info[1].As<Napi::String>().Utf8Value();
        } else {
            Napi::TypeError::New(env, "Expected arguments: (name) or (serverId, name)")
                .ThrowAsJavaScriptException();
            return env.Null();
        }
    } else {
        Napi::TypeError::New(env, "Expected arguments: (name) or (serverId, name)")
            .ThrowAsJavaScriptException();
        return env.Null();
    }

    // map name to code (try exact, then try "<name> Press")
    auto nit = g_nameToCode.find(name);
    if (nit == g_nameToCode.end()) {
        std::string alt = name + " Press";
        nit = g_nameToCode.find(alt);
        if (nit == g_nameToCode.end()) {
            return Napi::Boolean::New(env, false);
        }
    }

    int code = nit->second;

    if (checkAll) {
        // return true if any server reports this code held
        for (auto& kv : g_servers) {
            auto server = kv.second.get();
            if (!server) continue;
                if (server->IsButtonHeld(code)) {
                    return Napi::Boolean::New(env, true);
                }
        }
        return Napi::Boolean::New(env, false);
    } else {
        auto sit = g_servers.find(serverId);
        if (sit == g_servers.end()) return Napi::Boolean::New(env, false);
        auto server = sit->second.get();
        if (!server) return Napi::Boolean::New(env, false);
            bool held = server->IsButtonHeld(code);
        return Napi::Boolean::New(env, held);
    }
}

// Function to emit events to Node.js
void EmitToNode(const std::string& eventName, int count) 
{
    if (g_eventCallback) 
	{
        auto callback = [eventName, count](Napi::Env env, Napi::Function jsCallback) 
		{
            jsCallback.Call(
			{
                Napi::String::New(env, eventName),
                Napi::Number::New(env, count)
            });
        };
        g_eventCallback.NonBlockingCall(callback);
    }
}

// Function to emit raw data to Node.js
void EmitRawData(const char* buffer, int length) 
{
    if (g_rawCallback) 
	{
        // Copy the buffer data for thread safety
        std::vector<uint8_t> data(buffer, buffer + length);
        auto callback = [data](Napi::Env env, Napi::Function jsCallback) 
		{
            Napi::Buffer<uint8_t> nodeBuffer = Napi::Buffer<uint8_t>::Copy(env, data.data(), data.size());
            jsCallback.Call({nodeBuffer});
        };
        g_rawCallback.NonBlockingCall(callback);
    }
}

// Function to emit connection events to Node.js
void EmitConnectionEvent(const std::string& eventType, const std::string& ip, int port) 
{
    if (g_eventCallback) 
	{
        auto callback = [eventType, ip, port](Napi::Env env, Napi::Function jsCallback) 
		{
            Napi::Object connectionInfo = Napi::Object::New(env);
            connectionInfo.Set("ip", Napi::String::New(env, ip));
            connectionInfo.Set("port", Napi::Number::New(env, port));
            
            jsCallback.Call({
                Napi::String::New(env, eventType),
                connectionInfo
            });
        };
        g_eventCallback.NonBlockingCall(callback);
    }
}

// Create TourBox server
Napi::Value CreateServer(const Napi::CallbackInfo& info) 
{
    Napi::Env env = info.Env();

    if (info.Length() < 2 || !info[0].IsNumber() || !info[1].IsFunction()) 
	{
        Napi::TypeError::New(env, "Expected arguments: (port: number, eventCallback: function, ip?: string, rawCallback?: function)")
            .ThrowAsJavaScriptException();
        return env.Null();
    }

    int port = info[0].As<Napi::Number>().Int32Value();
    Napi::Function eventCallback = info[1].As<Napi::Function>();
    std::string ip = "127.0.0.1";
    
    // Check if IP address is provided as third parameter
    size_t rawCallbackIndex = 2;
    if (info.Length() >= 3 && info[2].IsString()) 
    {
        ip = info[2].As<Napi::String>().Utf8Value();
        rawCallbackIndex = 3;
    }

    // Create thread-safe event callback
    g_eventCallback = Napi::ThreadSafeFunction::New(
        env,
        eventCallback,
        "TourBoxEventCallback",
        0,  // Unlimited queue
        1   // One thread
    );

    // Create thread-safe raw callback if provided
    if (info.Length() > rawCallbackIndex && info[rawCallbackIndex].IsFunction()) 
	{
        Napi::Function rawCallback = info[rawCallbackIndex].As<Napi::Function>();
        g_rawCallback = Napi::ThreadSafeFunction::New(
            env,
            rawCallback,
            "TourBoxRawCallback",
            0,  // Unlimited queue
            1   // One thread
        );
    }

    try 
	{
        auto server = std::make_shared<TourBoxServerWrapper>();
        
        if (!server->Initialize()) 
		{
            Napi::Error::New(env, "Failed to initialize TourBox server")
                .ThrowAsJavaScriptException();
            return env.Null();
        }

        if (!server->StartServer(port, ip)) 
		{
            Napi::Error::New(env, "Failed to start TourBox server on " + ip + ":" + std::to_string(port))
                .ThrowAsJavaScriptException();
            return env.Null();
        }

        int serverId = g_nextServerId++;
        g_servers[serverId] = server;

        return Napi::Number::New(env, serverId);
    } 
	catch (const std::exception& e) 
	{
        Napi::Error::New(env, std::string("Server creation failed: ") + e.what())
            .ThrowAsJavaScriptException();
        return env.Null();
    }
}

// Stop TourBox server
Napi::Value StopServer(const Napi::CallbackInfo& info) 
{
    Napi::Env env = info.Env();

    if (info.Length() < 1 || !info[0].IsNumber()) 
	{
        Napi::TypeError::New(env, "Expected argument: (serverId: number)")
            .ThrowAsJavaScriptException();
        return env.Null();
    }

    int serverId = info[0].As<Napi::Number>().Int32Value();
    
    auto it = g_servers.find(serverId);
    if (it != g_servers.end()) 
	{
        it->second->Stop();
        g_servers.erase(it);
        
        if (g_eventCallback) 
		{
            g_eventCallback.Release();
        }
        if (g_rawCallback) 
		{
            g_rawCallback.Release();
        }
        
        return Napi::Boolean::New(env, true);
    }

    return Napi::Boolean::New(env, false);
}

// Initialize the addon
Napi::Object Init(Napi::Env env, Napi::Object exports) 
{
    exports.Set(
        Napi::String::New(env, "createServer"),
        Napi::Function::New(env, CreateServer)
    );
    
    exports.Set(
        Napi::String::New(env, "stopServer"),
        Napi::Function::New(env, StopServer)
    );

    exports.Set(
        Napi::String::New(env, "buttonState"),
        Napi::Function::New(env, ButtonState)
    );

    return exports;
}

NODE_API_MODULE(tourbox, Init)