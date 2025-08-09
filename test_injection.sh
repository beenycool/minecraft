#!/bin/bash

echo "=== Testing Minecraft Injection ==="
echo "Current time: $(date)"
echo ""

# Check if Minecraft is running
MC_PID=$(pgrep -f "minecraft|java.*BadlionClient" | head -1)
if [ -z "$MC_PID" ]; then
    echo "ERROR: Minecraft is not running!"
    exit 1
fi

echo "Found Minecraft process: PID $MC_PID"
echo ""

# Check log files
echo "=== Recent injection logs ==="
tail -20 /tmp/minecraft_injectable.log | grep -E "$(date '+%Y-%m-%d %H')" || echo "No recent logs found"
echo ""

echo "=== Recent injector logs ==="
tail -5 /tmp/minecraft_injector.log
echo ""

# Test if we can see SDL events
echo "=== Checking for SDL hook activity ==="
tail -f /tmp/minecraft_injectable.log | grep -E "(SDL|Mouse|Key|AutoClicker)" &
LOG_PID=$!

echo ""
echo "Monitoring logs for 10 seconds..."
echo "Try pressing 'R' key or holding left mouse button in Minecraft"
sleep 10

kill $LOG_PID 2>/dev/null

echo ""
echo "Test complete. Check above for any SDL/Mouse/Key events."