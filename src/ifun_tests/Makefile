
HEADERS=$(PWD)/include/ \
	$(wildcard $(PWD)/ext/*/include/)
INCLUDE=$(HEADERS:%=-I%)
SOURCES=$(wildcard *.cpp) \
	$(wildcard src/*.cpp) \
	$(wildcard ext/*/src/*.cpp)
OBJECTS=$(SOURCES:%.c=%.o)

CC=g++
#CC=clang++
CFLAGS=-Wall -Wextra -pedantic -std=c++11 -ftree-vectorize -O3
CFLAGS+=-Wno-switch -Wno-reorder -Wno-nested-anon-types
LDFLAGS=-fopenmp
DEFINE=#NDEBUG
DEFINITIONS=$(DEFINE:%=-D%)

ECHO=/bin/echo
NO_COLOR=\x1b[0m
GREEN=\x1b[32;01m
RED=\x1b[31;01m
YELLOW=\x1b[33;01m

.PHONY: help
help:
	@$(ECHO) -e "\nOptions are: $(GREEN)test$(NO_COLOR)\n" \
				"            $(RED)clean$(NO_COLOR)\n"

.PHONY: test
test: $(OBJECTS)
	$(CC) $(CFLAGS) $(DEFINITIONS) $(INCLUDE) -o $@ $^ $(LDFLAGS)
	@$(ECHO) -e "\nExecutable $(GREEN)test$(NO_COLOR) successfully generated.\n"

.PHONY: clean
clean:
	@$(ECHO) -en "$(RED)Cleaning build... $(NO_COLOR)"
	@rm *.o test 2>/dev/null || sleep 0
	@$(ECHO) -e "$(RED)done!$(NO_COLOR)"
