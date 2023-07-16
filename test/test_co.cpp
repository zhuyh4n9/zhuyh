#include "all.hpp"

int main()
{
    for(int i = 0;i<2000;i++){
        co [](){
            for(int i =0;i<50000;i++)
                yield_co;
        };
    }
    //zhuyh::Scheduler::Schd::getInstance() -> stop();
    return 0;
}
