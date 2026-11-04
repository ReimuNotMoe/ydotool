import os
from pathlib import Path
import socket
import subprocess
import sys
import tempfile
import unittest


BINARY = Path(sys.argv.pop(1)).resolve()


class InputByteTests(unittest.TestCase):
    def run_client(self, args, data=None):
        with tempfile.TemporaryDirectory(prefix="ydotool-bytes-") as directory:
            path = str(Path(directory) / "socket")
            with socket.socket(socket.AF_UNIX, socket.SOCK_DGRAM) as receiver:
                receiver.bind(path)
                result = subprocess.run(
                    [os.fsencode(BINARY), *args], input=data,
                    env={**os.environ, "YDOTOOL_SOCKET": path},
                    capture_output=True, timeout=3,
                )
                receiver.setblocking(False)
                events = []
                while True:
                    try:
                        events.append(receiver.recv(4096))
                    except BlockingIOError:
                        break
                return result, events

    def test_non_ascii_bytes_are_rejected_without_input_events(self):
        for value in range(128, 256):
            for command in (b"type", b"stdin"):
                with self.subTest(value=value, command=command):
                    args = [command, b"-e", b"0", b"--", bytes([value])] if command == b"type" else [command]
                    result, events = self.run_client(args, bytes([value]) if command == b"stdin" else None)
                    self.assertEqual(result.returncode, 2, result.stderr)
                    self.assertIn(b"ASCII", result.stderr)
                    self.assertEqual(events, [])

    def test_utf8_and_escaped_non_ascii_are_rejected(self):
        for args, data in (([b"type", "Ж".encode()], None),
                           ([b"type", b"\\xff"], None),
                           ([b"type", b"-f", b"-"], "Ж".encode())):
            with self.subTest(args=args):
                result, events = self.run_client(args, data)
                self.assertEqual(result.returncode, 2, result.stderr)
                self.assertEqual(events, [])

    def test_empty_input_and_incomplete_escapes_stop_at_end(self):
        for value in (b"", bytes([92]), b"\\x", b"\\xf"):
            result, events = self.run_client([b"type", value])
            self.assertEqual(result.returncode, 0, result.stderr)
            self.assertEqual(events, [])
        result, events = self.run_client([b"stdin"], b"")
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(events, [])

    def test_ascii_still_emits_key_events(self):
        for args, data in (([b"type", b"a"], None), ([b"stdin"], b"a")):
            result, events = self.run_client(args, data)
            self.assertEqual(result.returncode, 0, result.stderr)
            self.assertEqual(len(events), 4)


if __name__ == "__main__":
    unittest.main()
