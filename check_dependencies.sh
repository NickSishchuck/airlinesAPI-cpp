#!/bin/bash

echo "==== Checking Dependencies ===="
echo "Checking for MariaDB client libraries..."
if ldconfig -p | grep -q mariadb; then
  echo "[OK] MariaDB client libraries found"
else
  echo "[FAIL] MariaDB client libraries not found"
  echo "Install with: sudo pacman -S mariadb-libs"
fi

echo "Checking for Boost libraries..."
if ldconfig -p | grep -q boost; then
  echo "[OK] Boost libraries found"
else
  echo "[FAIL] Boost libraries not found"
  echo "Install with: sudo pacman -S boost"
fi

echo "Checking for nlohmann_json..."
if pacman -Q nlohmann-json &>/dev/null; then
  echo "[OK] nlohmann_json found"
else
  echo "[FAIL] nlohmann_json not found"
  echo "Install with: sudo pacman -S nlohmann-json"
fi

echo "Checking for Crow dependency (if installed manually)..."
if ls /usr/include/crow &>/dev/null; then
  echo "[OK] Crow headers found"
else
  echo "[FAIL] Crow headers not found in system path"
  echo "Install Crow from AUR or manually"
fi

echo "==== Check binary dependencies ===="
echo "Running ldd on your binary to show all dependencies:"
ldd ./airline_api | grep "not found" && echo "[FAIL] Missing libraries detected" || echo "[OK] All libraries resolved"

echo "==== Checking for permission issues ===="
echo "Current directory permissions:"
ls -la

echo "==== Checking for file access issues ===="
if [ -f config.json ]; then
  echo "[OK] config.json exists"
else
  echo "[FAIL] config.json missing"
  echo "Create config.json in the build directory"
fi

echo "==== Running with strace to debug ===="
echo "Running with strace to see where it's crashing (first 20 lines):"
strace ./airline_api 2>&1 | head -20
