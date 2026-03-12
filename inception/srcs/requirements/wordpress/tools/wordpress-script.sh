#!/bin/sh
set -e

WORDPRESS_DATABASE_USER_PASSWORD=$(tr -d '\r\n' < /run/secrets/db_password)
WORDPRESS_ADMIN_PASSWORD=$(tr -d '\r\n' < /run/secrets/credentials)
WORDPRESS_USER_PASSWORD=$(tr -d '\r\n' < /run/secrets/credentials)

echo "Setting up WordPress..."
echo "memory_limit = 512M" >> /etc/php83/php.ini

cd /var/www/html

# Download WP-CLI if missing
if [ ! -f /usr/local/bin/wp ]; then
	echo "Downloading WP-CLI..."
	wget -q https://raw.githubusercontent.com/wp-cli/builds/gh-pages/phar/wp-cli.phar -O /usr/local/bin/wp
	chmod +x /usr/local/bin/wp
fi

# Wait for MariaDB
echo "Checking if MariaDB is running..."
until mysql -h"$WORDPRESS_DATABASE_HOST" -u"$WORDPRESS_DATABASE_USER" -p"$WORDPRESS_DATABASE_USER_PASSWORD" -e "SHOW DATABASES;" >/dev/null 2>&1
do
	echo "MariaDB not ready yet..."
	sleep 2
done
echo "MariaDB is ready!"

# Install WordPress if missing
if [ ! -f /var/www/html/wp-config.php ]; then
	echo "Downloading WordPress..."
	/usr/local/bin/wp core download --allow-root --path=/var/www/html

	echo "Creating wp-config.php..."
	/usr/local/bin/wp config create \
		--path=/var/www/html \
		--dbname="$WORDPRESS_DATABASE_NAME" \
		--dbuser="$WORDPRESS_DATABASE_USER" \
		--dbpass="$WORDPRESS_DATABASE_USER_PASSWORD" \
		--dbhost="$WORDPRESS_DATABASE_HOST" \
		--force \
		--allow-root

	echo "Installing WordPress..."
	/usr/local/bin/wp core install \
		--path=/var/www/html \
		--url="https://$DOMAIN_NAME" \
		--title="$WORDPRESS_TITLE" \
		--admin_user="$WORDPRESS_ADMIN" \
		--admin_password="$WORDPRESS_ADMIN_PASSWORD" \
		--admin_email="$WORDPRESS_ADMIN_EMAIL" \
		--skip-email \
		--allow-root

	echo "Creating an extra WordPress user..."
	/usr/local/bin/wp user create "$WORDPRESS_USER" \
		"$WORDPRESS_USER_EMAIL" \
		--path=/var/www/html \
		--user_pass="$WORDPRESS_USER_PASSWORD" \
		--role=author \
		--allow-root
else
	echo "WordPress is already installed and configured."
fi

# Permissions
chmod -R 755 /var/www/html
chmod -R 775 /var/www/html/wp-content
chown -R www-data:www-data /var/www/html

# Start PHP-FPM
echo "Running PHP-FPM in the foreground..."
exec php-fpm83 -F
