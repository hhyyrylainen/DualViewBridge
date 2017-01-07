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

static std::ofstream LogFile("~/dualview_bridge_out.txt");
#endif

template<typename T>
void Log(const T &stuff){

#ifdef USE_LOGGING

    LogFile << stuff << std::endl;
    LogFile.flush();
    
#else

#endif
}


//! Main function for receiving data
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

class OutputPool{
public:
    ~OutputPool(){

        SendInternal({
                {"length", message.size()},
                {"content", message} });
    }

    std::string message;
};

static OutputPool OutputQueue;

//! Main function for sending data back
void Send(const std::string &message){

    OutputQueue.message += message + "\n";
}

void StartDualViewProcess(std::string args){

    // TODO: load these paths from some configuration file
    const auto workDir = "/home/hhyyrylainen/Projects/dualviewpp/build/";
    const auto progname = "/home/hhyyrylainen/Projects/dualviewpp/build/dualviewpp";

    

    Send("Plain received args: " + args);
    
    // Parse args //
    // Remove quotes first
    if(args.front() == '"'){

        args.erase(args.begin());
        args.pop_back();
    }
    
    // ; is used as a delimiter
    std::string delimiter = ";";

    size_t pos = 0;

    std::vector<std::string> parsedArgs;
    
    while((pos = args.find(delimiter)) != std::string::npos){
        
        parsedArgs.push_back(args.substr(0, pos));
        args.erase(0, pos + delimiter.length());
    }

    parsedArgs.push_back(args);

    std::stringstream startupMessage;
    startupMessage << "Starting DualView(" << progname << ") with " << parsedArgs.size() <<
        " arguments: ";

    for(const auto& arg : parsedArgs){
        startupMessage << arg << "; ";
    }
    
    Send(startupMessage.str());
    
    auto pid = fork();
    
    if(pid < 0){

        Log("Error: forking");
        Send("Error: forking");
        return;
    }
    
    if(pid > 0){

        // Parent exits here //
        Log("Successfully forked");
        Send("Successfully forked, bridge process exiting");
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
    if(chdir(workDir) < 0){
        
        std::exit(1);
    }



    // Parse args //
    
    std::vector<const char*> startargs;
    startargs.push_back(progname);

    for(const auto& str : parsedArgs){

        startargs.push_back(str.c_str());
    }
    
    startargs.push_back(nullptr);

    // Run the program //
    execv(progname, const_cast<char**>(&startargs.front()));

    // Child never returns from this function
    // and it is an error to return from execv
    Log("Error: child returned from execv");
    std::exit(1);
}

int main(){

    Log("DualView here");

    const auto command = ReadMessage();

    if(command.empty()){

        Send("Error: empty command received");
        return 2;
    }

    StartDualViewProcess(command);

    Send("Started process.");
    
    Log("Done");
    return 0;
}
