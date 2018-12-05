# **************************************************************************** #
#                                                                              #
#                                                         :::      ::::::::    #
#    Makefile                                           :+:      :+:    :+:    #
#                                                     +:+ +:+         +:+      #
#    By: xxxxxxxx <marvin@42.fr>                    +#+  +:+       +#+         #
#                                                 +#+#+#+#+#+   +#+            #
#    Created: 2016/11/30 12:38:27 by xxxxxxxx          #+#    #+#              #
#    Updated: 2016/12/09 23:34:44 by xxxxxxxx         ###   ########.fr        #
#                                                                              #
# **************************************************************************** #

NAME = test_leaker

CC	= gcc

CFLAGSX	= -Wall -Wextra -Werror

# usage ex : make re "LEAKER=1"  for using leaker 1 or not null to validate
LEAKER =

SRC_DIR = ./

INCLUDES	= ./

OBJ_DIR = ./obj_d


# put here all the sources to compile (not leaker.c)
SOURCES = test_leaker.c 

# LEAKER dependent section
ifeq ($(LEAKER),)
	SRCS = $(SOURCES)
	CFLAGS = $(CFLAGSX)
else
	SRCS = $(SOURCES) leaker.c			# leaker.c is added here
	CFLAGS = $(CFLAGSX) -DUSE_LEAKER	# enable define USE_LEAKER in code
endif

OBJ	=	$(SRCS:.c=.o)

OBJ_F	= $(addprefix $(OBJ_DIR)/,$(OBJ))

DEP_F	= $(OBJ_F:.o=.d)

#to detect if it is windows OS or not(multi_platform)
ifeq ($(shell echo "_"),"_")
	EXE_F	= $(NAME).exe
else
	EXE_F	= ./$(NAME)
endif

#variables for colors to highlight the make results
yellow	= \033[33;1m
cyan	= \033[36;1m
vert	= \033[32;1m
normal	= \033[0m


all: $(OBJ_DIR) $(EXE_F) finish

$(OBJ_DIR):
	@test -d $@ || mkdir $@

$(EXE_F): $(OBJ_F)
	@printf "\n$(yellow)$ Linking ...  $(normal)\n"
	$(CC) -o $@ $^

$(OBJ_F): $(OBJ_DIR)/%.o: $(SRC_DIR)/%.c  Makefile
	@printf "$(cyan)$ Compiling ...  $(normal)\n"
	$(CC) -c -MMD -MP $(CFLAGS) -o $@ $< -I$(INCLUDES)

-include $(DEP_F)
		
finish:
	@printf "\n\n$(vert)$(EXE_F) ... ready to execute! $(normal)\n\n"

clean:
	@rm -rf $(OBJ_DIR)

fclean: clean
	@rm -f $(EXE_F)

re: fclean all

.PHONY: all clean fclean re 

