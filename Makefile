.PHONY = all clean
#Defining Macros
CC = gcc
FLAGS = -Wall -g

all: Sender Receiver
#Creating Program
Sender: Sender.c
	$(CC) $(FLAGS) -o Sender Sender.c
Receiver: Receiver.c
	$(CC) $(FLAGS) -o Receiver Receiver.c

clean:
	rm -f *.o Sender Receiver