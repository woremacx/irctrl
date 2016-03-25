### Project name (also used for output file name)
PROJECT	= irmon

### Source files and search directory
CSRC    = main.c ir_ctrl.c uart.c
ASRC    = xitoa.S
VPATH   =

### Target device
DEVICE  = atmega32u4

### Include dirs, library dirs and definitions
LIBS	=
LIBDIRS	=
INCDIRS	=
DEFS	= F_CPU=16000000
ADEFS	= $(DEFS)

### Optimization level (0, 1, 2, 3, 4 or s)
OPTIMIZE = s

### C Standard level (c89, gnu89, c99 or gnu99)
CSTD = gnu89

### Warning contorls
WARNINGS = all extra

### Output directory
OBJDIR = obj

### Output file format (hex, bin or both) and debugger type
OUTPUT	= hex
HEXFMT  = srec
DEBUG	= dwarf-2

### Programs to build porject
CC      = avr-gcc
OBJCOPY = avr-objcopy
OBJDUMP = avr-objdump
SIZE    = avr-size
NM      = avr-nm

# Define all object files
COBJ      = $(CSRC:.c=.o) 
AOBJ      = $(ASRC:.S=.o)
COBJ      := $(addprefix $(OBJDIR)/,$(COBJ))
AOBJ      := $(addprefix $(OBJDIR)/,$(AOBJ))
PROJECT   := $(OBJDIR)/$(PROJECT)

# C flags
CFLAGS += -mmcu=$(DEVICE) -std=$(CSTD)
CFLAGS += -g$(DEBUG) -O$(OPTIMIZE) -mcall-prologues
CFLAGS += $(addprefix -W,$(WARNINGS)) $(addprefix -I,$(INCDIRS)) $(addprefix -D,$(DEFS))
CFLAGS += -Wp,-MM,-MP,-MT,$(OBJDIR)/$(*F).o,-MF,$(OBJDIR)/$(*F).d

# Assembler flags
ASFLAGS += -mmcu=$(DEVICE) -I. -x assembler-with-cpp $(addprefix -D,$(ADEFS)) -Wa,-gstabs,-g$(DEBUG)

# Linker flags
LDFLAGS += -Wl,-Map,$(PROJECT).map


# Default target.
all: build size

ifeq ($(OUTPUT),hex)
build: elf hex lst sym
hex: $(PROJECT).hex
else
ifeq ($(OUTPUT),bin)
build: elf bin lst sym
bin: $(PROJECT).bin
else
ifeq ($(OUTPUT),both)
build: elf hex bin lst sym
hex: $(PROJECT).hex
bin: $(PROJECT).bin
else
$(error "Invalid format: $(OUTPUT)")
endif
endif
endif

elf: $(PROJECT).elf
lst: $(PROJECT).lst 
sym: $(PROJECT).sym


# Display compiler version information.
version :
	@$(CC) --version

# Create final output file from ELF output file.
%.hex: %.elf
	@echo
	$(OBJCOPY) -R .eeprom -O ihex $< $@

%.bin: %.elf
	@echo
	$(OBJCOPY) -j .text -j .data -O binary $< $@

# Create extended listing file from ELF output file.
%.lst: %.elf
	@echo
	$(OBJDUMP) -S $< > $@

# Create a symbol table from ELF output file.
%.sym: %.elf
	@echo
	$(NM) -n $< > $@

# Display size of file.
size:
	@echo
	$(SIZE) -C --mcu=$(DEVICE) $(PROJECT).elf


# Link: create ELF output file from object files.
%.elf:  $(AOBJ) $(COBJ)
	@echo
	@echo Linking...
	$(CC) $(CFLAGS) $(AOBJ) $(COBJ) --output $@

# Compile: create object files from C source files.
$(COBJ) : $(OBJDIR)/%.o : %.c
	@echo
	@echo $< :
	$(CC) -c $(CFLAGS) $< -o $@

# Assemble: create object files from assembler source files.
$(AOBJ) : $(OBJDIR)/%.o : %.S
	@echo
	@echo $< :
	$(CC) -c $(ASFLAGS) $< -o $@


# Target: clean project.
clean:
	@echo
	rm -f -r $(OBJDIR) | exit 0


dfu: $(PROJECT).hex
	echo $(PROJECT).hex
	dfu-programmer $(DEVICE) erase
	dfu-programmer $(DEVICE) flash $(PROJECT).hex
	dfu-programmer $(DEVICE) reset

# Include the dependency files.
-include $(shell mkdir $(OBJDIR) 2>/dev/null) $(wildcard $(OBJDIR)/*.d)

