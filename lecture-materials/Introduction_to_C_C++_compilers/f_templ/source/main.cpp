#include <iostream>

using namespace std;

template <typename T>
inline T add (T a, T b) { 
   return a + b; 
}

int main() 
{
    cout<<"INT add:"<<add (1, 2)<<endl;
    cout<<"DOUBLE add:"<<add (1.0, 2.0)<<endl;
    cout<<"STRING add:"<<add (string("1"), string("2"))<<endl;
}

