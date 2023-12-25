TESTS=$(sort $(basename $(wildcard generated*.lama)))

LAMAC=../src/lamac
MAINC=../build/main

.PHONY: check $(TESTS)

check: $(TESTS)

$(TESTS): %: %.lama
	@echo "regression/deep-expressions/$@"
	@cat $@.input | $(LAMAC) -b $< > $@.bc
	@cat $@.input | $(LAMAC) -i $< > $@.log && diff $@.log orig/$@.log
	@cat $@.input | $(MAINC) $@.bc > $@.log && diff $@.log orig/$@.log
#	@LAMA=../../runtime $(LAMAC) $< && cat $@.input | ./$@ > $@.log && diff $@.log orig/$@.log
#	@cat $@.input | LAMA=../../runtime $(LAMAC) -i $< > $@.log && diff $@.log orig/$@.log
#	@cat $@.input | LAMA=../../runtime $(LAMAC) -s $< > $@.log && diff $@.log orig/$@.log

clean:
	rm -f *.log *.s *~ test*.bc
	find . -maxdepth 1 -type f -not -name '*.*' -not -name 'Makefile' -delete
