server: src/main.c src/packet_queue.c src/crc.c
	gcc -o server src/main.c src/packet_queue.c src/crc.c

client: src/client.c src/packet_queue.c src/crc.c
	gcc -o client src/client.c src/packet_queue.c src/crc.c