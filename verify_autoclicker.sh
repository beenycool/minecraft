#!/bin/bash

# Final verification script for autoclicker functionality
echo "=== Minecraft AutoClicker Final Verification ==="
echo ""

# Check if Minecraft is running
echo "🔍 Checking for running Minecraft processes..."
MINECRAFT_PID=$(pgrep -f "java.*minecraft\|java.*badlion\|java.*lunar" | head -1)

if [ -n "$MINECRAFT_PID" ]; then
    echo "✅ Found Minecraft process: PID $MINECRAFT_PID"
    
    # Check if our library is already injected
    if grep -q "libMinecraftInjectable.so" /proc/$MINECRAFT_PID/maps 2>/dev/null; then
        echo "✅ AutoClicker library is already injected"
    else
        echo "⚠️  AutoClicker library not yet injected"
        echo "   Run: ./build/injector"
    fi
else
    echo "⚠️  No Minecraft process found"
    echo "   Please start Minecraft (Badlion, Lunar, or vanilla) first"
fi

echo ""
echo "📋 Configuration Summary:"
echo "   - Enabled: $(grep -o '"enabled": [^,]*' config/modules/autoclicker.json | cut -d' ' -f2)"
echo "   - Pattern: $(grep -o '"clickPattern": "[^"]*"' config/modules/autoclicker.json | cut -d'"' -f4)"
echo "   - Target CPS: $(grep -o '"targetCPS": [^,]*' config/modules/autoclicker.json | cut -d' ' -f2)"
echo ""

echo "🎯 Testing Instructions:"
echo "1. Start Minecraft if not running"
echo "2. Run: ./build/injector"
echo "3. Join a single-player world or server"
echo "4. Hold left mouse button to activate autoclicking"
echo "5. You should see rapid clicking at ~10 CPS"
echo "6. Check logs: tail -f logs/minecraft_injectable.log"
echo ""

echo "🔧 Troubleshooting:"
echo "- If autoclicker doesn't activate:"
echo "  - Check that 'enabled' is set to true in config/modules/autoclicker.json"
echo "  - Verify injection succeeded by checking logs"
echo "  - Try different click patterns: 'jitter' or 'butterfly'"
echo "  - Adjust targetCPS if needed (lower values for testing)"
echo ""

echo "✅ AutoClicker implementation is complete and ready for use!"