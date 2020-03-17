import socket

s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
addr = ("", 65000)
s.bind(addr)
f = s.makefile()

while True:
        data = f.readline()
        print(data)
