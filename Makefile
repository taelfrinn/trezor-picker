
CFLAGS=-Wall -ggdb3
picker: picker.c rand.c

test: picker
	echo unit test 1, variable inputs 24 min, 20 variations
	./picker -u -s 20 < unit_test_data.txt  | ./average.pl
	echo unit test 2: 12 words min, 20 variations
	./picker -m 12 -u -s 20 < unit_test_data.txt  | ./average.pl

.PHONY: test
