kcpserv:ikcp.o unp_simple.o time_simple.h
	g++ Source.cpp ikcp.o unp_simple.o -o kcpserv -g
ikcp.o:ikcp.h
	g++ -c ikcp.c -o ikcp.o -g
unp_simple.o:unp_simple.cpp
	g++ -c unp_simple.cpp -o unp_simple.o -g
.PHONY:clean
clean:
	rm -f *.o kcpserv
