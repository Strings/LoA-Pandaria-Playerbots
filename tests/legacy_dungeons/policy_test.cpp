#include "SoloDungeonPolicy.h"
#include "TestMain.h"
#include <limits>
void RunTests()
{
    using namespace LegacyDungeons;
    UMP_CHECK(OriginalCap(15,25)==25);
    UMP_CHECK(OriginalCap(70,0)==70);
    UMP_CHECK(Margin(-1)==1 && Margin(20)==20);
    auto decision=[](unsigned level,unsigned cap,unsigned margin) { return Decide(true,false,true,true,false,level,cap,margin); };
    UMP_CHECK(decision(25,25,20)==Decision::Native);
    UMP_CHECK(decision(24,25,20)==Decision::Native);
    UMP_CHECK(decision(44,25,20)==Decision::Deferred);
    UMP_CHECK(decision(45,25,20)==Decision::Solo);
    UMP_CHECK(decision(90,70,20)==Decision::Solo);
    UMP_CHECK(decision(90,0,20)==Decision::Native);
    UMP_CHECK(decision(90,89,1)==Decision::Solo);
    UMP_CHECK(decision(90,1,std::numeric_limits<unsigned>::max())==Decision::Deferred);
    UMP_CHECK(Decide(false,false,true,true,false,90,25,20)==Decision::Native);
    UMP_CHECK(Decide(true,true,true,true,false,90,25,20)==Decision::Native);
    UMP_CHECK(Decide(true,false,false,true,false,90,25,20)==Decision::Native);
    UMP_CHECK(Decide(true,false,true,false,false,90,25,20)==Decision::RequireSpecific);
    UMP_CHECK(Decide(true,false,true,true,true,90,25,20)==Decision::RequireSolo);
    UMP_CHECK(Decide(true,false,true,true,false,1,25,20,true)==Decision::Solo);
    UMP_CHECK(Decide(true,false,true,true,false,1,0,20,true)==Decision::UnknownCap);
}
