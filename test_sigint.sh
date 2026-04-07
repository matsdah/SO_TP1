#!/bin/bash

# Limpiar memoria compartida antes de empezar
rm -f /dev/shm/game_state /dev/shm/game_sync /dev/shm/sem.*

echo "Starting game..."
/tmp/SO_TP1_bin/master -w 10 -h 10 -t 60 -d 100 -v /tmp/SO_TP1_bin/vista -p /tmp/SO_TP1_bin/jugador /tmp/SO_TP1_bin/jugador &
MASTER_PID=$!

# Esperar 2 segundos
sleep 2

echo ""
echo "Sending SIGINT to master (PID: $MASTER_PID)..."
kill -INT $MASTER_PID

# Esperar a que termine
wait $MASTER_PID
EXIT_CODE=$?

echo ""
echo "Master exited with code: $EXIT_CODE"
echo ""
echo "Checking shared memory files..."
ls -la /dev/shm/ | grep -E 'game_|sem\.' || echo "✓ No shared memory files found (cleanup successful!)"
