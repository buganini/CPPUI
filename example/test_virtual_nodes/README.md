# Qt virtual-node test

This is the C++/Qt port of [PUI/tests/virtual_nodes.py](https://github.com/buganini/PUI/blob/009e9b9c3fbd724f0b73c862f8213ddc4742405c/tests/virtual_nodes.py). It exercises sibling
views, a nested view, independently redrawn views, and removing/reinserting a
virtual subtree.

```sh
cmake -S . -B build
cmake --build build
./build/test_virtual_nodes
ctest --test-dir build --output-on-failure
```

Run with `--smoke` for assertions covering independent updates, native Hide/Show
clicks, and repeated nested subtree removal/reinsertion. CTest runs it offscreen;
`-DCPPUI_VALGRIND_TESTS=ON` also registers a Memcheck run. The root Qt build
includes the same automated example checks.
