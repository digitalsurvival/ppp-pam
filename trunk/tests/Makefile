uuid_test: uuid_test.c sha2.o
	gcc -o $@ -Wall -lssl -luuid sha2.o  $<

sha2.o: ../sha2/sha2.c
	gcc -Wall -o $@ -c $<

clean:
	rm *.o uuid_test
