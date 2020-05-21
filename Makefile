xor: xor.c
	gcc -o xor xor.c -fsanitize=address
