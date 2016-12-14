#include <iostream>
#include <cstdint>
#include <fstream>

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "json.hpp"

using json = nlohmann::json;

//#define USE_LOGGING

#ifdef USE_LOGGING

static std::ofstream LogFile("/home/hhyyrylainen/dualview_bridge_out.txt");
#endif

template<typename T>
void Log(const T &stuff){

#ifdef USE_LOGGING

    LogFile << stuff << std::endl;
    LogFile.flush();
    
#else

#endif
}



std::string ReadMessage(){

    uint32_t size;
    std::cin.read(reinterpret_cast<char*>(&size), 4);

    if(!std::cin.good()){

        Log("Cin ended");
        std::exit(0);
        return "";
    }

    Log("Size: ");
    Log(size);
    
    std::string command;
    command.resize(size);
    std::cin.read(const_cast<char*>(command.c_str()), size);

    Log("Received command: " + command);
    return command;
}

void WriteMessage(const std::string &message){

    Log("Writing message:" + message);

    // First size //
    uint32_t size = message.size();
    std::cout.write(reinterpret_cast<char*>(&size), 4);
    
    // Then the data //
    std::cout.write(const_cast<char*>(message.c_str()), size);
}

void SendInternal(const json &value){

    WriteMessage(value.dump());
}

void Send(const std::string &message){

    SendInternal({
            {"length", message.size()},
            {"content", message} });
}

void StartDualViewProcess(){

    auto pid = fork();
    
    if(pid < 0){

        Log("Error: forking");
        Send("Error: forking");
        return;
    }
    
    if(pid > 0){

        // Parent exits here //
        Log("Successfully forked");
        return;
    }

    // Create a new sid //
    auto sid = setsid();
    if(sid < 0){
        
        std::exit(1);
    }

    freopen("/dev/null", "r", stdin);
    freopen("/dev/null", "w", stdout);
    freopen("/dev/null", "w", stderr);

    // Reset umask, this should be the default //
    umask(022);

    // Move to executable directory //
    if(chdir("/home/hhyyrylainen/Projects/dualviewpp/build/") < 0){
        
        std::exit(1);
    }

    const auto progname = "/home/hhyyrylainen/Projects/dualviewpp/build/dualviewpp";

    const char* args[] = {
        progname,
        //"--main",
        nullptr
    };

    // Run the program //
    execv(progname, const_cast<char**>(args));

    // Child never returns from this function
    // and it is an error to returns from execv
    std::exit(1);
}

int main(){

    Log("DualView here");

    const auto command = ReadMessage();


    StartDualViewProcess();

    Send("Started process.");
    
    Log("Done");
    return 0;
}
