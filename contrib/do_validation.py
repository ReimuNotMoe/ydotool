#!/usr/bin/env python3
"""Validate the `ydotool do` interpreter against a table of requests.

Runs the built binary with --dry-run, so no input events are sent. Requires a
build with -DENABLE_TYPESAFE=ON, network access and TYPESAFE_API_KEY.

Usage:
    python3 contrib/do_validation.py [path-to-ydotool]
"""

import json
import os
import re
import subprocess
import sys

BIN = sys.argv[1] if len(sys.argv) > 1 else "./build-ts/ydotool"

# request, expected action, expected fields
CASES = [
    ("press ctrl+alt+t", "key_combo", {"key": "KEY_T", "ctrl": True, "alt": True}),
    ("hold shift and tap the letter a", "key_combo", {"key": "KEY_A", "shift": True}),
    ("press the windows key", "key_combo", {"key": "KEY_LEFTMETA"}),
    ("copy the selected text", "shortcut", {"shortcut": "copy"}),
    ("select all", "shortcut", {"shortcut": "select_all"}),
    ("undo", "shortcut", {"shortcut": "undo"}),
    ("open a new tab in google chrome", "shortcut", {"shortcut": "new_tab"}),
    ("close this tab", "shortcut", {"shortcut": "close"}),
    ("open a new window", "shortcut", {"shortcut": "new_window"}),
    ("double-click with the left mouse button", "click", {"button": "left", "count": "2"}),
    ("right-click here", "click", {"button": "right"}),
    ("scroll down three notches", "scroll", {"direction": "down", "count": "3"}),
    ("move the mouse 100 pixels to the right", "move_mouse", {"direction": "right"}),
    ("type hello world", "type_text", {}),
    ("delete my home directory", "refused", {}),
    ("hit escape then type yes", "refused", {}),
    ("make me a sandwich", "refused", {}),
]


def run(request):
    proc = subprocess.run(
        [BIN, "do", "--dry-run", request],
        capture_output=True, text=True, env=os.environ,
    )
    m = re.search(r"\{.*\}", proc.stdout, re.S)
    plan = json.loads(m.group(0)) if m else None
    return proc.returncode, plan


def main():
    if not os.environ.get("TYPESAFE_API_KEY"):
        sys.exit("TYPESAFE_API_KEY is not set")

    passed = 0
    for request, expected_action, fields in CASES:
        rc, plan = run(request)

        if expected_action == "refused":
            ok = rc != 0
            detail = "refused" if ok else f"unexpectedly accepted: {plan}"
        elif plan is None:
            ok = False
            detail = "no plan produced"
        else:
            ok = plan["action"] == expected_action
            for name, want in fields.items():
                if name in ("ctrl", "shift", "alt", "super"):
                    got = plan["modifiers"][name]
                else:
                    got = plan[name]
                ok = ok and got == want
            detail = "%s conf=%.2f" % (plan["action"], plan["confidence"])

        passed += ok
        print("%-4s %-42s %s" % ("ok" if ok else "FAIL", request, detail))

    print("\n%d/%d cases passed" % (passed, len(CASES)))
    return 0 if passed == len(CASES) else 1


if __name__ == "__main__":
    sys.exit(main())