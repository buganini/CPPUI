# Native DOM offset cache — 2026-10-18

PUI `1c7b45c` caches a retained view's flattened position within its native
owner. This removes repeated tree traversal in C++ as well as Python, so the
optimization is adopted. It affects independent retained-view updates, not
Canvas painting or full-tree reconciliation.

## Measurement

[`benchmarks/dom_offsets.cpp`](../benchmarks/dom_offsets.cpp) creates 16, 128 or
1,024 retained sibling views, each initially containing three native mock nodes.
It measures offset lookup, unchanged-structure redraw, and structural redraw.
The structural case changes the target's child count on every update. There is
no toolkit rendering or event-loop cost in these timings.

Both versions use `g++ -O3 -DNDEBUG -std=c++11`; the baseline headers come from
CPPUI `445002f`. Seven samples alternate before/after order without concurrent
builds. The table reports medians in microseconds per redraw. Raw samples and
compiler identity are in [the measurement file](../benchmarks/results/2026-10-18-dom-offsets.json).

| Views | Stable before | Stable cached | Speedup | Structural before | Structural cached |
| ---: | ---: | ---: | ---: | ---: | ---: |
| 16 | 0.502 | 0.522 | 0.96x | 0.437 | 0.469 |
| 128 | 0.879 | 0.521 | 1.69x | 0.775 | 0.837 |
| 1024 | 5.859 | 0.526 | 11.14x | 5.254 | 5.273 |

Small trees gain little, and structural changes invalidate the cache and can
add overhead. The larger stable-tree improvement justifies the native port;
these ratios are not application frame-rate predictions. A single HeavyCanvas
view doing expensive drawing does not benefit from caching sibling offsets.

To repeat against either header version:

```sh
g++ -O3 -DNDEBUG -std=c++11 -I/path/to/checkout \
    benchmarks/dom_offsets.cpp -o /tmp/dom_offsets
/tmp/dom_offsets
```

## Invalidation and ownership

Owners lazily allocate version tokens; successful target lookups retain only
the token and integer offset, never the parent declaration. A token identifies
both the owner and its current structure. Missing lookups are not cached.
Structural changes invalidate the native owner and logical virtual ancestors,
including intermediate mutations during reconciliation. Declaration retirement
and deletion clear cached tokens. The old DOM already records native order in
CPPUI, so PUI's extra old-list sorting is not copied.

The native tests compare cached offsets with known flattened native order over
200 nested growth, shrink, insertion, removal and reversed/reordered updates.
They also exercise direct virtual-owner queries, owner identity and teardown.
Core regressions pass AddressSanitizer, UBSan and leak checks.
