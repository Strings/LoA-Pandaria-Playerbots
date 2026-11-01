#ifndef ULTIMATEMOP_EXPERIENCE_RULES_H
#define ULTIMATEMOP_EXPERIENCE_RULES_H
#include <string_view>
namespace Twinks
{
    enum class XPRequest { Status, On, Off, Invalid };
    constexpr XPRequest ParseXPRequest(std::string_view args)
    {
        auto first = args.find_first_not_of(" \t\r\n");
        if (first == std::string_view::npos)
            return XPRequest::Status;
        args = args.substr(first, args.find_last_not_of(" \t\r\n") - first + 1);
        return args == "status" ? XPRequest::Status : args == "on" ? XPRequest::On : args == "off" ? XPRequest::Off : XPRequest::Invalid;
    }
}
#endif
