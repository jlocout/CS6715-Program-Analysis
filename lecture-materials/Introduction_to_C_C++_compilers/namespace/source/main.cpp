#include <iostream>

namespace A 
{
    void process(int i) 
    {
        std::cout <<"process from A: "<<i<<std::endl;
    }
}

namespace B 
{
    void process(int i) 
    {
        std::cout <<"process from B: "<<i<<std::endl;
    }
}

int main() 
{
    int val = 1;
    A::process(val);
    B::process(val); 
    return 0;
}

