# Lambda Method
```
#include <iostream>

#define ABC(...) (*(new _ABC(__VA_ARGS__))) + [](const _ABC *)

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
        std::cout << "build\n";
    };
}
```

# Range-Based For-Loop Method
```
for(auto _: ABC()){
    std::cout << "build\n";
}
```