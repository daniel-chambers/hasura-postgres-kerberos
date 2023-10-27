FROM debian:bullseye-slim
RUN apt update && apt install -y krb5-user

# The Kerberos configuration specifying the realm
COPY krb5.conf /etc/krb5.conf
