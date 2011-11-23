
CXXFLAGS += -I../include -std=c++0x -g -Wall

MAKEFILEDIR = $(lastword $(MAKEFILE_LIST))

all: skype-dbb-read

libskypelog-reader.a: skypelog-reader.o
	$(AR) rcs $@ $^

skype-dbb-read: main.o libskypelog-reader.a
	$(CXX) $(CXXFLAGS) -o $@ $^

clean:
	$(RM) *.o skype-dbb-read

