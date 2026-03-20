
CC = clang++
LD = clang++

SOURCE_PATH  = source
INCLUDE_PATH = include
OBJ_PATH     = obj

CFLAGS       = -O0 -g -I$(INCLUDE_PATH) -emit-llvm
LD_CFLAGS    = -flto 

SOURCE_FILES = $(wildcard $(SOURCE_PATH)/*.cpp)
BC_FILES     = $(addprefix $(OBJ_PATH)/, $(addsuffix .bc,$(notdir $(basename $(SOURCE_FILES)))))


TARGET = demo
TARGET_BC = $(TARGET).bc

.PHONY:all clean

all: $(TARGET) $(TARGET_BC)

# Link all BC files to target (executable)
$(TARGET): $(BC_FILES)
	@if [ ! -d $(OBJ_PATH) ]; then mkdir $(OBJ_PATH); fi
	$(LD) -o $@ $(BC_FILES) $(LDFLAGS)

# Link all BC files to one BC file 
$(TARGET_BC): $(BC_FILES)
	llvm-link -o $(TARGET_BC) $(BC_FILES) $(LDFLAGS)
	llvm-dis $(TARGET_BC)

# Compile all C files to BC files 
$(OBJ_PATH)/%.bc: $(SOURCE_PATH)/%.cpp
	@if [ ! -d $(OBJ_PATH) ]; then mkdir $(OBJ_PATH); fi
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -rf $(OBJ_PATH) $(TARGET) $(TARGET).bc $(TARGET).ll

