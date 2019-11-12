COLLECT = collect
RUN = run
FPT = fpt

CXX       :=   g++
CC        :=   gcc 
COLLECTOBJ :=   collect.o control.o
RUNOBJ    :=   run.o control.o
FPTOBJ    := run_fpt.o control.o cv_lane.o

# linking libraries of OpenCV
COLLECTLDFLAGS   =   $(shell pkg-config --libs opencv) -lpthread
RUNLDFLAGS  =  $(COLLECTLDFLAGS) -lhineon -ln2cube -ldputils

CUR_DIR =   $(shell pwd)
SRC     =   $(CUR_DIR)/src
BUILD   =   $(CUR_DIR)/build
VPATH   =   $(SRC)
MODEL   =   $(CUR_DIR)/model/dpu_testModel_0.elf
YOLO    =   $(CUR_DIR)/model/yolo_model.elf
ARCH    =     $(shell uname -m | sed -e s/arm.*/armv71/ \
	-e s/aarch64.*/aarch64/ )

CFLAGS  :=  -O2 -Wall -Wpointer-arith -std=c++11 -ffast-math

ifeq ($(ARCH),armv71)
	CFLAGS +=  -mcpu=cortex-a9 -mfloat-abi=hard -mfpu=neon
endif
ifeq ($(ARCH),aarch64)
	CFLAGS += -mcpu=cortex-a53
endif

.PHONY: clean


$(COLLECT) : $(COLLECTOBJ)
	$(CXX) $(CFLAGS) $(addprefix $(BUILD)/, $^) -o $(BUILD)/$@ $(COLLECTLDFLAGS)

$(RUN) : $(RUNOBJ)
	$(CXX) $(CFLAGS) $(addprefix $(BUILD)/, $^) $(MODEL) -o $(BUILD)/$@ $(RUNLDFLAGS)

$(FPT) : $(FPTOBJ)
	$(CXX) $(CFLAGS) $(addprefix $(BUILD)/, $^) -o $(BUILD)/$@ $(RUNLDFLAGS)

%.o : %.cc
	$(CXX) -c $(CFLAGS) $< -o $(BUILD)/$@

clean:
	$(RM) -rf $(BUILD)/*

