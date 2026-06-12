#!/usr/bin/env bash
set -e

rsync -rvh --no-times --omit-dir-times \
  -e 'ssh -F /dev/null -o StrictHostKeyChecking=no -o UserKnownHostsFile=/tmp/beagle_known_hosts' \
  --exclude 'build/' \
  --exclude 'CMakeFiles/' \
  --exclude 'CMakeCache.txt' \
  --exclude 'cmake_install.cmake' \
  --exclude '*.o' \
  --exclude '*.elf' \
  --exclude '*.bin' \
  --exclude '*.hex' \
  --exclude '*.map' \
  --exclude '__pycache__/' \
  ~/git_workplace/beagle_play/extern/ \
  debian@192.168.7.2:/home/debian/extern/

echo "extern synced to BeaglePlay, build artifacts excluded."
