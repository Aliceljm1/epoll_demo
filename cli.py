import socket

cli = socket.socket( socket.AF_INET, socket.SOCK_STREAM )

cli.connect(("127.0.0.1", 8888))
#  link socket
while True:
	inputed = raw_input("press any key to continue...")
#get input info
	if len(inputed)==0:
		print "exiting..."
		exit(0)

	cli.send(inputed)
#send msg to server
	print cli.recv(4096)
#get msg from server




