#include <iostream>
using namespace std;

class Base 
{
public:
    virtual void foo() { cout << "Base::foo()" << endl; }
};

class Derived : public Base 
{
public:
    void foo() override { cout << "Derived::foo()" << endl; }
};

void callFunction(Base* obj) 
{
    obj->foo();
}

int main() 
{
    Base b;
    Derived d;
    callFunction(&b);
    callFunction(&d);
}
