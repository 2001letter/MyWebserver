# 编译器和标志
CXX = g++
CXXFLAGS = -fsanitize=address -g -Wall -pthread -std=c++20
LDFLAGS = -lmysqlcppconn -fsanitize=address

# 目标可执行文件（放在 out 目录）
TARGET = bin/main

# 源文件（使用 wildcard 获取所有 .cpp 文件）
SRCS = code/main.cpp $(wildcard code/log/*.cpp) $(wildcard code/server/*.cpp) $(wildcard code/pool/*.cpp) $(wildcard code/buffer/*.cpp) $(wildcard code/http/*.cpp) $(wildcard code/timer/*.cpp) 

# 对象文件（放在 temp 目录，保持子目录结构）
OBJS = $(addprefix temp/, $(SRCS:.cpp=.o))

# 默认目标
all: $(TARGET)

# 链接生成可执行文件
$(TARGET): $(OBJS)
	mkdir -p $(dir $@)
	$(CXX) -o $@ $^ $(LDFLAGS)

# 编译 .cpp 到 .o，并放在 temp 目录下
temp/%.o: %.cpp
	mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# 清理中间文件和可执行文件
clean:
	rm -rf temp bin

# 防止 make 把 clean 当成文件
.PHONY: all clean