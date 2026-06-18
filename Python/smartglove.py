# socket: lets us open a network connection to the ESP32-CAM over WiFi
import socket
# pynput: lets us simulate keyboard key presses as if the user typed them
from pynput.keyboard import Key, Controller

# ===== ESP32-CAM connection settings =====
# HOST: the IP address the ESP32 prints to Serial Monitor when it connects to WiFi
# PORT: must match the port number in the ESP32 Arduino code (wifiServer(8080))
# Both this laptop and the ESP32 must be on the same WiFi network (phone hotspot)
HOST = "xxx.xxx.xx.xx"   # <-- Change this to your ESP32's actual IP
PORT = 8080

# Create a virtual keyboard — we'll use this to "press" arrow keys and Escape
keyboard = Controller()

# Open a TCP connection to the ESP32-CAM's WiFi server
# This is like calling a phone number — we dial the ESP32's IP and wait for it to pick up
sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
print(f"Connecting to SmartGlove at {HOST}:{PORT}...")
sock.connect((HOST, PORT))   # Blocks here until the ESP32 accepts the connection
print("Connected! Listening for commands... (Ctrl+C to stop)")

# Show the user what each gesture does
print()
print("  Tilt LEFT    = Left arrow  (previous photo)")
print("  Tilt RIGHT   = Right arrow (next photo)")
print("  Tilt FORWARD = Escape      (exit / go back)")
print()

# Main loop — runs until the user presses Ctrl+C or the connection drops
try:
    while True:
        # Wait for one byte from the ESP32 (it sends 'L', 'R', 'F', or 'C')
        # recv(1) blocks here until a character arrives — no busy waiting, no CPU waste
        data = sock.recv(1)

        # If recv returns empty data, the ESP32 disconnected (lost power, WiFi dropped, etc.)
        if not data:
            print("Connection lost")
            break

        # Convert the raw byte to a readable character
        cmd = data.decode('utf-8', errors='ignore')

        # React to each command by simulating the corresponding key press
        if cmd == 'R':
            # Hand tilted right → press right arrow key (next slide/photo)
            keyboard.press(Key.right)
            keyboard.release(Key.right)
            print("-> RIGHT arrow")
        elif cmd == 'L':
            # Hand tilted left → press left arrow key (previous slide/photo)
            keyboard.press(Key.left)
            keyboard.release(Key.left)
            print("<- LEFT arrow")
        elif cmd == 'F':
            # Hand tilted forward (fingers down) → press Escape (exit slideshow)
            keyboard.press(Key.esc)
            keyboard.release(Key.esc)
            print("** ESCAPE (forward tilt)")
        elif cmd == 'C':
            # Hand is flat (center) → do nothing, just log it
            print("-- CENTER (no key press)")

# Ctrl+C was pressed — exit gracefully instead of crashing
except KeyboardInterrupt:
    print("\nStopped by user")

# This runs no matter how we exit (Ctrl+C, connection lost, or error)
# Always close the socket so the ESP32 knows we disconnected cleanly
finally:
    sock.close()
    print("Disconnected")