RESET=$(shell echo -e "\033[0m")
RED=$(shell echo -e "\033[1;31m")
GREEN=$(shell echo -e "\033[1;32m")
YELLOW=$(shell echo -e "\033[1;33m")
BLUE=$(shell echo -e "\033[1;34m")

DATA_DIR=/home/viljar/data
MARIADB_DIR=$(DATA_DIR)/mariadb
WORDPRESS_DIR=$(DATA_DIR)/wordpress

COMPOSE_FILE=srcs/docker-compose.yml

all: mariadb_data wordpress_data images up
	@echo "$(GREEN)ALL GOOD. DONE!$(RESET)"

mariadb_data:
	@echo "$(YELLOW)Creating MariaDB data directory...$(RESET)"
	@mkdir -p $(MARIADB_DIR)

wordpress_data:
	@echo "$(YELLOW)Creating WordPress data directory...$(RESET)"
	@mkdir -p $(WORDPRESS_DIR)

images:
	@echo "$(BLUE)Building docker images...$(RESET)"
	@docker-compose -f $(COMPOSE_FILE) build

up:
	@echo "$(BLUE)Starting containers...$(RESET)"
	@docker-compose -f $(COMPOSE_FILE) up -d

down:
	@echo "$(RED)Stopping containers...$(RESET)"
	@docker-compose -f $(COMPOSE_FILE) down

clean:
	@echo "$(RED)Removing containers, images and volumes...$(RESET)"
	@docker-compose -f $(COMPOSE_FILE) down --rmi all -v

fclean: clean
	@echo "$(RED)Removing data directories...$(RESET)"
	@doas rm -rf $(DATA_DIR)
	@docker system prune -f --volumes

re: fclean all

.PHONY: all clean fclean re up down images mariadb_data wordpress_data