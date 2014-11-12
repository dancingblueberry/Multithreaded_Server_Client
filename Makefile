# /*
# * Course: CS 100 Fall 2013 *
# * First Name: Amanda
# * Last Name: Berryhill
# * Username: aberr005
# * email address: aberr005@ucr.edu
# *
# * Assignment: 9
# * I hereby certify that the contents of this file represent
# * my own original individual work. Nowhere herein is there
# * code from any outside resources such as another individual,
# * a website, or publishings unless specifically designated as
# * permissible by the instructor or TA. 
# */

COMPILE=g++
FLAGS=
# -g
ALLEXECUTEABLES=mtServer mtClient
ALLSOURCECODE=mtServer.cpp mtClient.cpp
ALLTYPESCRIPTS=mtServer.log mtClient.log

all: mtServer mtClient

mtServer: mtServer.cpp
	$(COMPILE) $(FLAGS) -o mtServer mtServer.cpp

mtClient: mtClient.cpp
	$(COMPILE) $(FLAGS) -pthread -o mtClient mtClient.cpp

tar: 
	tar -czvf hw9.tgz Makefile $(ALLSOURCECODE) $(ALLTYPESCRIPTS)

scp: 
	scp hw9.tgz aberr005@well.cs.ucr.edu:~/cs100/hw/hw9

clean: 
	rm -rf $(ALLEXECUTEABLES)
