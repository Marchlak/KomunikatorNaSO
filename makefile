CC=gcc
CFLAGS=-Wall -g
TARGET=komunikator
OBJ_DIR=build

all: $(TARGET)

$(TARGET): $(OBJ_DIR)/main.o $(OBJ_DIR)/klient.o $(OBJ_DIR)/serwer.o $(OBJ_DIR)/wiadomosci.o
	$(CC) $(CFLAGS) $^ -o $@

$(OBJ_DIR)/main.o: main.c klient.h serwer.h wiadomosci.h | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/klient.o: klient.c klient.h wiadomosci.h | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/serwer.o: serwer.c klient.h serwer.h wiadomosci.h | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/wiadomosci.o: wiadomosci.c wiadomosci.h | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR):
	mkdir $(OBJ_DIR)

clean:
	rm -f $(OBJ_DIR)/*.o $(TARGET)
	rmdir $(OBJ_DIR)
