
PROJECT:=swhx

NDLL:=../bin/linux/$(PROJECT).ndll
EXE:=../bin/linux/$(PROJECT)

SRC_PATHS:=../src ../src/linux
SRCS:=$(filter-out boot.c, $(foreach PATH,$(SRC_PATHS), $(wildcard $(PATH)/*.c)))
HEADERS:=$(foreach PATH,$(SRC_PATHS), $(wildcard $(PATH)/*.h))


CC:=gcc -I../src
GTK_FLAGS:=$(shell pkg-config --cflags --libs gtk+-2.0)
X_FLAGS:=-lXt -lXmu -lXp -lXtst
#DIST: NEKO_FLAGS:=-fPIC -shared -L/usr/lib -lneko -lz  -ldl
NEKO_FLAGS:=-fPIC -shared -L/usr/lib/neko -lneko -lz -ldl
LOCAL_FLAGS:=-I../include -I../src/linux -DLINUX -DXP_UNIX=1 -DMOZ_X11 -D_DEBUG

.PHONY: $(NDLL) $(EXE)

all: $(NDLL) $(EXE)

$(NDLL): $(SRCS) $(HEADERS)
	@echo srcs: $(SRCS), headers: $(HEADERS)
	$(CC) $(NEKO_FLAGS) $(GTK_FLAGS) $(X_FLAGS) $(LOCAL_FLAGS) -o $@ $(SRCS)

$(EXE): $(NDLL) ../src/boot.c
	$(CC) -L/usr/lib/neko -lneko -lz -ldl $(GLIB_FLAGS) $(GTK_FLAGS) $(X_FLAGS) $(LOCAL_FLAGS) -o $@ ../src/boot.c
