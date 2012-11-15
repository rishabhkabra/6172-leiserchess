OPTS=-encoding US-ASCII -O

lauto.jar : Game.class Leiserchess.class Harness.class Pattach.class PlayGame.class Book.class Counter.class Player.class main.txt
	jar cmf main.txt lauto.jar *.class
	cp -f lauto.jar ../tests/	 

Game.class : Game.java
	javac ${OPTS} Game.java

Leiserchess.class : Leiserchess.java
	javac ${OPTS} Leiserchess.java

Harness.class : Harness.java
	javac ${OPTS} Harness.java

Pattach.class : Pattach.java
	javac ${OPTS} Pattach.java

PlayGame.class : PlayGame.java
	javac ${OPTS} PlayGame.java

Book.class : Book.java
	javac ${OPTS} Book.java

Counter.class : Counter.java
	javac ${OPTS} Counter.java

Player.class : Player.java
	javac ${OPTS} Player.java

clean : 
	rm -rf *.class *~ *.o lauto.jar
