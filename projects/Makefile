
PROJECT:=swhx

NDLL:=../bin/linux/$(PROJECT).ndll

SRC_PATHS:=../src ../src/linux
SRCS:=$(foreach PATH,$(SRC_PATHS), $(wildcard $(PATH)/*.c))
HEADERS:=$(foreach PATH,$(SRC_PATHS), $(wildcard $(PATH)/*.h))

CC:=gcc -I../src
GTK_FLAGS:=$(shell pkg-config --libs --cflags gtk+-2.0)
X_FLAGS:=-lXt -lXmu -lXp -lXtst
NEKO_FLAGS:=-fPIC -shared -L/usr/lib -lneko -lz  -ldl
LOCAL_FLAGS:=-I../include -I../src/linux -DLINUX -DXP_UNIX=1 -DMOZ_X11 -D_DEBUG

$(NDLL): $(SRCS) $(HEADERS)
	@echo srcs: $(SRCS), headers: $(HEADERS)
	$(CC) $(NEKO_FLAGS) $(GTK_FLAGS) $(X_FLAGS) $(LOCAL_FLAGS) -o $@ $(SRCS)
