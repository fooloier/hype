#!/usr/bin/env python3
"""Run Hype verification with isolated state and persistent evidence."""
import argparse
import fcntl
import hashlib
import json
import os
from pathlib import Path
import shutil
import signal
import subprocess
import sys
import tempfile
import time

ROOT = Path(__file__).resolve().parents[3]
BASE = ROOT / 'build/verification'
APP = ROOT / 'build/hype'
TESTS = ROOT / 'build/tests/hype-tests'
GUI = {
    'overview': ['overviewMode', 'visualSplitterDrag'],
    'editing': ['autosavePreservesSlideEditorCursor', 'typingCodeInVisualEditorKeepsTheDeck'],
    'media': ['visualOperations', 'placeMenuPlacesImage'],
    'presenter': ['presenterViewUsesSeparateScreens'],
}


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def environment(run):
    scratch = run / 'scratch'
    env = dict(os.environ)
    for key in list(env):
        if (key.startswith('HYPE_') and key not in ('HYPE_QMAKE', 'HYPE_MICROTEX_SOURCE')) or key in ('DISPLAY', 'WAYLAND_DISPLAY'):
            env.pop(key)
    env.update(XDG_CONFIG_HOME=str(scratch / 'config'),
               XDG_STATE_HOME=str(scratch / 'state'),
               XDG_DATA_HOME=str(scratch / 'data'),
               XDG_CACHE_HOME=str(scratch / 'cache'),
               XDG_RUNTIME_DIR=str(scratch / 'runtime'),
               OMARCHY_PATH=str(scratch / 'omarchy'),
               QT_QPA_PLATFORM='offscreen', QT_QUICK_BACKEND='software',
               QT_QPA_PLATFORMTHEME='generic', QT_STYLE_OVERRIDE='Fusion')
    return env


def execute(run, argv, *, env=None, expected=0, timeout=120):
    evidence = run / 'evidence'
    index = len(list(evidence.glob('command-*.json'))) + 1
    target = evidence / f'command-{index:03d}.json'
    record = {'argv': list(map(str, argv)), 'cwd': str(run / 'scratch'),
              'started': time.time(), 'expectedExitCode': expected}
    try:
        with subprocess.Popen(record['argv'], cwd=record['cwd'], env=env or environment(run),
                              stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True,
                              start_new_session=True) as process:
            try:
                stdout, stderr = process.communicate(timeout=timeout)
            except BaseException:
                os.killpg(process.pid, signal.SIGTERM)
                try:
                    process.communicate(timeout=5)
                except subprocess.TimeoutExpired:
                    os.killpg(process.pid, signal.SIGKILL)
                    process.communicate()
                raise
            result = subprocess.CompletedProcess(record['argv'], process.returncode, stdout, stderr)
        record.update(stdout=result.stdout, stderr=result.stderr, exitCode=result.returncode)
    except BaseException as error:
        record.update(error=str(error))
        raise
    finally:
        target.write_text(json.dumps(record, indent=2) + '\n')
    print(f'{target.name}: exit {result.returncode}: {" ".join(record["argv"])}')
    if result.returncode != expected:
        raise RuntimeError(f'Expected exit {expected}; see {target}\n{result.stderr}')
    return result


def locate(value):
    run = Path(value).resolve()
    if run.parent != BASE.resolve() or not (run / 'run.json').is_file():
        raise RuntimeError('Expected a run directory created by this helper under build/verification')
    return run


def doctor(run):
    manifest = json.loads((run / 'run.json').read_text())
    if not (run / 'scratch').is_dir():
        raise RuntimeError('Run was cleaned; launch a fresh run')
    for path in (APP, TESTS):
        if digest(path) != manifest['binaries'][str(path)]:
            raise RuntimeError(f'Build changed since launch: {path}; launch again')
    # These commands inspect the seeded deck without changing it.
    execute(run, [APP, '--version'])
    report = json.loads(execute(run, [APP, 'check', 'talk/presentation.md', '--json']).stdout)
    if not report['ok'] or report['slides'] < 1:
        raise RuntimeError('Seed deck is not usable')
    themes = json.loads(execute(run, [APP, 'themes', '--json']).stdout)
    if 'paper' not in themes['themes']:
        raise RuntimeError('Seeded paper theme is missing')
    print(f'Healthy isolated run: {run}')


def launch():
    BASE.mkdir(parents=True, exist_ok=True)
    run = Path(tempfile.mkdtemp(prefix=time.strftime('%Y%m%d-%H%M%S-'), dir=BASE))
    (run / 'scratch/runtime').mkdir(parents=True, mode=0o700)
    (run / 'evidence').mkdir()
    (run / 'run.json').write_text('{}\n')
    print(f'RUN={run}', flush=True)
    try:
        theme = run / 'scratch/omarchy/themes/paper'
        theme.mkdir(parents=True)
        (theme / 'colors.toml').write_text('background = "#ffffff"\nforeground = "#111111"\naccent = "#2255aa"\n')
        # The build products are shared even though each drive has private state.
        with (BASE / 'build.lock').open('w') as lock:
            fcntl.flock(lock, fcntl.LOCK_EX)
            execute(run, ['sh', ROOT / 'bin/build'], timeout=600)
            qmake = (ROOT / 'build/.qmake-used').read_text().strip()
            (ROOT / 'build/tests').mkdir(exist_ok=True)
            execute(run, [qmake, ROOT / 'tests/tests.pro', '-o', ROOT / 'build/tests/Makefile'], timeout=60)
            execute(run, ['make', '-C', ROOT / 'build/tests', '-j4'], timeout=600)
            binaries = {str(path): digest(path) for path in (APP, TESTS)}
        revision = subprocess.run(['git', '-C', str(ROOT), 'rev-parse', 'HEAD'],
                                  capture_output=True, text=True, check=True).stdout.strip()
        (run / 'run.json').write_text(json.dumps({'root': str(ROOT), 'revision': revision,
                                                 'binaries': binaries}, indent=2) + '\n')
        execute(run, [APP, 'new', 'talk/presentation.md', '--title', 'Verification talk', '--theme', 'paper', '--json'])
        shutil.copy2(run / 'scratch/talk/presentation.md', run / 'evidence/seed.md')
        doctor(run)
        execute(run, [APP, 'open', 'talk/presentation.md', '--screenshot', run / 'evidence/launch.png'])
        print(f'READY: {run}\nNo persistent editor process; each drive owns its child process.')
    except BaseException:
        cleanup(run)
        raise


def gui(run, journey):
    doctor(run)
    env = environment(run)
    env['HYPE_GUI_TESTS'] = '1'
    env['HYPE_SPLITTER_SCREENSHOT_DIR'] = str(run / 'evidence')
    for key, filename in [('HYPE_MENU_SCREENSHOT', 'overview-menu.png'),
                          ('HYPE_SHORTCUTS_SCREENSHOT', 'shortcuts.png'),
                          ('HYPE_FILE_MENU_SCREENSHOT', 'file-menu.png'),
                          ('HYPE_PASTE_SCREENSHOT', 'paste-dialog.png')]:
        env[key] = str(run / 'evidence' / filename)
    if journey == 'presenter':
        env['QT_QPA_PLATFORM'] = f'offscreen:configfile={ROOT}/tests/fixtures/presenter-screens.json'
    result = execute(run, [TESTS, *GUI[journey], '-v2'], env=env, timeout=180)
    if 'SKIP ' in result.stdout or 'FAIL!' in result.stdout or '0 passed' in result.stdout:
        raise RuntimeError('GUI journey skipped or failed; inspect its command evidence')
    if journey == 'overview':
        for name in ('overview-menu.png', 'shortcuts.png', 'file-menu.png'):
            if not (run / 'evidence' / name).is_file():
                raise RuntimeError(f'Missing GUI proof: {name}')
        for size in ('default-window', 'minimum-window'):
            for state in ('before', 'expanded'):
                name = f'splitter-{size}-{state}.png'
                if not (run / 'evidence' / name).is_file():
                    raise RuntimeError(f'Missing GUI proof: {name}')
    (run / 'evidence' / f'{journey}-result.json').write_text(json.dumps({
        'journey': journey, 'tests': GUI[journey], 'status': 'passed',
        'surface': 'real Main.qml in Qt Test, offscreen software rendering',
        'limits': 'Native file portals, desktop GPU, actual monitors and installed launcher are unverified.'
    }, indent=2) + '\n')


def cleanup(run):
    scratch = run / 'scratch'
    if scratch.exists():
        shutil.rmtree(scratch)
    retained = sorted(p.name for p in (run / 'evidence').iterdir())
    (run / 'cleanup.json').write_text(json.dumps({'scratchRemoved': not scratch.exists(),
                                               'evidenceRetained': retained}, indent=2) + '\n')
    print(f'Cleaned scratch; {len(retained)} evidence files retained at {run / "evidence"}')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('action', choices=['launch', 'doctor', 'gui', 'cli', 'snapshot', 'cleanup'])
    parser.add_argument('run', nargs='?')
    parser.add_argument('arguments', nargs=argparse.REMAINDER)
    args = parser.parse_args()
    if args.action == 'launch':
        launch()
        return
    if not args.run:
        parser.error('This action requires the RUN directory printed by launch')
    run = locate(args.run)
    if args.action == 'doctor':
        doctor(run)
    elif args.action == 'cleanup':
        cleanup(run)
    elif args.action == 'gui':
        if len(args.arguments) != 1 or args.arguments[0] not in GUI:
            parser.error('gui requires overview, editing, media, or presenter')
        gui(run, args.arguments[0])
    elif args.action == 'snapshot':
        doctor(run)
        execute(run, [APP, 'open', 'talk/presentation.md', '--screenshot', run / 'evidence/editor.png', *args.arguments])
    else:
        arguments = args.arguments
        expected = 0
        if arguments[:1] == ['--expect-failure']:
            expected, arguments = 1, arguments[1:]
        if arguments[:1] == ['--']:
            arguments = arguments[1:]
        if not arguments or arguments[0] not in ('new', 'check', 'slides', 'themes', 'render', 'export', 'help'):
            parser.error('cli supports new/check/slides/themes/render/export/help; outputs should go into evidence')
        execute(run, [APP, *arguments], expected=expected)


if __name__ == '__main__':
    try:
        main()
    except (OSError, ValueError, RuntimeError, subprocess.SubprocessError) as error:
        print(str(error), file=sys.stderr)
        sys.exit(1)
