all:sender receiver
sender:sender.c
	g++ -o sender sender.c
receiver:receiver.c
	g++ -o receiver receiver.c
clean:
	rm -f sender
	rm -f receiver
