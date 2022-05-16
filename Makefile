.PHONY:all
all: srv test_cgi
srv: unitTest.cpp
	g++ -o $@ $^ -std=c++11 -lpthread
test_cgi: test_cgi.cpp
	g++ -o $@ $^ -std=c++11
.PHONY:clean
clean:
	rm -f srv test_cgi wwwroot/test_cgi
