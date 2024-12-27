# 设置编译器和编译选项，-O0 表示不进行优化，-g 表示生成调试信息
CXX = g++
CXXFLAGS = -std=c++11 -O0 -g

# 设置源文件，Makefile 会根据这些文件生成相应的目标文件（.o 文件）
SOURCES = driver.cpp Node.cpp PProcedure.cpp Value.cpp
# 使用 Makefile 的模式替换功能，表示将每个 .cpp 文件替换为相应的 .o 文件，$(OBJECTS) 是 将要链接的目标文件。
OBJECTS = $(SOURCES:.cpp=.o)
# 定义了最终生成的可执行文件名
TARGET = driver

# 默认目标
all: $(TARGET)

# 将所有目标文件（$(OBJECTS)）链接成最终的可执行文件 driver
$(TARGET): clean $(OBJECTS)
	$(CXX) $(OBJECTS) -o $(TARGET)

# 编译源文件为目标文件
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# 清理生成的文件
clean:
	rm -f $(OBJECTS) $(TARGET)

# 安装目标（如果需要）
install: all
	@echo "安装程序"
