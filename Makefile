vpath %.c ../src

CC = gcc
CFLAGS = -c -Wall -std=gnu99
LDFLAGS =

DSTDIR := /usr/local
OBJDIR := obj
SRCDIR := src

SRC = clevo-indicator.c
OBJ = $(patsubst %.c,$(OBJDIR)/%.o,$(SRC)) 

TARGET = bin/clevo-indicator

CFLAGS += `pkg-config --cflags gio-2.0 dbusmenu-glib-0.4 cairo`
LDFLAGS += `pkg-config --libs gio-2.0 dbusmenu-glib-0.4 cairo`

all: $(TARGET)

install: $(TARGET)
	@echo Install to ${DSTDIR}/bin/
	@sudo install -m 4750 -g adm $(TARGET) ${DSTDIR}/bin/

test: $(TARGET)
	@sudo chown root $(TARGET)
	@sudo chgrp adm  $(TARGET)
	@sudo chmod 4750 $(TARGET)

$(TARGET): $(OBJ) Makefile
	@mkdir -p bin
	@echo linking $(TARGET) from $(OBJ)
	@$(CC) $(OBJ) -o $(TARGET) $(LDFLAGS) -lm

clean:
	rm $(OBJ) $(TARGET)

$(OBJDIR)/%.o : $(SRCDIR)/%.c Makefile
	@echo compiling $< 
	@mkdir -p obj
	@$(CC) $(CFLAGS) -c $< -o $@

#$(OBJECTS): | obj

#obj:
#	@mkdir -p $@
