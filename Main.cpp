#include <iostream>
#include <cstdint>
#include <fstream>

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "json.hpp"

using json = nlohmann::json;

// Config
#define CONFIG_FILE_LOCATION "dualview/bridge.json"
#define VERBOSE_LOG_LOCATION "dualview_bridge_out.txt"
#define LOGGING_DEFAULT_VALUE 0

// Start of code

static bool loggingEnabled = LOGGING_DEFAULT_VALUE;

static std::ofstream LogFile;

std::string DetermineLogLocation();

template<typename T>
void Log(const T &stuff){

    if(!loggingEnabled)
        return;

    if(!LogFile.is_open()){
        LogFile.open(DetermineLogLocation());

        if(!LogFile.good()){
            std::cout << "FAILED TO OPEN LOG FILE FOR WRITING!" << std::endl;
            throw std::runtime_error("Failed to open log file for writing");
        }
    }

    LogFile << stuff << std::endl;
    LogFile.flush();
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

        if(message.empty())
            return;

        SendInternal({
                {"length", message.size()},
                {"content", message} });
    }

    std::string message;
};

static OutputPool OutputQueue;

class Config{
public:

    std::string workDir;
    std::string dualViewExecutable;
};

//! Main function for sending data back
void Send(const std::string &message){

    OutputQueue.message += message + "\n";
}

void StartDualViewProcess(std::string args, const Config& config){

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
    startupMessage << "Starting DualView(" << config.dualViewExecutable << ") with " <<
        parsedArgs.size() << " arguments: ";

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
    if(chdir(config.workDir.c_str()) < 0){

        std::exit(1);
    }

    std::vector<const char*> startargs;
    startargs.push_back(config.dualViewExecutable.c_str());

    for(const auto& str : parsedArgs){

        startargs.push_back(str.c_str());
    }

    startargs.push_back(nullptr);

    // Run the program //
    execv(config.dualViewExecutable.c_str(), const_cast<char**>(&startargs.front()));

    // Child never returns from this function
    // and it is an error to return from execv
    Log("Error: child returned from execv");
    std::exit(1);
}

std::string ReadBridgeJson() {
#ifdef _WIN32
    #error "Support for Windows has not been implemented"
#endif

    const char* xdgDataHome = std::getenv("XDG_DATA_HOME");
    const char* home = std::getenv("HOME");

    // Determine the base directory
    std::string baseDir;
    if (xdgDataHome) {
        baseDir = xdgDataHome;
    } else if (home) {
        baseDir = std::string(home) + "/.local/share";
    } else {
        throw std::runtime_error(
            "Neither XDG_DATA_HOME nor HOME environment variables are set.");
    }

    // Construct the full path to config file
    std::string filePath = baseDir + "/" + CONFIG_FILE_LOCATION;

    // Open the file and read its contents
    std::ifstream file(filePath);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file: " + filePath);
    }

    // Read the file contents
    std::string content((std::istreambuf_iterator<char>(file)),
        std::istreambuf_iterator<char>());
    file.close();

    return content;
}


Config ParseConfig(){
    auto parsedJson = nlohmann::json::parse(ReadBridgeJson());

    Config result{};

    result.workDir = parsedJson.at("workDir");
    result.dualViewExecutable = parsedJson.at("dualviewExecutable");

    if(parsedJson.contains("logging")){
        loggingEnabled = parsedJson["logging"].get<bool>();
    }

    return result;
}

std::string DetermineLogLocation(){
#ifdef _WIN32
#error "Support for Windows has not been implemented"
#endif

    const char* home = std::getenv("HOME");

    if(home){
        return std::string(home) + "/" + VERBOSE_LOG_LOCATION;
    }

    return VERBOSE_LOG_LOCATION;
}

int main(){

    Log("DualView here");

    const auto command = ReadMessage();

    if(command.empty()){

        Send("Error: empty command received");
        return 2;
    }

    try{
        const auto config = ParseConfig();

        StartDualViewProcess(command, config);

    } catch(const std::exception& e){
        Log("Error when running or processing config");
        Log(e.what());
        Send("Error: problem when loading config or executing dualview (" +
            std::string(e.what()) + ")");
        return 3;
    }

    Send("Started process.");

    Log("Done");
    return 0;
}
