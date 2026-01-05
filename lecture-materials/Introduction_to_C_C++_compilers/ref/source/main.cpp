#include <iostream>
 
void increment(int& ref) 
{
    ref++;
}
 
int main() 
{
    int x = 10;
    increment(x);
    std::cout << x << std::endl;
    return 0;
}

