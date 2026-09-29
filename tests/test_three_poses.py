"""Read original PNGs and exercise the ROM's C pixel routine via a host DLL."""
import ctypes
import json
from pathlib import Path
from PIL import Image

import subprocess
import tempfile
repo = Path(__file__).resolve().parents[1]
root = repo
temporary = tempfile.TemporaryDirectory()
library = Path(temporary.name) / 'poses.so'
subprocess.run(['gcc', '-shared', '-fPIC', '-O2', '-Wall', '-Wextra', '-Werror', str(repo / 'src/three_pose_pixels.c'), '-o', str(library)], check=True)
lib = ctypes.CDLL(str(library))
build = lib.BuildThreePosePixels
build.argtypes = [ctypes.POINTER(ctypes.c_ubyte), ctypes.POINTER(ctypes.c_ubyte), ctypes.c_uint]
build.restype = None

def tiles(image):
    pixels = list(image.getdata())
    data = bytearray(2048)
    for y in range(64):
        for x in range(64):
            value = pixels[y * 64 + x]
            assert 0 <= value < 16
            offset = ((y // 8) * 8 + x // 8) * 32 + (y % 8) * 4 + (x % 8) // 2
            data[offset] |= value << ((x & 1) * 4)
    return data

results = []
failures = []
for path in sorted((repo / 'graphics/pokemon').rglob('*.png')):
    if path.name not in ('anim_front.png', 'back.png'):
        continue
    if any(part in ('question_mark', 'egg') for part in path.parts):
        continue
    image = Image.open(path)
    assert image.mode == 'P', path
    for index in range(image.height // 64):
        source = tiles(image.crop((0, index * 64, 64, index * 64 + 64)))
        if not any(source):  # Unused blank upstream frames are not playable poses.
            continue
        src = (ctypes.c_ubyte * 2048).from_buffer_copy(source)
        frames = [bytes(source)]
        for pose in (1, 2):
            guarded = (ctypes.c_ubyte * 2080)(*([0xA5] * 2080))
            dst = ctypes.cast(ctypes.byref(guarded, 16), ctypes.POINTER(ctypes.c_ubyte))
            build(src, dst, pose)
            assert bytes(guarded[:16]) == b'\xa5' * 16
            assert bytes(guarded[-16:]) == b'\xa5' * 16
            frame = bytes(guarded[16:-16])
            original_colors = {n for byte in source for n in (byte & 15, byte >> 4)} | {0}
            assert all(n in original_colors for byte in frame for n in (byte & 15, byte >> 4))
            frames.append(frame)
        unique = len(set(frames))
        record = {'file': str(path.relative_to(repo)), 'source_frame': index, 'unique_poses': unique}
        results.append(record)
        if unique != 3:
            failures.append(record)

report = {'checked_source_frames': len(results), 'checked_sheets': len({r['file'] for r in results}),
          'failures': failures, 'checks': ['three distinct tile buffers', 'destination bounds', 'palette indices preserved'],
          'results': results}
(root / 'pose-test-report.json').write_text(json.dumps(report, indent=2))
print(json.dumps({key: value for key, value in report.items() if key != 'results'}, indent=2))
assert not failures
