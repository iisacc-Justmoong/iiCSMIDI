"""Reject Qt and file-creation bypasses from the native MIDI library."""
from pathlib import Path
import re
import sys
root=Path(sys.argv[1])
for path in (root/'src').rglob('*'):
    if path.suffix not in ('.h','.cpp'): continue
    text=path.read_text()
    assert not re.search(r'#\s*include\s*[<"](?:Qt\w*|Q[A-Z]\w*)',text), f'Qt include: {path}'
    assert not re.search(r'\b(?:QString|QByteArray|QObject|QFile|QSaveFile)\b',text), f'Qt type: {path}'
    assert not re.search(r'#\s*include\s*[<"](?:fstream|fcntl.h|windows.h)',text), f'file bypass: {path}'
for path in [root/'CMakeLists.txt',root/'cmake/iiCSMIDIConfig.cmake.in']:
    assert not re.search(r'find_(?:package|dependency)\(\s*(?:Qt\w*|iiFileProvider\s)',path.read_text()), f'Qt package dependency: {path}'
print('iiCSMIDI native public/source/package boundary passed')
