#=====================
# Simple C++ MakeFile
#=====================
CC = g++
DB = gdb
TARGET = main.cpp
FILE = $(NAME).cpp
O_NAME = receiver.exe
CFLAGS = -std=c++17
#=====================

all:	# Compiles
	@$(CC) $(CFLAGS) -I . -o $(O_NAME) $(TARGET)

run:	# Compile and run
	@clear
	@$(CC) $(CFLAGS) -I . -o $(O_NAME) $(TARGET)
	@./$(O_NAME)

debug:	# Compile and Debug
	@clear
	@$(CC) $(CFLAGS) -g -I . -o $(O_NAME) $(TARGET)
	@$(DB) ./$(O_NAME)

#test:	# Compile and test with 'Grade file'
#	@rm -f $(O_NAME)
#	@$(CC) -std=c++11 -o $(O_NAME) $(TARGET)
	

clean:	# Remove compiled file
	@if [ -f  $(O_NAME) ]; then\
		rm -f $(O_NAME);\
		echo "Removed $(O_NAME)";\
		sleep 0.25;\
		clear;\
	else\
		echo "Already Clean";\
		sleep 0.25;\
		clear;\
	fi

# +++
# Set up commands
# +++

init:	# Creates a C++ File
	@if [ -f $(FILE) ]; then\
			echo " '$(FILE)' Already Exists ";\
			sleep 1;\
			clear;\
	else\
			echo " Made default ";\
			touch $(FILE);\
			echo "#include <iostream>\n" >> $(FILE);\
			echo "int main(){\n" >> $(FILE);\
			echo "	// Code Here\n" >> $(FILE);\
			echo "	return 0;" >> $(FILE);\
			echo "}" >> $(FILE);\
			sleep 1;\
			clear;\
	fi

# ===================================


