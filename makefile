SHELL=/bin/bash -O globstar

CC=clang
CFLAGS=-std=c11 -Wall -Wextra -Werror -pedantic-errors -ferror-limit=3
LDLIBS=-lm -pthread
SANS=-g -fsanitize=address
OPTIMIZE=-o0 -march=native

TARGET=./atoms
BUILDPATH=.build/
SRC=$(shell ls **/*.c 2> /dev/null | grep -v ' ')
OBJS=$(addprefix $(BUILDPATH), $(SRC:.c=.o))
BUILDSUBDIRS=$(shell echo $(dir $(OBJS))|sed 's/ /\n/g'|sort|uniq|sed 's/\n/ /g')

TESTS=$( shell ls **/*.in 2> /dev/null )
RESULTS=$(TESTS:.in=.out)

.PHONY: all clean FORCE panic
all: $(TARGET)

test: $(RESULTS)

clean:
	rm -rf $(BUILDPATH) $(TARGET) $(RESULTS)

$(TARGET): $(OBJS)
	@echo linking $@
	@$(CC) $(SANS) $^ $(LDLIBS) -o $@

$(BUILDSUBDIRS):
	mkdir -p $@

# implicit rules for files with matching pattern and required prereq
%.out: %.in %.exp $(TARGET) FORCE
	@$(TARGET) < $< | tee $@ | diff - $(word 2, $^)
	@echo passed $(basename $(notdir $@))

%.out: %.in $(TARGET) FORCE
	@$(TARGET) < $< > $@
	@echo no errors $(basename $(notdir $@))

# allows use of runtime vars in prereq via $$ escape
.SECONDEXPANSION:

# folder timestamp changes when files inside are changed
# fix: | tells make that the prereq is order only
$(BUILDPATH)%.o: %.c $$(wildcard $$(dir $$@)*.h) | $$(dir $$@)
	@echo building $@
	@$(CC) $(SANS) $(CFLAGS) $(OPTIMIZE) -c $< -o $@
