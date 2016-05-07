#
# Modelovanie a simulacia: system hromadnej obsluhy logistickej firmy
# Vojvoda Jakub, Bendik Lukas
# 2013
#
# -----------------------------------------------------

# Projekt name
HNAME = libstr
CNAME1 = strateg1
CNAME2 = strateg2

#HNAME3 = libstr3
#CNAME3 = strateg3


# C++ compiler
CXX = g++
CXXFLAGS = -g -Wall -Wextra -pedantic

# Linker
LDFLAGS = -lm
LDLIBS = -lsimlib

# Run
ST = ./$(CNAME1); ./$(CNAME2)

# Clean
RM = rm -f

# Debug mode
debug: CXXFLAGS += -DDEBUG

# ---------------------------------------------------------------------

all: $(CNAME1) $(CNAME2)

$(CNAME1): $(CNAME1).cpp $(HNAME).h
	$(CXX) $(CXXFLAGS) -o $(CNAME1) $(CNAME1).cpp $(LDFLAGS) $(LDLIBS)

$(CNAME2): $(CNAME2).cpp $(HNAME).h
	$(CXX) $(CXXFLAGS) -o $(CNAME2) $(CNAME2).cpp $(LDFLAGS) $(LDLIBS)

#$(CNAME3): $(CNAME3).cpp $(HNAME3).h
#	$(CXX) $(CXXFLAGS) -o $(CNAME3) $(CNAME3).cpp $(LDFLAGS) $(LDLIBS)


debug: $(CNAME1) $(CNAME2)

run: 
	$(ST)

clean: 
	$(RM) $(CNAME1) $(CNAME2) *~
