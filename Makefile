# Makefile for ssh_popup

CC = gcc
CFLAGS = `pkg-config --cflags gtk+-3.0`
LIBS = `pkg-config --libs gtk+-3.0`
TARGET = ssh_popup
INSTALL_DIR = /usr/local/bin

all: $(TARGET)

$(TARGET): ssh_popup.o
	$(CC) -o $(TARGET) ssh_popup.o $(LIBS)

ssh_popup.o: ssh_popup.c
	$(CC) $(CFLAGS) -c ssh_popup.c

clean:
	rm -f $(TARGET) ssh_popup.o

install: all
	sudo cp $(TARGET) $(INSTALL_DIR)
