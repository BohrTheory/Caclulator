#!/usr/bin/env python3
"""Run the candidate GoogleTest suites against explicit producer/UI inputs."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import tarfile
import urllib.request
import xml.etree.ElementTree as ET

GTEST_URL = 'https://codeload.github.com/google/googletest/tar.gz/refs/tags/v1.17.0'
GTEST_SHA256 = '65fab701d9829d38cb77c14acdc431d2108bfdbf8979e40eb8ae567edf10b27c'


def main():
    parser = argparse.ArgumentParser(__doc__)
    for name in ('producer', 'experience', 'archive', 'output'):
        parser.add_argument('--' + name, type=Path, required=True)
    parser.add_argument('--mode', choices=['normal', 'asan-ubsan', 'tsan'], default='normal')
    parser.add_argument('--link', action='append', default=[])
    parser.add_argument('--filter', default='*')
    args = parser.parse_args()
    out = args.output.resolve()
    out.mkdir(parents=True, exist_ok=False)
    package = out / 'googletest.tar.gz'
    urllib.request.urlretrieve(GTEST_URL, package)
    if hashlib.sha256(package.read_bytes()).hexdigest() != GTEST_SHA256:
        raise SystemExit('GoogleTest archive SHA256 mismatch')
    with tarfile.open(package) as archive:
        archive.extractall(out, filter='data')
    gtest = out / 'googletest-1.17.0/googletest'
    flags = {'normal': [], 'asan-ubsan': ['-fsanitize=address,undefined'],
             'tsan': ['-fsanitize=thread']}[args.mode]
    executable = out / 'calculator_tests'
    source = Path(__file__).resolve().parent
    c_object = out / 'c_consumer.o'
    subprocess.run(['clang', '-std=c11', '-Wall', '-Wextra', '-Werror', '-g',
                    '-O1', *flags, '-I' + str(args.producer.resolve() / 'include'),
                    '-c', str(source / 'c_consumer.c'), '-o', str(c_object)], check=True)
    command = ['clang++', '-std=c++17', '-Wall', '-Wextra', '-Werror', '-g', '-O1',
               '-fno-omit-frame-pointer', '-pthread', *flags,
               '-I' + str(args.producer.resolve() / 'include'),
               '-I' + str(args.experience.resolve() / 'ui/src'),
               '-isystem', str(gtest / 'include'), '-I' + str(gtest),
               *[str(source / (name + '.cpp')) for name in ('boundary', 'consumer', 'controller')],
               str(args.experience.resolve() / 'ui/src/adapter.cpp'),
               str(args.experience.resolve() / 'ui/src/controller.cpp'),
               str(gtest / 'src/gtest-all.cc'), str(gtest / 'src/gtest_main.cc'),
               str(c_object), str(args.archive.resolve()), *args.link, '-o', str(executable)]
    provenance = dict(mode=args.mode, filter=args.filter, command=command,
                      googletest=dict(url=GTEST_URL, sha256=GTEST_SHA256),
                      sources={p.name: hashlib.sha256(p.read_bytes()).hexdigest()
                               for p in [source / name for name in
                               ('boundary.cpp', 'consumer.cpp', 'controller.cpp',
                                'c_consumer.c', 'run_gtest.py')]},
                      rustArchiveSHA256=hashlib.sha256(args.archive.read_bytes()).hexdigest())
    (out / 'test-inputs.json').write_text(json.dumps(provenance, indent=2) + '\n')
    print(subprocess.list2cmdline(command), flush=True)
    subprocess.run(command, check=True)
    subprocess.run([str(executable), '--gtest_fail_if_no_test_linked',
                    '--gtest_filter=' + args.filter,
                    '--gtest_output=xml:' + str(out / 'results.xml')], check=True)
    if int(ET.parse(out / 'results.xml').getroot().get('tests', '0')) == 0:
        raise SystemExit('No tests selected')


if __name__ == '__main__':
    main()
