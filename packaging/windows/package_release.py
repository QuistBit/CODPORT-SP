"""Build the public 724A Windows ZIP from the five tested release binaries."""
import argparse
import hashlib
from pathlib import Path
import zipfile


HERE = Path(__file__).resolve().parent
REPO = HERE.parent.parent
ZIP_NAME = 'CODPORT-SP-724A-Windows-x86.zip'
MODULES = {
    'WolfSP.exe': '74f520f684299980d984d336a58ef1caff1c6413ae401100492a47c41d21cb72',
    'cgamex86.dll': 'e8048099cb35735c97914e66ea421ea62c15bcf7a604e44aa33469bffd48aba3',
    'qagamex86.dll': '0eff768b940448b8a7d53f1fa55ac8b85b6399d1cc2befd7860d1ec4df276eb6',
    'soft_oal.dll': '606cf8b4c22c7ae00813585279ac045115907bc66d1483eb26355907021ea4b0',
    'uix86.dll': '5a794bf2052c7479d4ee514fb0e8cd52a9c442f08e90977f254e5f0b4380a187',
}
PLAYER_SHA = '96944d70d4a272c1e880d0d55cda44150d729932583ad63118c9ba62f46a1aab'
NOTICES = {
    'RTCW-SP-COPYING.txt': 'COPYING.txt',
    'RTCW-SP-README.txt': 'README.txt',
    'minimp3-LICENSE.txt': 'src/client/minimp3/LICENSE',
    'EAL-LICENSE.txt': 'src/client/al_eal/LICENSE',
}
OPENAL_NOTICES = ('OpenAL-Soft-README.txt', 'openal-soft/COPYING', 'openal-soft/LICENSE-pffft',
                  'openal-soft/fmt-11.2.0/LICENSE', 'openal-soft/gsl/LICENSE')


def sha(data):
    return hashlib.sha256(data).hexdigest()


def windows_text(path):
    return path.read_bytes().replace(b'\r\n', b'\n').replace(b'\n', b'\r\n')


def payload(runtime):
    files = {}
    for name, expected in MODULES.items():
        data = (runtime / name).read_bytes()
        if sha(data) != expected:
            raise ValueError(f'{name} does not match the 724A release')
        files['bin/' + name] = data
    player = windows_text(REPO / 'main/training_player.cfg')
    if sha(player) != PLAYER_SHA:
        raise ValueError('training_player.cfg does not match the installed release')
    files['profile/main/training_player.cfg'] = player
    files['profile/main/wolfconfig.cfg'] = windows_text(HERE / 'wolfconfig.cfg')
    for name in ('Play-Training.cmd', 'Play-Training.ps1', 'START-HERE.md'):
        files[name] = windows_text(HERE / name)
    for name, source in NOTICES.items():
        files['licenses/' + name] = windows_text(REPO / source)
    for name in ('SOURCES.txt', *OPENAL_NOTICES):
        files['licenses/' + name] = windows_text(HERE / 'licenses' / name)
    return files


def package(runtime, output):
    files = payload(runtime)
    output.mkdir(exist_ok=True, parents=True)
    archive = output / ZIP_NAME
    temporary = archive.with_suffix('.tmp')
    with zipfile.ZipFile(temporary, 'w') as bundle:
        for name, data in sorted(files.items()):
            member = zipfile.ZipInfo(name, date_time=(2026, 11, 24, 0, 0, 0))
            member.create_system = 0
            member.external_attr = 0x20
            bundle.writestr(member, data, compress_type=zipfile.ZIP_DEFLATED, compresslevel=9)
    with zipfile.ZipFile(temporary) as bundle:
        if bundle.testzip() or bundle.namelist() != sorted(files):
            raise ValueError('Archive directory or CRC verification failed')
        for name, data in files.items():
            if bundle.read(name) != data:
                raise ValueError(f'Archive content verification failed for {name}')
    temporary.replace(archive)
    digest = sha(archive.read_bytes())
    checksum = archive.with_suffix(archive.suffix + '.sha256')
    checksum.write_bytes(f'{digest}  {archive.name}\n'.encode('ascii'))
    print(f'{digest}  {archive}')
    print(checksum)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--runtime', required=True, type=Path, help='Directory containing the qualified release EXE and DLLs')
    parser.add_argument('--output', required=True, type=Path, help='Directory for ZIP and SHA-256 companion')
    args = parser.parse_args()
    package(args.runtime, args.output)
