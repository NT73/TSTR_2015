### Makefile du projet TSTR 2015

PROGRAMS = reverb
RM = /bin/rm
SRC_PATH = rtaudio-4.1.1
INCLUDE = rtaudio-4.1.1
OBJECT_PATH = rtaudio-4.1.1/tests/Release
vpath %.o $(OBJECT_PATH)

OBJECTS	=	RtAudio.o 

CC       = g++
DEFS     =   -DHAVE_GETTIMEOFDAY -D__LINUX_ALSA__
CFLAGS   = -O2 -Wall -Wextra
CFLAGS  += -I$(INCLUDE) -I../include
LIBRARY  = -lpthread -lasound 

%.o : $(SRC_PATH)/%.cpp
	$(CC) $(CFLAGS) $(DEFS) -c $(<) -o $(OBJECT_PATH)/$@

%.o : $(INCLUDE)/include/%.cpp
	$(CC) $(CFLAGS) $(DEFS) -c $(<) -o $(OBJECT_PATH)/$@

all : $(PROGRAMS)

reverb : src/reverb.cpp src/somefunc.cpp $(OBJECTS)
	$(CC) $(CFLAGS) $(DEFS) -o reverb src/reverb.cpp src/somefunc.cpp $(OBJECT_PATH)/*.o $(LIBRARY)
	
clean : 
	$(RM) -f $(OBJECT_PATH)/*.o
	$(RM) -f $(PROGRAMS)
	$(RM) -f *.raw *~ *.exe
	$(RM) -fR *.dSYM
