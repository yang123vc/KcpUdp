kcpserv:ikcp.o unp_simple.o time_simple.h
	g++ -o kcpserv Source.cpp ikcp.o unp_simple.o -g
ikcp.o:ikcp.h
	g++ -c -o ikcp.o ikcp.c -g
unp_simple.o:unp_simple.cpp
	g++ -c -o unp_simple.o unp_simple.cpp -g
.PHONY:clean
clean:
	rm -f *.o kcpserv
