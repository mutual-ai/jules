SUFFIXES += .d

SRC =
OBJ := $(SRC:.cpp=.o) test_suite.o
DEP := $(SRC:.cpp=.d) test_suite.d

CXXFLAGS += -std=c++1z -O2 -DNDEBUG -Wall -Wextra -I..

NODEPS = clean

all: test_suite

ifeq (0, $(words $(findstring $(MAKECMDGOALS), $(NODEPS))))
-include $(DEP)
endif

test: test_suite
	@echo "Running test suite..."
	@./test_suite

test_suite: $(OBJ)
	@echo "Linking test suite..."
	@$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $(OBJ)

%.o: %.cpp
	@echo "Compiling ($<)..."
	@$(CXX) $(CXXFLAGS) -c -o $@ $<

%.d: %.cpp
	@echo "Finding dependencies ($<)..."
	@$(CXX) $(CXXFLAGS) -MM -MT '$(patsubst %.cpp,%.o,$<)' $< -MF $@

clean:
	@echo "Cleaning test..."
	@find . -iname '*.[od]' -exec rm {} \;

.PHONY: test clean
