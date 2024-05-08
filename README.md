# Lambda Method
```c++
// g++ -std=c++11  -o test test.cpp
#include <iostream>

#define ABC(...) (*(new _ABC(__VA_ARGS__))) + [](_ABC * parent)

class _ABC{
    public:
    _ABC() {
        std::cout << "_ABC\n";
    }

    _ABC &operator+ (void (*f)(_ABC *))
    {
        std::cout << "add body\n";
        f(this);
        return *this;
    }

    _ABC &xx() {
        std::cout << "xx@" << this << "\n";
        return *this;
    }

    _ABC &oo() {
        std::cout << "oo@" << this << "\n";
        return *this;
    }
};

int main(int argc, char *argv[]){
    ABC() {
        std::cout << "build, parent=" << parent << "\n";
    };

    // method chaining
    (*(new _ABC())).xx().oo() + [](_ABC * parent){
        std::cout << "build2, parent=" << parent << "\n";
    };
}
```

## Pros
* Cleaner Syntax
* Better view refresh granularity

## Cons
* Uses macro (no namespace)
* Requires trailing semicolon
* No syntax sugar for method chaining?

# Range-Based For-Loop Method
```c++
for(auto parent: ABC()->xx()->oo()){
    std::cout << "build\n";
}
```
* See `example/minimal/main.cpp`

## Pros
* No MACRO
* No trailing semicolon
* Method chaining

## Cons
* Worse view refresh granularity
