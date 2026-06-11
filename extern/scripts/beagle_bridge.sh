#!/usr/bin/env bash
set -e

BUILD_DIR=/home/debian/extern/beagle_recver/build
LOG_DIR=/home/debian/extern/logs

start() {
  stop
  mkdir -p "$LOG_DIR"
  rm -f /tmp/beagle_freedom_latest.json /tmp/beagle_freedom_latest.json.tmp
  cd "$BUILD_DIR"
  nohup ./beagle_recv_freedom > "$LOG_DIR/beagle_recv_freedom.log" 2>&1 &
  nohup ./beagle_send_host > "$LOG_DIR/beagle_send_host.log" 2>&1 &
  nohup ./beagle_recv_host > "$LOG_DIR/beagle_recv_host.log" 2>&1 &
  echo "beagle bridge started"
}

stop() {
  pkill -f beagle_recv_freedom 2>/dev/null || true
  pkill -f beagle_send_host 2>/dev/null || true
  pkill -f beagle_recv_host 2>/dev/null || true
  echo "beagle bridge stopped"
}

status() {
  pgrep -af 'beagle_recv_freedom|beagle_send_host|beagle_recv_host' || true
}

logs() {
  echo "--- recv freedom ---"
  tail -40 "$LOG_DIR/beagle_recv_freedom.log" 2>/dev/null || true
  echo "--- send host ---"
  tail -40 "$LOG_DIR/beagle_send_host.log" 2>/dev/null || true
  echo "--- recv host ---"
  tail -40 "$LOG_DIR/beagle_recv_host.log" 2>/dev/null || true
  echo "--- latest ---"
  cat /tmp/beagle_freedom_latest.json 2>/dev/null || true
  echo
}

case "$1" in
  start)
    start
    ;;
  stop)
    stop
    ;;
  restart)
    stop
    start
    ;;
  status)
    status
    ;;
  logs)
    logs
    ;;
  *)
    echo "usage: $0 {start|stop|restart|status|logs}"
    exit 1
    ;;
esac
