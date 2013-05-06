__author__ = 'hpl'

import socket

# UDP_IP = "192.168.0.42"
# UDP_PORT = 8888
# MESSAGE = "Hello, World!"
#
# print "UDP target IP:", UDP_IP
# print "UDP target port:", UDP_PORT
# print "message:", MESSAGE
#
# sock = socket.socket(socket.AF_INET, # Internet
#                      socket.SOCK_DGRAM) # UDP
# sock.sendto(MESSAGE, (UDP_IP, UDP_PORT))

ip = '192.168.0.72'
port = 8888

sock = socket.socket(socket.AF_INET,socket.SOCK_DGRAM)
sock.bind((ip,port))

while True:
    data, addr = sock.recvfrom(1024)
    #print 'received: ', data, addr
    print data.split(';')
    datalist = data.split(';')

