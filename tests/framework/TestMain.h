#ifndef ULTIMATEMOP_TEST_MAIN_H
#define ULTIMATEMOP_TEST_MAIN_H

#include <exception>
#include <iostream>
#include <stdexcept>
#include <string>

// Exceptions keep checks active in Release and report the failed contract to CTest.
#define UMP_CHECK(condition) do { if (!(condition)) throw std::runtime_error( \
    std::string(__FILE__) + ":" + std::to_string(__LINE__) + ": " #condition); } while (false)

void RunTests();

int main()
{
    try
    {
        RunTests();
        return 0;
    }
    catch (std::exception const& error)
    {
        std::cerr << error.what() << '\n';
    }
    catch (...)
    {
        std::cerr << "Unexpected non-standard exception\n";
    }
    return 1;
}

#endif
