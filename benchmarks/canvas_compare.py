#!/usr/bin/env python3
"""Native Canvas A/B experiment; creates candidate headers only in a temporary directory."""
import argparse
import concurrent.futures
import io
import json
import os
from pathlib import Path
import shlex
import statistics
import subprocess
import tarfile
import tempfile

repo = Path(__file__).resolve().parents[1]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--base', default='0889cf3')
parser.add_argument('--rounds', type=int, default=3)
parser.add_argument('--samples', type=int, default=7)
parser.add_argument('--cpu', type=int, help='Optional Linux CPU affinity')
parser.add_argument('--heavy-count', type=int, default=30000)
parser.add_argument('--results', type=Path, default=Path('canvas-results.json'))
args = parser.parse_args()
if min(args.rounds, args.samples, args.heavy_count) < 1:
    parser.error('counts must be positive')


def output(command):
    return subprocess.check_output(command, cwd=repo)


with tempfile.TemporaryDirectory(prefix='cppui-canvas-bench-') as directory:
    root = Path(directory)
    archive = output(['git', 'archive', args.base, 'cppui'])
    for variant in ('before', 'after'):
        with tarfile.open(fileobj=io.BytesIO(archive)) as files:
            files.extractall(root / variant, filter='data')
    # Experimental setter guards only. Scope-cache integration would be needed
    # before shipping this prototype; none of these workloads call push/pop.
    for backend in ('qt', 'wx'):
        path = root / 'after' / 'cppui' / backend / 'canvas.hpp'
        text = path.read_text()
        if backend == 'qt':
            text = text.replace('        SavedState frame;', '        SavedState frame;\n        const QPen * lastPen = nullptr;\n        const QBrush * lastBrush = nullptr;')
            for name in ('Pen', 'Brush'):
                old = f'painter.set{name}(found->second);'
                assert old in text
                text = text.replace(old, f'if(last{name} != &found->second) {{ painter.set{name}(found->second); last{name} = &found->second; }}')
        else:
            text = text.replace('    SavedState frame;', '    SavedState frame;\n    const wxGraphicsPen * lastPen = nullptr;\n    const wxGraphicsBrush * lastBrush = nullptr;')
            for name in ('Pen', 'Brush'):
                for value, identity in (('found->second', '&found->second'), (f'wxNullGraphics{name}', f'&wxNullGraphics{name}')):
                    old = f'painter.Set{name}({value});'
                    assert old in text
                    text = text.replace(old, f'if(last{name} != {identity}){{painter.Set{name}({value});last{name}={identity};}}')
        path.write_text(text)

    qt_flags = shlex.split(output(['pkg-config', '--cflags', '--libs', 'Qt6Widgets']).decode())
    wx_flags = shlex.split(output(['wx-config', '--cxxflags', '--libs', 'core,base']).decode())

    def build(pair):
        backend, variant = pair
        flags = qt_flags if backend == 'qt' else ['-DCPPUI_BENCH_WX', *wx_flags]
        subprocess.run([os.environ.get('CXX', 'c++'), '-O2', '-DNDEBUG',
                        '-std=c++17' if backend == 'qt' else '-std=c++11',
                        '-I' + str(root / variant), str(repo / 'benchmarks/canvas.cpp'),
                        *flags, '-o', str(root / f'{backend}-{variant}')], check=True)

    pairs = [(b, v) for b in ('qt', 'wx') for v in ('before', 'after')]
    with concurrent.futures.ThreadPoolExecutor(max_workers=os.cpu_count()) as pool:
        list(pool.map(build, pairs))
    rows = []
    metadata = dict(base=output(['git', 'rev-parse', args.base]).decode().strip(),
                    qt=output(['pkg-config', '--modversion', 'Qt6Widgets']).decode().strip(),
                    wx=output(['wx-config', '--version']).decode().strip(), cpu=args.cpu)
    for backend in ('qt', 'wx'):
        for workload, count in (('same', 200000), ('mixed', 200000), ('heavy', args.heavy_count)):
            for pair in range(args.rounds):
                for variant in (('before', 'after') if pair % 2 == 0 else ('after', 'before')):
                    command = [str(root / f'{backend}-{variant}'), workload, str(count), str(args.samples)]
                    if args.cpu is not None:
                        command = ['taskset', '-c', str(args.cpu), *command]
                    values = subprocess.check_output(command, env=dict(os.environ, QT_QPA_PLATFORM='offscreen'), text=True).strip().split(',')
                    row = dict(backend=backend, workload=workload, count=count, variant=variant,
                               pair=pair, hash=values[2], samples=list(map(float, values[3:])))
                    rows.append(row)
                    args.results.write_text(json.dumps(dict(metadata=metadata, rows=rows), indent=2) + '\n')
                    print(backend, workload, variant, pair, statistics.median(row['samples']), flush=True)
            group = [r for r in rows if r['backend'] == backend and r['workload'] == workload]
            if len({r['hash'] for r in group}) != 1:
                raise RuntimeError(f'{backend} {workload}: pixels differ')
            med = {v: statistics.median(x for r in group if r['variant'] == v for x in r['samples']) for v in ('before', 'after')}
            print(backend, workload, med, 'improvement %', 100 * (1 - med['after'] / med['before']), flush=True)
