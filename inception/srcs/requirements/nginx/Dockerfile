FROM alpine:3.20.6

RUN apk update && apk add --no-cache nginx openssl && rm -rf /var/cache/apk/*

EXPOSE 443

RUN mkdir -p /etc/nginx/ssl

RUN openssl req -x509 -nodes \
	-out /etc/nginx/ssl/public_certificate.crt \
	-keyout /etc/nginx/ssl/private.key \
	-subj "/C=FI/ST=Uusimaa/L=Helsinki/O=42/CN=vuljas.42.fr"

RUN adduser -D -H -s /sbin/nologin -g www-data -G www-data www-data

COPY conf/nginx.cnf /etc/nginx/nginx.cnf
RUN chmod 644 /etc/nginx/nginx.cnf

CMD ["nginx", "-c", "/etc/nginx/nginx.cnf", "-g", "daemon off;"]
