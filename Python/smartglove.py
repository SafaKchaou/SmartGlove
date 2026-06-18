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

# Command handling will be added next