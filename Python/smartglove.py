import socket
from pynput.keyboard import Key, Controller

HOST = "192.168.43.205"
PORT = 8080

keyboard = Controller()

sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
print(f"Connecting to SmartGlove at {HOST}:{PORT}...")
sock.connect((HOST, PORT))
print("Connected! Listening for commands... (Ctrl+C to stop)")
print()
print("  Tilt LEFT    = Left arrow  (previous photo)")
print("  Tilt RIGHT   = Right arrow (next photo)")
print("  Tilt FORWARD = Escape      (exit / go back)")
print()

try:
    while True:
        data = sock.recv(1)
        if not data:
            print("Connection lost")
            break
        cmd = data.decode('utf-8', errors='ignore')

        if cmd == 'R':
            keyboard.press(Key.right)
            keyboard.release(Key.right)
            print("-> RIGHT arrow")
        elif cmd == 'L':
            keyboard.press(Key.left)
            keyboard.release(Key.left)
            print("<- LEFT arrow")
        elif cmd == 'F':
            keyboard.press(Key.esc)
            keyboard.release(Key.esc)
            print("** ESCAPE (forward tilt)")
        elif cmd == 'C':
            print("-- CENTER (no key press)")

except KeyboardInterrupt:
    print("\nStopped by user")
finally:
    sock.close()
    print("Disconnected")