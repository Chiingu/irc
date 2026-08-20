NAME := ircserv
CXX := c++
CXXFLAGS := -Wall -Wextra -Werror -std=c++98
CPPFLAGS := -Iinclude
DEPFLAGS := -MMD -MP

SRCS := src/main.cpp src/Utils.cpp src/Message.cpp src/Client.cpp \
		src/Channel.cpp src/Server.cpp src/ServerIO.cpp \
		src/ServerRegistration.cpp src/ServerMessaging.cpp \
		src/ServerChannel.cpp src/ServerMode.cpp src/ServerInfo.cpp \
		src/Bot.cpp
OBJS := $(SRCS:.cpp=.o)
DEPS := $(OBJS:.o=.d)

all: $(NAME)

$(NAME): $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) -o $(NAME)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) $(DEPFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(DEPS)

fclean: clean
	rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re

-include $(DEPS)
