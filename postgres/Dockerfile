FROM postgres:16-bullseye
ARG DEBIAN_FRONTEND=noninteractive
RUN apt update && apt install -y krb5-user libpam-krb5 libpam-ccreds

# Make a volume for where the keytab file will live with the
# Postgres service principal's key
RUN mkdir /var/lib/postgresql/kerberos
VOLUME /var/lib/postgresql/kerberos

# The Kerberos configuration specifying the realm
COPY krb5.conf /etc/krb5.conf
# Modified config where we have enabled GSSAPI authentication 
# for all incoming connections that will use our realm
COPY pg_hba.conf.sample /usr/share/postgresql/16/pg_hba.conf.sample
# Modified config where we have specified our keytab file location
COPY postgresql.conf.sample /usr/share/postgresql/postgresql.conf.sample
