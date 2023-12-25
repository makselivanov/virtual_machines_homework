DEBUG_FILES=stack-dump-before data-dump-before extra-roots-dump-before heap-dump-before stack-dump-after data-dump-after extra-roots-dump-after heap-dump-after
TESTS=$(sort $(filter-out test111, $(basename $(wildcard test*.lama))))

LAMAC=../src/lamac
MAINC=../build/main

.PHONY: check $(TESTS)

check: ctest111 $(TESTS)

$(TESTS): %: %.lama
	@echo "regression/$@"
	@cat $@.input | $(LAMAC) -b $< > $@.bc
	@cat $@.input | $(LAMAC) -i $< > $@.log && diff $@.log orig/$@.log
	@cat $@.input | $(MAINC) $@.bc > $@.log && diff $@.log orig/$@.log

ctest111:
	@echo "regression/test111"
	$(LAMAC) test111.lama && cat test111.input | ./test111 > test111.log && diff test111.log orig/test111.log

clean:
	$(RM) test*.log *.s *.sm *~ $(TESTS) *.i $(DEBUG_FILES) test111 test*.bc
	$(MAKE) clean -C expressions
	$(MAKE) clean -C deep-expressions
