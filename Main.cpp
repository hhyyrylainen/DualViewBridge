#include <iostream>

int main(){

    std::cout << "DualView here" << std::endl;

    while(true){

        std::string command;
        std::getline(std::cin, command);

        std::cout << "Got: " + command << std::endl;
        break;
    }

    return 0;
}
