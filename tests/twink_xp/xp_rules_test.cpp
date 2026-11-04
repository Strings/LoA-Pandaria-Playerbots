#include "ExperienceRules.h"
#include "TestMain.h"
void RunTests()
{
    using namespace Twinks;
    UMP_CHECK(ParseXPRequest("") == XPRequest::Status);
    UMP_CHECK(ParseXPRequest("  \t\n") == XPRequest::Status);
    UMP_CHECK(ParseXPRequest("status") == XPRequest::Status);
    UMP_CHECK(ParseXPRequest(" on \t") == XPRequest::On);
    UMP_CHECK(ParseXPRequest("off") == XPRequest::Off);
    UMP_CHECK(ParseXPRequest("on extra") == XPRequest::Invalid);
    UMP_CHECK(ParseXPRequest("OFF") == XPRequest::Invalid);
    UMP_CHECK(ParseXPRequest("toggle") == XPRequest::Invalid);
}
