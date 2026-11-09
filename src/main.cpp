#include "cli_dispatcher.hpp"

#include <iostream>

int main(int argc, char* argv[])
{
    return folderflow::runCli(argc, argv, std::cin, std::cout, std::cerr);
}
