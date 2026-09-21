#!/usr/bin/env python3
"""Smoke-test the `ydotool mcp` server over raw stdio JSON-RPC.

No MCP SDK or desktop session is needed: the script binds its own datagram
socket as a stand-in for ydotoold and checks the emitted input events.

Usage:
    python3 contrib/mcp_test.py [path-to-ydotool]

Requires a build with -DENABLE_MCP=ON.
"""

import json
import os
import socket
import struct
import subprocess
import sys
import tempfile
import threading

BIN = sys.argv[1] if len(sys.argv) > 1 else "./build-mcp/ydotool"

EVENT_NAMES = {0: "SYN", 1: "KEY", 2: "REL"}
SYN = (0, 0, 0)


def start_capture(path):
    sock = socket.socket(socket.AF_UNIX, socket.SOCK_DGRAM)
    try:
        os.unlink(path)
    except FileNotFoundError:
        pass
    sock.bind(path)
    sock.settimeout(5)

    events = []

    def receive():
        while True:
            try:
                data, _ = sock.recvfrom(64)
            except socket.timeout:
                break
            if len(data) >= 24:
                _, _, typ, code, val = struct.unpack("<qqHHi", data[:24])
                events.append((typ, code, val))

    thread = threading.Thread(target=receive, daemon=True)
    thread.start()
    return sock, events, thread


def run(reqs, args=(), env=None):
    payload = "\n".join(json.dumps(r) for r in reqs) + "\n"
    proc = subprocess.run(
        [BIN, "mcp", *args],
        input=payload,
        capture_output=True,
        text=True,
        timeout=60,
        env=env,
    )
    out = [json.loads(line) for line in proc.stdout.splitlines() if line.strip()]
    return out, proc


def content_text(response):
    result = response.get("result", {})
    items = result.get("content") or []
    for item in items:
        if item.get("type") == "text":
            return item.get("text", "")
    return ""


def main():
    failures = 0

    def check(name, ok, detail=""):
        nonlocal failures
        if not ok:
            failures += 1
        print("%-4s %-42s %s" % ("ok" if ok else "FAIL", name, detail))

    # Protocol basics, no desktop session needed.
    out, _ = run([
        {"jsonrpc": "2.0", "id": 1, "method": "initialize",
         "params": {"protocolVersion": "2025-06-18", "capabilities": {},
                    "clientInfo": {"name": "mcp_test", "version": "0"}}},
        {"jsonrpc": "2.0", "method": "notifications/initialized"},
        {"jsonrpc": "2.0", "id": 2, "method": "tools/list"},
        {"jsonrpc": "2.0", "id": 3, "method": "ping"},
        {"jsonrpc": "2.0", "id": 4, "method": "tools/call",
         "params": {"name": "no_such_tool"}},
    ], args=["--perception=none"])

    check("initialize negotiates version",
          len(out) > 0 and out[0].get("result", {}).get("protocolVersion") == "2025-06-18")

    names = []
    for response in out:
        if "tools" in response.get("result", {}):
            names = [t["name"] for t in response["result"]["tools"]]

    for required in ("computer_get_state", "computer_screenshot", "computer_type",
                     "computer_press", "computer_click", "computer_scroll",
                     "computer_move", "computer_focus_window"):
        check("tools/list has %s" % required, required in names)

    check("ping answers", any(o.get("id") == 3 and o.get("result") == {} for o in out))
    check("unknown tool is an error",
          any(o.get("id") == 4 and "error" in o for o in out))

    # Modern hosts may skip initialize; tools/list must still answer.
    out, _ = run([{"jsonrpc": "2.0", "id": 1, "method": "tools/list"}],
                 args=["--perception=none"])
    check("tools/list without initialize", len(out) == 1 and "tools" in out[0]["result"])

    # Malformed input.
    proc = subprocess.run([BIN, "mcp", "--perception=none"], input="not json\n",
                          capture_output=True, text=True, timeout=30)
    out = [json.loads(line) for line in proc.stdout.splitlines() if line.strip()]
    check("parse error is reported",
          len(out) == 1 and out[0].get("error", {}).get("code") == -32700)

    # Dry-run must not emit anything.
    with tempfile.TemporaryDirectory() as tmp:
        sock, events, thread = start_capture(os.path.join(tmp, "ydotool.sock"))
        env = dict(os.environ, YDOTOOL_SOCKET=sock.getsockname())

        out, _ = run([
            {"jsonrpc": "2.0", "id": 1, "method": "tools/call",
             "params": {"name": "computer_press", "arguments": {"keys": ["ctrl", "alt", "t"]}}},
            {"jsonrpc": "2.0", "id": 2, "method": "tools/call",
             "params": {"name": "computer_press",
                        "arguments": {"keys": ["ctrl", "alt", "delete"]}}},
            {"jsonrpc": "2.0", "id": 3, "method": "tools/call",
             "params": {"name": "computer_press", "arguments": {"keys": ["nope"]}}},
        ], args=["--perception=none", "--dry-run"], env=env)

        check("dry-run reports the plan", "dry_run" in content_text(out[0]))
        check("dry-run emits nothing", events == [])
        check("dangerous combo refused", out[1].get("result", {}).get("isError") is True)
        check("unknown key refused", out[2].get("result", {}).get("isError") is True)

        # Real actions against the capture socket.
        out, _ = run([
            {"jsonrpc": "2.0", "id": 1, "method": "tools/call",
             "params": {"name": "computer_press", "arguments": {"keys": ["ctrl", "alt", "t"]}}},
            {"jsonrpc": "2.0", "id": 2, "method": "tools/call",
             "params": {"name": "computer_type", "arguments": {"text": "ab"}}},
            {"jsonrpc": "2.0", "id": 3, "method": "tools/call",
             "params": {"name": "computer_click", "arguments": {"button": "right", "count": 2}}},
            {"jsonrpc": "2.0", "id": 4, "method": "tools/call",
             "params": {"name": "computer_scroll", "arguments": {"direction": "down", "amount": 3}}},
            {"jsonrpc": "2.0", "id": 5, "method": "tools/call",
             "params": {"name": "computer_move", "arguments": {"dx": 100}}},
            {"jsonrpc": "2.0", "id": 6, "method": "tools/call",
             "params": {"name": "computer_key", "arguments": {"key": "esc", "action": "press"}}},
        ], args=["--perception=none"], env=env)

        thread.join(timeout=6)

        expected = [
            (1, 29, 1), (1, 56, 1), (1, 20, 1), (1, 20, 0), (1, 56, 0), (1, 29, 0),
            (1, 30, 1), (1, 30, 0), (1, 48, 1), (1, 48, 0),
            (1, 273, 1), (1, 273, 0), (1, 273, 1), (1, 273, 0),
            (2, 8, -3),
            (2, 0, 100),
            (1, 1, 1), (1, 1, 0),
        ]
        got = [e for e in events if e != SYN]
        check("emitted events match", got == expected,
              "" if got == expected else "got %s" % got[:8])

        # Read-only mode hides and refuses actions.
        out, _ = run([
            {"jsonrpc": "2.0", "id": 1, "method": "tools/list"},
            {"jsonrpc": "2.0", "id": 2, "method": "tools/call",
             "params": {"name": "computer_press", "arguments": {"keys": ["a"]}}},
        ], args=["--perception=none", "--read-only"], env=env)

        listed = [t["name"] for t in out[0]["result"]["tools"]]
        check("read-only lists perception only",
              "computer_press" not in listed and "computer_get_state" in listed)
        check("read-only refuses actions",
              out[1].get("result", {}).get("isError") is True)

        # require-focus without perception refuses actions.
        out, _ = run([
            {"jsonrpc": "2.0", "id": 1, "method": "tools/call",
             "params": {"name": "computer_press", "arguments": {"keys": ["a"]}}},
        ], args=["--perception=none", "--require-focus"], env=env)
        check("require-focus refuses unknown focus",
              out[0].get("result", {}).get("isError") is True)

    print()
    print("%d checks failed" % failures if failures else "all checks passed")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())