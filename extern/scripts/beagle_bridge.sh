#!/usr/bin/env bash
set -e

APP_DIR=/home/debian/extern/beagle_gateway
BUILD_DIR="$APP_DIR/build"
LOG_DIR=/home/debian/extern/logs
APP="$BUILD_DIR/beagle_gateway"
PWM_DIR=/sys/class/pwm/pwmchip0/pwm0

build() {
  cmake -S "$APP_DIR" -B "$BUILD_DIR"
  cmake --build "$BUILD_DIR"
}

start() {
  stop
  mkdir -p "$LOG_DIR"
  rm -f /tmp/beagle_freedom_latest.json /tmp/beagle_freedom_latest.json.tmp
  build
  if [ "$(id -u)" != "0" ]; then
    echo "warning: not running as root, fan PWM may fail"
  fi
  nohup "$APP" > "$LOG_DIR/beagle_gateway.log" 2>&1 &
  echo "beagle bridge started"
}

write_pwm() {
  local file="$1"
  local value="$2"

  if [ -w "$file" ]; then
    echo "$value" > "$file" || true
    return
  fi

  if command -v sudo >/dev/null 2>&1 && sudo -n test -w "$file" 2>/dev/null; then
    echo "$value" | sudo tee "$file" >/dev/null || true
  fi
}

fan_off() {
  if [ -d "$PWM_DIR" ]; then
    write_pwm "$PWM_DIR/duty_cycle" 0
    write_pwm "$PWM_DIR/enable" 0
  fi
}

stop() {
  pkill -x beagle_gateway 2>/dev/null || true
  pkill -f beagle_recv_freedom 2>/dev/null || true
  pkill -f beagle_send_host 2>/dev/null || true
  pkill -f beagle_recv_host 2>/dev/null || true
  fan_off
  echo "beagle bridge stopped"
}

status() {
  pgrep -af 'beagle_gateway|beagle_recv_freedom|beagle_send_host|beagle_recv_host' || true
}

logs() {
  tail -120 "$LOG_DIR/beagle_gateway.log" 2>/dev/null || true
}

case "$1" in
  build)
    build
    ;;
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
    echo "usage: $0 {build|start|stop|restart|status|logs}"
    exit 1
    ;;
esac
