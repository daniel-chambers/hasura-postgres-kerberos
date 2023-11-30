FROM debian:bullseye-slim
RUN apt update && apt install -y krb5-user gcc

# The Kerberos configuration specifying the realm
COPY krb5.conf /etc/krb5.conf

COPY canonical-name.c /tmp/canonical-name.c
RUN gcc /tmp/canonical-name.c -o /usr/bin/canonical-name
