CC = g++
CPPFLAGS = -Wall -g -O0
PROGRAM_NAME = fodinha

SOURCE_DIR = src
OBJECTS_DIR = build
SOURCES := $(shell find $(SOURCE_DIR) -name '*.cpp')
OBJECTS := $(patsubst $(SOURCE_DIR)/%.cpp,$(OBJECTS_DIR)/%.o,$(SOURCES))

all: $(PROGRAM_NAME)

run:
	./script.sh

$(PROGRAM_NAME): $(OBJECTS)
	$(CC) $(LDFLAGS) -o $(PROGRAM_NAME) $(OBJECTS)

$(OBJECTS_DIR):
	mkdir -p $(OBJECTS_DIR)

$(OBJECTS_DIR)/%.o: $(SOURCE_DIR)/%.cpp | $(OBJECTS_DIR)
	$(CC) $(CPPFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJECTS_DIR)

purge: clean
	rm -f $(PROGRAM_NAME)
