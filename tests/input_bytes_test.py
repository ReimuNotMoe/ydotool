import os
from pathlib import Path
import socket
import struct
import subprocess
import sys
import tempfile
import threading
import unittest


BINARY = Path(sys.argv.pop(1)).resolve()


class InputByteTests(unittest.TestCase):
    def run_client(self, args, data=None):
        with tempfile.TemporaryDirectory(prefix="ydotool-bytes-") as directory:
            path = str(Path(directory) / "socket")
            with socket.socket(socket.AF_UNIX, socket.SOCK_DGRAM) as receiver:
                receiver.bind(path)
                events = []
                def drain():
                    while True:
                        event = receiver.recv(4096)
                        if event == b"stop":
                            return
                        events.append(event)
                thread = threading.Thread(target=drain)
                thread.start()
                try:
                    result = subprocess.run(
                        [os.fsencode(BINARY), *args], input=data,
                        env={**os.environ, "YDOTOOL_SOCKET": path},
                        capture_output=True, timeout=3,
                    )
                finally:
                    with socket.socket(socket.AF_UNIX, socket.SOCK_DGRAM) as stopper:
                        stopper.sendto(b"stop", path)
                    thread.join()
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

    def test_later_non_ascii_arguments_are_rejected_before_typing(self):
        for args in ([b"type", b"abc", "Ж".encode()],
                     [b"type", b"abc\\x80"],
                     [b"type", b"-e", b"0", b"abc\x80"]):
            with self.subTest(args=args):
                result, events = self.run_client(args)
                self.assertEqual(result.returncode, 2, result.stderr)
                self.assertIn(b"ASCII", result.stderr)
                self.assertEqual(events, [])

    def test_escape_state_does_not_leak_between_arguments(self):
        for incomplete in (bytes([92]), b"\\x", b"\\xf"):
            with self.subTest(incomplete=incomplete):
                result, events = self.run_client([b"type", incomplete, b"a"])
                self.assertEqual(result.returncode, 0, result.stderr)
                codes = [struct.unpack("@llHHi", event)[3] for event in events
                         if struct.unpack("@llHHi", event)[2] == 1]
                self.assertEqual(codes, [30, 30])

    def test_stdin_processes_every_byte(self):
        for data, status, codes in ((b"abc", 0, [30, 30, 48, 48, 46, 46]),
                                    (b"a\x80", 2, [30, 30])):
            with self.subTest(data=data):
                result, events = self.run_client([b"stdin"], data)
                self.assertEqual(result.returncode, status, result.stderr)
                actual = [struct.unpack("@llHHi", event)[3] for event in events
                          if struct.unpack("@llHHi", event)[2] == 1]
                self.assertEqual(actual, codes)

    def test_stdin_arrow_sequences_keep_following_ascii(self):
        for suffix, code in ((b"A", 103), (b"B", 108), (b"C", 106), (b"D", 105)):
            with self.subTest(suffix=suffix):
                result, events = self.run_client([b"stdin"], b"\x1b[" + suffix + b"a")
                self.assertEqual(result.returncode, 0, result.stderr)
                codes = [struct.unpack("@llHHi", event)[3] for event in events
                         if struct.unpack("@llHHi", event)[2] == 1]
                self.assertEqual(codes, [code, code, 30, 30])

    def test_valid_escaped_ascii_still_emits_expected_keys(self):
        result, events = self.run_client([b"type", b"\\x61\\n\\t\\\\"])
        self.assertEqual(result.returncode, 0, result.stderr)
        codes = [struct.unpack("@llHHi", event)[3] for event in events
                 if struct.unpack("@llHHi", event)[2] == 1]
        self.assertEqual(codes, [30, 30, 28, 28, 15, 15, 43, 43])

    def test_file_input_remains_streamed(self):
        result, events = self.run_client([b"type", b"-f", b"-"], b"ab\x80")
        self.assertEqual(result.returncode, 2, result.stderr)
        self.assertIn(b"ASCII", result.stderr)
        codes = [struct.unpack("@llHHi", event)[3] for event in events
                 if struct.unpack("@llHHi", event)[2] == 1]
        self.assertEqual(codes, [30, 30, 48, 48])


if __name__ == "__main__":
    unittest.main()
