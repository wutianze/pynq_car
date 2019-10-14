COLLECT = collect
RUN = run

CXX       :=   g++
CC        :=   gcc 
COLLECTOBJ :=   collect.o control.o
RUNOBJ    :=   run.o control.o

# linking libraries of OpenCV
COLLECTLDFLAGS   =   $(shell pkg-config --libs opencv) -lpthread
RUNLDFLAGS  =  $(COLLECTLDFLAGS) -lhineon -ln2cube -ldputils

CUR_DIR =   $(shell pwd)
SRC     =   $(CUR_DIR)/src
BUILD   =   $(CUR_DIR)/build
VPATH   =   $(SRC)
MODEL   =   $(CUR_DIR)/model/dpu_test.elf

CFLAGS  :=  -O2 -Wall -Wpointer-arith -std=c++11 -ffast-math -mcpu=cortex-a9 -mfloat-abi=hard -mfpu=neon


.PHONY: all clean

#all: $(BUILD) $(PROJECT)

$(COLLECT) : $(COLLECTOBJ)
	$(CXX) $(CFLAGS) $(addprefix $(BUILD)/, $^) -o $(BUILD)/$@ $(COLLECTLDFLAGS)

$(RUN) : $(RUNOBJ)
	$(CXX) $(CFLAGS) $(addprefix $(BUILD)/, $^) $(MODEL) -o $(BUILD)/$@ $(RUNLDFLAGS)

%.o : %.cc
	$(CXX) -c $(CFLAGS) $< -o $(BUILD)/$@

clean:
	$(RM) -rf $(BUILD)

$(BUILD) :
	-mkdir -p $@
