FROM debian:bullseye-slim
ARG DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y apt-utils
RUN apt-get install -y krb5-kdc krb5-admin-server

RUN mkdir /var/log/kerberos
COPY krb5.conf /etc/krb5.conf
COPY kdc.conf /etc/krb5kdc/kdc.conf
COPY kadm5.acl /etc/krb5kdc/kadm5.acl

# Create the initial database with our realm and with the specified password (bad practice to put that here!)
RUN kdb5_util create -r EXAMPLE-REALM.TEST -s -P Password123

EXPOSE 88

CMD /usr/sbin/krb5kdc -n
