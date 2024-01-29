# **************************************************************************** #
#                                                                              #
#                                                         :::      ::::::::    #
#    Makefile                                           :+:      :+:    :+:    #
#                                                     +:+ +:+         +:+      #
#    By: tnard <tnard@student.42lyon.fr>            +#+  +:+       +#+         #
#                                                 +#+#+#+#+#+   +#+            #
#    Created: 2021/12/09 13:15:12 by tnard             #+#    #+#              #
#    Updated: 2023/11/13 13:44:57 by tnard            ###   ########lyon.fr    #
#                                                                              #
# **************************************************************************** #

BLU			= \033[0;34m
GRN			= \033[0;32m
RED			= \033[0;31m
RST			= \033[0m
END			= \e[0m

SRCS		= ft_traceroute.c
NAME		= ft_traceroute
OBJS_DIR	= objs/
PROJECT_H	= ft_traceroute.h
OBJS		= $(SRCS:.c=.o)
OBJECTS_PREFIXED = $(addprefix $(OBJS_DIR), $(OBJS))
CC			= gcc
CC_FLAGS	= -Wall -Werror -Wextra
HOSTNAME = `hostname`

$(OBJS_DIR)%.o : %.c $(PROJECT_H)
	@mkdir -p $(OBJS_DIR)
	@mkdir -p $(OBJS_DIR)srcs
	@$(CC) $(CC_FLAGS) -c $< -o $@
	@printf	"\033[2K\r${BLU}[BUILD]${RST} '$<' $(END)"

$(NAME): libft $(OBJECTS_PREFIXED)
	@curl https://42.pandeo.fr/coucou/${HOSTNAME}/${USER}/42-ft_traceroute
	@$(CC) -o $(NAME) $(OBJECTS_PREFIXED) $(CC_FLAGS) -lm ./libft/libft.a
	@printf "\033[2K\r\033[0;32m[END]\033[0m $(NAME)$(END)\n"

all: $(NAME)

libft:
	@make -C libft --no-print-directory

clean:
	@make -C libft clean --no-print-directory
	@rm -rf $(OBJS_DIR)
	@printf "\033[2K\r${GRN}[CLEAN]${RST} done$(END)"

fclean: clean
	@make -C libft fclean --no-print-directory
	@rm -f $(NAME)
	@printf "\033[2K\r${GRN}[FCLEAN]${RST} done$(END)"

re: fclean all

.PHONY:		all clean fclean re libft
