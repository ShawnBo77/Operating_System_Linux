# indicating that target "all" and "clean" are not files
.PHONY: all clean

# set some variables
CC= g++
CFLAGS= -Wall -Wextra -O3 -Wpedantic
LDFLAGS = -lpthread
OUTPUT_OPTION= -MMD -MP -o $@

SOURCE= sched_demo_313551156.cpp
OBJS= $(SOURCE:.cpp=.o)
DEPS= $(SOURCE:.cpp=.d)
TARGET= sched_demo_313551156

# first command of make
all: $(TARGET)

-include $(DEPS)

# implicit targets
# %.o: %.c
#        $(CC) $^ -o $@ -c $(CFLAGS)

$(OBJS): $(SOURCE)
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET): $(OBJS)
	$(CC) $(OBJS) $(LDFLAGS) -o $@

clean:
	@rm -f $(TARGET) $(OBJS) $(DEPS)
