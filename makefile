#author=Naris Rangsiyawaranon

BASHPATH:=$(shell which bash)
SHELL:=$(BASHPATH) -O globstar
undefine BASHPATH

CC=clang
CFLAGS=-std=c11 -Wall -Wextra -Werror -pedantic-errors -ferror-limit=3
LDLIBS=-lm -pthread
SANS=-g -fsanitize=address
OPTIMIZE:=-o0 -march=native

# For simple c files
%: %.c
@$(CC) $(SANS) $(CFLAGS) $(OPTIMIZE) $< -o $@


LS_NO_SPACE:=$(shell ls $(1) 2> /dev/null | grep -v ' ')
UNIQ_DIRS:=$(shell echo $(dir $(1))|tr ' ' '\n'|sort|uniq|tr '\n' ' ')

TARGET=./main
BUILDPATH=.build/
SRC:=$(call LS_NO_SPACE, **/*.c)
OBJS:=$(addprefix $(BUILDPATH), $(SRC:.c=.o))
BUILDSUBDIRS:=$(call UNIQ_DIRS, $(OBJS))
TESTPATH=.tests/
TESTS:=$(call LS_NO_SPACE, **/*.in)
RESULTS:=$(addprefix $(TESTPATH), $(TESTS:.in=.out))
TESTSUBDIRS:=$(call UNIQ_DIRS, $(RESULTS))


.PHONY: clean test FORCE 

test: $(TARGET) $(RESULTS)

clean:
	rm -rf $(BUILDPATH) $(TARGET) $(TESTPATH)
	@PATH="$$PATH:$(dir $(shell find / -name llvm-symbolizer 2> /dev/null))"

$(TARGET): $(OBJS)
	@echo linking $@
	@$(CC) $(SANS) $^ $(LDLIBS) -o $@

$(BUILDSUBDIRS) $(TESTSUBDIRS):
	@mkdir -p $@


# allows use of runtime vars in prereq via $$ escape
.SECONDEXPANSION:

# folder timestamp changes when files inside are changed
# fix: | tells make that the prereq is order only
$(BUILDPATH)%.o: %.c $$(wildcard $$(dir $$@)*.h) | $$(dir $$@)
	@echo compiling $@
	@$(CC) $(SANS) $(CFLAGS) $(OPTIMIZE) -c $< -o $@


$(TESTPATH)%.out: %.in %.exp $(TARGET) FORCE | $$(dir $$@)
	@$(TARGET) < $< | tee $@ | diff - $(word 2, $^) 
	@echo passed $(basename $(notdir $@))  

$(TESTPATH)%.out: %.in $(TARGET) FORCE | $$(dir $$@)
	@$(TARGET) < $< > $@
	@echo no errors $(basename $(notdir $@))
