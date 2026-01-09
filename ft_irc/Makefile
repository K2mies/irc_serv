# **************************************************************************** #
#                                                                              #
#                                                         :::      ::::::::    #
#    Makefile                                           :+:      :+:    :+:    #
#                                                     +:+ +:+         +:+      #
#    By: rhvidste <rhvidste@student.hive.email.com  +#+  +:+       +#+         #
#                                                 +#+#+#+#+#+   +#+            #
#    Created: 2025/12/18 13:38:18 by rhvidste          #+#    #+#              #
#    Updated: 2025/12/18 13:41:36 by rhvidste         ###   ########.fr        #
#                                                                              #
# **************************************************************************** #

NAME		:=	ircserv
CC			:=	c++
FLAGS		:=	-Wall -Werror -Wextra -std=c++20
DEBUG_FLAGS	:=	-g
# ---------------------------------------------------------------------------- #
INC_DIR		:=	./inc
HEADERS		:=	$(INC_DIR)/Server.hpp				\
							$(INC_DIR)/Channel.hpp			\
							$(INC_DIR)/Client.hpp				\
							$(INC_DIR)/Command.hpp
# ---------------------------------------------------------------------------- #
SRC_DIR		:=	./src
OBJ_DIR		:=	./obj
# ---------------------------------------------------------------------------- #
SRC				:=	$(SRC_DIR)/main.cpp					\
							$(SRC_DIR)/Server.cpp				\
							$(SRC_DIR)/Channel.cpp			\
							$(SRC_DIR)/Clientl.cpp			\
							$(SRC_DIR)/Command.cpp

# ---------------------------------------------------------------------------- #
OBJ			:=	$(patsubst $(SRC_DIR)/%.cpp, $(OBJ_DIR)/%.o, $(SRC))

.SECONDARY: $(OBJ)
# ---------------------------------------------------------------------------- #
RED			:= \033[0;31m
GREEN		:= \033[0;32m
EC			:= \033[0m
# ---------------------------------------------------------------------------- #

all: $(NAME)

$(NAME): $(OBJ)
	@$(CC) $(FLAGS) -I $(INC_DIR) $(OBJ) -o $@
	@echo "$(GREEN)$(NAME) created$(EC)"

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp $(HEADERS)
	@mkdir -p $(OBJ_DIR)
	$(CXX) $(FLAGS) -I $(INC_DIR) -c $< -o $@

# ---------------------------------------------------------------------------- #
clean:
	@rm -rf $(OBJ_DIR)
	@echo "$(RED)obj files removed$(EC)"

fclean: clean
	@rm -rf $(NAME)
	@echo "$(RED)Full clean up completed$(EC)"

re: fclean all
# ---------------------------------------------------------------------------- #
.PHONY: all clean fclean re
# ---------------------------------------------------------------------------- #
