#!/bin/bash
# Watches for the ESP32-S3 serial port and automatically compiles + flashes
# esp32_marauder whenever the board is (re)connected. Runs indefinitely.
export PATH="$HOME/.local/bin:$PATH"
FQBN="esp32:esp32:esp32s3:PartitionScheme=app3M_fat9M_16MB,FlashSize=16M,PSRAM=opi"
PROJECT_DIR="$HOME/Desktop/esp32-fustool/esp32_marauder"
LIBDIR="$HOME/Arduino/libraries"

last_port=""

echo "[auto-flash] izleniyor... (cikmak icin Ctrl+C)"

while true; do
  port=""
  for p in /dev/ttyACM0 /dev/ttyACM1 /dev/ttyUSB0 /dev/ttyUSB1; do
    if [ -e "$p" ]; then
      port="$p"
      break
    fi
  done

  if [ -n "$port" ] && [ "$port" != "$last_port" ]; then
    echo "[auto-flash] cihaz algilandi: $port, 2sn bekleniyor..."
    sleep 2
    echo "[auto-flash] derleniyor ve flashlaniyor..."
    cd "$PROJECT_DIR" || exit 1
    if arduino-cli compile --fqbn "$FQBN" --warnings none --libraries "$LIBDIR" --upload --port "$port" .; then
      echo "[auto-flash] basarili: $(date)"
    else
      echo "[auto-flash] HATA: derleme/flash basarisiz: $(date)"
    fi
    last_port="$port"
  elif [ -z "$port" ]; then
    last_port=""
  fi

  sleep 2
done
