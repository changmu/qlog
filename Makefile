BIN=qlogtest
OBJS=qlog.o main.o
FLAGS=-g -std=c++11 -pthread

all:${BIN}

qlogtest: ${OBJS}
	g++ ${FLAGS} $^ -o $@

.cc.o:
	g++ ${FLAGS} -c $< -o $@

.PHONY: clean all
clean:
	-rm -f ${BIN} ${OBJS}
