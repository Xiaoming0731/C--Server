CXX = g++
CXXFLAGS = -std=c++17 -Wall -g
INCLUDE = -I. -Itcp

TARGET = echo_server

SRCS = echo_server.cpp \
tcp/util.cpp \
tcp/CurrentThread.cpp \
tcp/EventLoop.cpp \
tcp/Epoller.cpp \
tcp/Channel.cpp \
tcp/Acceptor.cpp \
tcp/TcpConnection.cpp \
tcp/TcpServer.cpp \
tcp/Buffer.cpp \
tcp/ThreadPool.cpp

OBJS = $(SRCS:.cpp=.o)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) $^ -o $@

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDE) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: clean
