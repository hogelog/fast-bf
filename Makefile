TARGETS = bf-jit bf-vm-opt bf-jit-opt

CXXFLAGS = -m32 -Wall -W -O2 -fno-operator-names

all: $(TARGETS)

clean:
	rm -f *.o $(TARGETS)
