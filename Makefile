CC     = gcc
CFLAGS = -Wall -Wextra -std=c11 -O2 -g
VECTOR_OBJ = ../lab3/vector/generic.o

# ВАЖНО: Строгий порядок линковки сверху вниз (от вызывающих к реализующим)
OBJ_SHARED = index/search.o \
             index/index.o \
             avl/avl.o \
             rbtree/rbtree.o \
             btree/btree.o \
             posting.o \
             $(VECTOR_OBJ)

.PHONY: all app u_tests test clean

all: app u_tests

# main.o идёт самым первым!
app: main.o $(OBJ_SHARED)
	$(CC) $(CFLAGS) -o app main.o $(OBJ_SHARED) -lpsapi

test_avl: avl/tests.o avl/avl.o posting.o $(VECTOR_OBJ)
	$(CC) $(CFLAGS) -o test_avl avl/tests.o avl/avl.o posting.o $(VECTOR_OBJ)

test_rb: rbtree/tests.o rbtree/rbtree.o posting.o $(VECTOR_OBJ)
	$(CC) $(CFLAGS) -o test_rb rbtree/tests.o rbtree/rbtree.o posting.o $(VECTOR_OBJ)

test_btree: btree/tests.o btree/btree.o posting.o $(VECTOR_OBJ)
	$(CC) $(CFLAGS) -o test_btree btree/tests.o btree/btree.o posting.o $(VECTOR_OBJ)

u_tests: test_avl test_rb test_btree
	./test_avl
	./test_rb
	./test_btree

test: app
	@echo "=== E2E: preprocessing ==="
	mkdir -p data/test
	python3 preprocess.py \
		--input  data/test/Questions.csv \
		--output data/test/docs.jsonl
	@echo "=== E2E: indexing ==="
	./app index --type=avl   --data=data/test/docs.jsonl --index=data/test/idx_avl.txt
	./app index --type=rb    --data=data/test/docs.jsonl --index=data/test/idx_rb.txt
	./app index --type=btree --data=data/test/docs.jsonl --index=data/test/idx_btree.txt
	@echo "=== E2E: searching ==="
	./app search --type=avl   --index=data/test/idx_avl.txt   --json "python list"
	./app search --type=rb    --index=data/test/idx_rb.txt    --json "python list"
	./app search --type=btree --index=data/test/idx_btree.txt --json "python list"
	@echo "=== E2E OK ==="

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f app test_avl test_rb test_btree
	rm -f *.o avl/*.o rbtree/*.o btree/*.o index/*.o ../lab3/vector/*.o
	rm -f data/index_*.txt data/test/docs.jsonl data/test/idx_*.txt