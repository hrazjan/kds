server: src/main.c src/packet_queue.c src/crc.c src/sha256.c
	gcc -o server src/main.c src/packet_queue.c src/crc.c src/sha256.c

client: src/client.c src/packet_queue.c src/crc.c src/sha256.c
	gcc -o client src/client.c src/packet_queue.c src/crc.c src/sha256.c
