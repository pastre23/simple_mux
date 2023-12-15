# Makefile per cross-compilare un programma per aarch64 utilizzando le librerie FFmpeg

# Impostazioni del compilatore
CC=aarch64-linux-gnu-gcc
CFLAGS=-I./libs/ffmpeg/include
LDFLAGS=-L./libs/ffmpeg/lib -lavformat -lavcodec -lavutil -lswresample

# Il nome del file sorgente e dell'eseguibile
SRC=./src/main.c
TARGET=simple_mux

# Regola per compilare l'eseguibile
$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET) $(LDFLAGS)

# Pulizia
clean:
	rm -f $(TARGET)
