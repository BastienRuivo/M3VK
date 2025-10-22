
#include <iostream>
#include <cstdlib>

#include "header/HelloTriangleApp.h"

int main()
{
    HelloTriangleApp application;

    try
    {
        application.Run();
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return 0;
}
