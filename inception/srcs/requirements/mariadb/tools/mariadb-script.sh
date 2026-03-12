#!/bin/sh
set -e

MYSQL_DATADIR="/var/lib/mysql"
MYSQL_SOCKET="/run/mysqld/mysqld.sock"
WORDPRESS_DATABASE_USER_PASSWORD=$(tr -d '\r\n' < /run/secrets/db_password)


# Initialize system tables if they don't exist
if [ ! -d "$MYSQL_DATADIR/mysql" ]; then
    echo "[MariaDB] Initializing datadir..."

    mariadb-install-db \
        --user=mysql \
        --basedir=/usr \
        --datadir="$MYSQL_DATADIR"

    echo "[MariaDB] Running bootstrap SQL..."
    mariadbd \
        --user=mysql \
        --bootstrap \
        --datadir="$MYSQL_DATADIR" <<EOF

FLUSH PRIVILEGES;
ALTER USER 'root'@'localhost' IDENTIFIED BY '${MYSQL_ROOT_PASSWORD}';

CREATE DATABASE IF NOT EXISTS \`${WORDPRESS_DATABASE_NAME}\`
  CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;

CREATE USER IF NOT EXISTS '${WORDPRESS_DATABASE_USER}'@'%' IDENTIFIED BY '${WORDPRESS_DATABASE_USER_PASSWORD}';
GRANT ALL PRIVILEGES ON \`${WORDPRESS_DATABASE_NAME}\`.* TO '${WORDPRESS_DATABASE_USER}'@'%';
DELETE FROM mysql.user WHERE user='';
DROP DATABASE IF EXISTS test;
FLUSH PRIVILEGES;
EOF

    echo "[MariaDB] Initialization complete."
else
    echo "[MariaDB] Database already initialized."
fi

sed -i '/^[[:space:]]*skip-networking[[:space:]]*$/d' /etc/my.cnf.d/mariadb-server.cnf
sed -i 's/^[[:space:]]*bind-address[[:space:]]*=.*/bind-address = 0.0.0.0/' /etc/my.cnf.d/mariadb-server.cnf

echo "[MariaDB] Starting main server..."
exec mariadbd-safe