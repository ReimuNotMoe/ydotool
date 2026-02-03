#!/usr/bin/env bash
set -e

BUILD=build

case "$1" in
  install)
    mkdir -p "$BUILD"
    cmake -S . -B "$BUILD"
    cmake --build "$BUILD" -j"$(nproc)"
    sudo cmake --install "$BUILD"
    ;;

  uninstall)
    [ -f "$BUILD/install_manifest.txt" ] || {
      echo "No install_manifest.txt found"
      exit 1
    }
    sudo xargs rm -f < "$BUILD/install_manifest.txt"
    systemctl --user daemon-reload || true
    ;;

  *)
    echo "Usage: $0 {install|uninstall}"
    exit 1
    ;;
esac
