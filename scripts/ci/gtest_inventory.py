"""Execute upstream GoogleTest and require exact discovery/report inventories."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import xml.etree.ElementTree as ET


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def verify_run(binary, inventory, target, output):
    binary, inventory, output = map(Path, (binary, inventory, output))
    binary = binary.resolve()
    output.mkdir(parents=True, exist_ok=False)
    evidence = dict(schemaVersion=1, target=target, status='failed', phase='inventory',
                    candidate=os.getenv('CANDIDATE_SHA'), workflow=os.getenv('WORKFLOW_SHA'),
                    base=os.getenv('BASE_SHA'), commands=[])
    try:
        document = json.loads(inventory.read_text())
        spec = document.get('targets', {}).get(target)
        if document.get('schemaVersion') != 1 or not spec or not spec.get('cases'):
            raise ValueError('Missing required target/cases: ' + target)
        expected = set(spec['cases'])
        if any('DISABLED_' in case or not oracle for case, oracle in spec['cases'].items()):
            raise ValueError('Disabled case or missing oracle in inventory')
        evidence.update(binarySHA256=digest(binary), inventorySHA256=digest(inventory))
        # Environment filters/sharding/repeats must never change required execution.
        env = {k: v for k, v in os.environ.items() if not k.startswith('GTEST_')}

        def run(phase, flags):
            evidence['phase'] = phase
            command = [str(binary), *flags]
            result = subprocess.run(command, env=env, capture_output=True)
            log = output / (phase + '.log')
            log.write_bytes(result.stdout + b'\nSTDERR:\n' + result.stderr)
            evidence['commands'].append(dict(argv=command, cwd=str(Path.cwd()),
                exit=result.returncode, log=log.name, logSHA256=digest(log)))
            if result.returncode:
                raise ValueError(f'{phase} exit {result.returncode}')
            return result.stdout.decode('utf-8')

        listing = run('discovery', ['--gtest_list_tests'])
        (output / 'discovery.txt').write_text(listing)
        suite, discovered = '', []
        for line in listing.splitlines():
            name = line.split('#', 1)[0].strip()
            if not name:
                continue
            if not line[0].isspace():
                suite = name if name.endswith('.') else ''
            elif suite:
                discovered.append(suite + name)
        if len(discovered) != len(set(discovered)) or set(discovered) != expected:
            raise ValueError('GoogleTest discovery inventory mismatch')
        report = output.resolve() / 'results.xml'
        run('execution', ['--gtest_filter=*', '--gtest_repeat=1',
                          '--gtest_output=xml:' + str(report)])
        evidence['phase'] = 'XML'
        try:
            root = ET.parse(report).getroot()
            if root.tag != 'testsuites' or int(root.attrib['tests']) != len(expected):
                raise ValueError('Wrong root/count')
            suites = root.findall('testsuite')
            cases = root.findall('./testsuite/testcase')
            actual = [c.attrib['classname'] + '.' + c.attrib['name'] for c in cases]
            for node in [root, *suites]:
                if any(int(node.attrib[key]) != 0 for key in ('failures', 'disabled', 'errors')):
                    raise ValueError('Nonzero failure/disabled/error count')
                if int(node.get('skipped', '0')) != 0:
                    raise ValueError('Nonzero skipped count')
                if int(node.attrib['tests']) != len(cases if node is root else node.findall('testcase')):
                    raise ValueError('Inconsistent count')
            if (not cases or len(actual) != len(set(actual)) or set(actual) != expected
                    or any(c.get('status') != 'run' or c.get('result') != 'completed'
                           or len(c) for c in cases)):
                raise ValueError('Missing, duplicate, failed or skipped case')
        except (OSError, ET.ParseError, KeyError, ValueError) as error:
            raise ValueError('Invalid GoogleTest XML: ' + str(error)) from error
        evidence.update(status='passed', phase='complete', required=len(expected),
                        executed=len(actual), skipped=0, reportSHA256=digest(report))
    except Exception as error:
        evidence['error'] = str(error)
        raise
    finally:
        (output / 'test-evidence.json').write_text(json.dumps(evidence, indent=2) + '\n')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(__doc__)
    for name in ('binary', 'inventory', 'target', 'output'):
        parser.add_argument('--' + name, required=True)
    args = parser.parse_args()
    verify_run(args.binary, args.inventory, args.target, args.output)
