
CXXFLAGS += -std=c++0x -g -Wall

all: skype-dbb-read

skype-dbb-read: main.o skypelog-reader.o
	$(CXX) $(CXXFLAGS) -o $@ $^

clean:
	$(RM) *.o skype-dbb-read

