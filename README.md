# Lambda Method
```
// g++ -std=c++11  -o test test.cpp
#include <iostream>

#define ABC(...) (*(new _ABC(__VA_ARGS__))) + [](const _ABC * parent)

class _ABC{
    public:
    _ABC() {
        std::cout << "_ABC\n";
    }
    _ABC operator+ (void (*f)(const _ABC *)) const
    {
        std::cout << "add body\n";
        f(this);
        return *this;
    }
};

int main(int argc, char *argv[]){
    ABC() {
        std::cout << "build, parent=" << parent << "\n";
    };
}
```

## Pros
* Cleaner Syntax
* Better view refresh granularity

## Cons
* Requires trailing semicolon

# Range-Based For-Loop Method
```
for(auto parent: ABC()){
    std::cout << "build\n";
}
```

## Pros
* No MACRO
* No trailing semicolon

## Cons
* Worse view refresh granularity
