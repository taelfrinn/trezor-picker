
CFLAGS=-Wall -ggdb3
picker: picker.c rand.c

test: picker
	echo unit test 1, variable inputs 24 min, 20 variations
	./picker -u -s 20 < unit_test_data.txt  | ./average.pl
	echo unit test 2: 12 words min, 20 variations
	./picker -m 12 -u -s 20 < unit_test_data.txt  | ./average.pl

validate_3:
	cat wordlist.txt  | cut -b 1-3 | ./count_runs.pl | ./average.pl

.PHONY: test validate_3
