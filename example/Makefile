UNAME := $(shell uname -s)

TARGET = demo
SRC = demo.c

RAYLIB_DIR = raylib
RAYLIB_SRC = $(RAYLIB_DIR)/src

ifeq ($(UNAME), Darwin)
    CC = clang
    CFLAGS = -Wall -Wextra -O2
    CFLAGS += -I$(RAYLIB_SRC)
    LDFLAGS = -L$(RAYLIB_SRC) -lraylib
    LDFLAGS += -framework Foundation -framework Cocoa -framework IOKit
    LDFLAGS += -framework OpenGL -lobjc
    OUTPUT = $(TARGET)
endif

EMCC = emcc
EM_FLAGS = -O2 -Wall -DPLATFORM_WEB
EM_FLAGS += -I$(RAYLIB_SRC)
EM_LDFLAGS = $(RAYLIB_SRC)/libraylib.web.a
EM_LDFLAGS += -sFORCE_FILESYSTEM=1
EM_LDFLAGS += -sEXPORTED_RUNTIME_METHODS='["ccall"]'
EM_LDFLAGS += -sEXPORTED_FUNCTIONS='["_pick__deliver_single","_pick__deliver_multi_lines","_pick__deliver_msg","_main"]'
EM_LDFLAGS += -sUSE_GLFW=3
EM_LDFLAGS += -sASYNCIFY
EM_LDFLAGS += -sTOTAL_MEMORY=67108864
EM_LDFLAGS += -sALLOW_MEMORY_GROWTH=1
EM_OUTPUT = $(TARGET).html

all: native

raylib-native:
	@cd $(RAYLIB_SRC) && make clean
	@cd $(RAYLIB_SRC) && make PLATFORM=PLATFORM_DESKTOP

raylib-web:
	@cd $(RAYLIB_SRC) && make clean
	@cd $(RAYLIB_SRC) && make PLATFORM=PLATFORM_WEB

native: $(SRC) pick.h raylib-native
	$(CC) $(CFLAGS) -x objective-c $(SRC) $(LDFLAGS) -o $(OUTPUT)

web: $(SRC) pick.h raylib-web
	@if [ -f shell.html ]; then \
		$(EMCC) $(EM_FLAGS) $(SRC) $(EM_LDFLAGS) --shell-file shell.html -o $(EM_OUTPUT); \
	else \
		$(EMCC) $(EM_FLAGS) $(SRC) $(EM_LDFLAGS) -o $(EM_OUTPUT); \
	fi

clean:
	rm -f $(OUTPUT) $(EM_OUTPUT) $(TARGET).js $(TARGET).wasm $(TARGET).data

.PHONY: all native web clean raylib-native raylib-web