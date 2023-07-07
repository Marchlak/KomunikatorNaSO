CC=gcc
CFLAGS=-Wall -g
TARGET=komunikator
OBJ_DIR=build

all: $(TARGET)

$(TARGET): $(OBJ_DIR)/main.o $(OBJ_DIR)/user.o $(OBJ_DIR)/serwer.o $(OBJ_DIR)/frames.o
	$(CC) $(CFLAGS) $^ -o $@

$(OBJ_DIR)/main.o: main.c user.h serwer.h frames.h | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/user.o: user.c user.h frames.h | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/serwer.o: serwer.c user.h serwer.h frames.h | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/frames.o: frames.c frames.h | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR):
	mkdir $(OBJ_DIR)

clean:
	rm -f $(OBJ_DIR)/*.o $(TARGET)
	rmdir $(OBJ_DIR)
