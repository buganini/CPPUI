# Qt virtual-node test

This is the C++/Qt port of `PUI/tests/virtual_nodes.py`. It exercises sibling
views, a nested view, independently redrawn views, and removing/reinserting a
virtual subtree.

```sh
cmake -S . -B build
cmake --build build
./build/test_virtual_nodes
```
