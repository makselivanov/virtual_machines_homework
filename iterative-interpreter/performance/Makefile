TESTS=$(sort $(basename $(wildcard *.lama)))

LAMAC=../src/lamac
MAINC=../build/main

.PHONY: check $(TESTS)

check: $(TESTS)

$(TESTS): %: %.lama
	@echo $@
	$(LAMAC) -b $< > $@.bc
	echo 0 | `which time` -f "$@\t%U" $(LAMAC) -i $<
	`which time` -f "$@\t%U" $(MAINC) $@.bc

clean:
	$(RM) test*.log *.s *~ $(TESTS) *.i