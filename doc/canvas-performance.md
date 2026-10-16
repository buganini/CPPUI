# Native Canvas performance audit

Audited PUI `0c160e8..f0c1cf5` on 2026-10-17. The new `push()`/`pop()` API is
ported to Qt and wx, with compatible no-ops for LVGL. The redundant pen/brush
setter guards from `cae7713` and `f0c1cf5` are **not adopted** in CPPUI.
Web/Pyodide command encoding, color caches, repaint scheduling and session
changes remain outside the requested sync.

PUI avoids Python binding dispatch as well as native setter work. CPPUI already
calls the native methods directly. Its wx implementation uses wxGraphicsContext
and cached wxGraphicsPen/Brush objects, whereas PUI's geometry uses wxDC. In wx,
[graphics setters assign reference-counted objects](https://github.com/wxWidgets/wxWidgets/blob/v3.2.8/src/common/graphcmn.cpp#L663);
this is different from the Python/DC path. Native rendering still dominates the
measured workloads. Scope support is a functionality change and is retained
regardless of the optimization decision.

## Measurements

Baseline: CPPUI `0889cf3`, which already contains the per-frame style caches.
The candidate adds pointer-identity guards around pen/brush setters, including
null styles. It still performs the same style-cache lookups and draws every
primitive. This measures only the **incremental setter guard**, not the earlier
per-frame caches or the new scope API. The prototype does not implement scope
tracking and must not be shipped as a production header.

Environment: Ryzen 7 5700G, GCC 16.0.1, `-O2 -DNDEBUG`, Qt 6.10.2 software QImage
rasterization, wx 3.2.9 GTK/Cairo with a 32-bit wxBitmap. Surfaces are 1024×720.
Data generation, surface clearing and pixel hashing are outside the timed region;
context creation, CanvasPainter creation/destruction, drawing and flushing are
inside. Each invocation warms up two frames and measures seven. Three A/B pairs
alternate AB / BA / AB; the table reports the median of 21 frames per variant.
There were no concurrent CPPUI builds during measurement.

| Backend / workload | Existing setters (ms) | Guarded setters (ms) | Time reduction |
| --- | ---: | ---: | ---: |
| Qt, repeated color | 205.985 | 201.115 | 2.36% |
| Qt, changing colors | 241.993 | 242.715 | −0.30% |
| Qt, 30,000-row HeavyCanvas geometry | 97.144 | 93.472 | 3.78% |
| wx, repeated color | 838.832 | 840.902 | −0.25% |
| wx, changing colors | 868.470 | 865.327 | 0.36% |
| wx, 30,000-row HeavyCanvas geometry | 518.950 | 529.190 | −1.97% |

The repeated/changing workloads each issue 200,000 line/rectangle pairs. The
HeavyCanvas workload uses the cookbook's seeded data and its OHLCV/indicator
geometry passes (about nine calls per row), excluding text, hover and window
presentation. All A/B pixel hashes match within each backend/workload.

A further Qt check used 100,000 rows, CPU 0 affinity, four alternating pairs and
seven measured frames per invocation. Its aggregate medians were 475.704 ms
and 475.413 ms (0.06%). Paired median reductions were 0.70%, 0.00%, −1.08% and
1.16%. System speed varied across pairs; an earlier affinity run changed timing
bands during a pair and was discarded rather than treating its aggregate as a
regression. These results do not establish a useful gain for the larger chart.
They are local observations, not cross-platform performance guarantees.

Qt's small repeat-style gain does not justify adding another cache that must
stay synchronized with native state and nested scopes. wx has no consistent
benefit. CPPUI therefore keeps unconditional native setters, and keeps the
existing per-frame object caches. Reconsider if a relevant native workload
shows a repeatable benefit. No timing threshold is added to CI.

## Reproduce

The [benchmark](../benchmarks/canvas.cpp) draws through the real CPPUI headers.
The [comparison script](../benchmarks/canvas_compare.py) exports a specified Git
revision into temporary directories, builds both variants with the same flags,
alternates them and rejects mismatched pixels. It requires Python 3.12+, a C++
compiler, Qt6Widgets pkg-config metadata, wx-config and a display for wx.
It does not modify the working tree's headers.

```sh
python3 benchmarks/canvas_compare.py --base 0889cf3 \
  --rounds 3 --samples 7 --results /tmp/canvas-results.json
# Optional affinity and a larger representative dataset:
python3 benchmarks/canvas_compare.py --base 0889cf3 --cpu 0 \
  --heavy-count 100000 --rounds 4 --samples 7 \
  --results /tmp/canvas-large-results.json
```

[Recorded samples and hashes](../benchmarks/results/2026-10-17-canvas-styles.json)
include the initial six workloads and the final Qt large-dataset confirmation.

wx scopes explicitly restore CPPUI graphics pen/brush objects. Native
[wxCairoContext PushState/PopState](https://github.com/wxWidgets/wxWidgets/blob/v3.2.8/src/generic/graphicc.cpp#L2763)
only saves/restores Cairo state, not wx's pen/brush wrapper members; simply
copying Qt's native save/restore implementation would be incorrect here.
