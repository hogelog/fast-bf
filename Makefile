TARGETS = bf-jit bf-vm-opt bf-jit-opt bf-jit-opt2

CXXFLAGS = -m32 -Wall -W -O2 -fno-operator-names

all: $(TARGETS)

clean:
	rm -f *.o $(TARGETS)
