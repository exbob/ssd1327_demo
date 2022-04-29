ssd1327_demo : ssd1327_demo.c ssd1327.c
	$(CC) $^ -o $@
clean :
	rm ssd1327_demo
install :
	scp ssd1327_demo root@192.168.1.105:/home/root/