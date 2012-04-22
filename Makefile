TARGETS = bf-jit bf-vm-opt

CXXFLAGS = -m32 -Wall -W -O2 -fno-operator-names

all: $(TARGETS)

clean:
	rm -f *.o $(TARGETS)
