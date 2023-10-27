# Hasura using Kerberos Authentication with Postgres Demo
This repository demonstrates Kerberos being used to authenticate against a Postgres server in Hasura.
This is done by manually providing the Kerberos ticket cache file to the Hasura container that contains
the user principal's Ticket-Granting Ticket (TGT).

If you don't understand how Kerberos works, this explainer is highly recommended: https://kerberos.org/software/tutorial.html

## Setup
### Configure Kerberos
First start the Kerberos Key Distribution Center (KDC):

```
docker compose up --build -d kdc
```

This has been pre-configured with a Kerberos realm called `EXAMPLE-REALM.TEST` in `/etc/krb5kdc/kdc.conf`.
Kerberos tooling has also been configured to know about this realm and default to it in `/etc/krb5.conf`.
The DNS address of this container within the compose project's network will be `kdc.example-realm.test`.

Once the container is running, we need to shell into it and create two principals.

```
> docker compose exec -it kdc /bin/bash
```

#### The user principal
This principal represents the user that will be logging into the Postgres database. We will use
[`kadmin.local`](https://web.mit.edu/kerberos/krb5-1.12/doc/admin/admin_commands/kadmin_local.html) to create the user
principal in the Kerberos database, with a known password which we will need later to log in as that user.

```
> kadmin.local -q "add_principal -pw Password123 testuser"
```

### The service principal
This principal represents the Postgres database itself. We will use `kadmin.local` again to create the service principal,
with a random key. We don't need to know the key ourselves, as we will export the key to a keytab file which Postgres will
read. The directory to which we export the file is a mounted volume that is shared with the Postgres container, for ease of
this example.

```
> kadmin.local -q "add_principal -randkey postgres/postgres.example-realm.test"
> kadmin.local -q "ktadd -k /mnt/postgres-kerberos/postgres.keytab postgres/postgres.example-realm.test"
```

### Configure Postgres
We have already configured Postgres to accept [GSSAPI](https://www.postgresql.org/docs/16/gssapi-auth.html) Kerberos
authenticated requests from our `EXAMPLE-REALM.TEST` by modifying `/var/lib/postgresql/data/pg_hba.conf`.

Postgres will accept requests directed towards any service principal that it finds in the keytab file it has been given.
In the previous step we exported the service principal (`postgres/postgres.example-realm.test`) and the key for it
into the `postgres.keytab` file in a volume which is mounted at `/var/lib/postgresql/kerberos`. The `krb_server_keyfile`
setting in `/var/lib/postgresql/data/postgresql.conf` has been set to point at this file.

However, the Postgres DB process runs as the `postgres` user, so we need to change the ownership of the keytab file so
that it can access it. Let's start the Postgres container, then shell into it:

```
> docker compose up --build -d postgres
> docker compose exec -it postgres /bin/bash
```

Now let's change the ownership of the keytab file:

```
> chown postgres:postgres /var/lib/postgresql/kerberos/postgres.keytab
```

Postgres needs a local Postgres login to be created to match user principals that are trying to log in via
Kerberos. If the name of a Postgres user matches the Kerberos user principal name, it is accepted. Explicit mappings
between Kerberos user principals and Postgres users can also be made in the `/var/lib/postgresql/data/pg_ident.conf`
file. For simplicity here, though, we'll create a login in Postgres that matches our user principal and a database
for them to use.

We'll log in with the admin user and create our Postgres user and database:

```
> psql postgres://postgres:hasura@postgres.example-realm.test:5432/postgres -c 'CREATE ROLE "testuser@EXAMPLE-REALM.TEST" WITH LOGIN NOSUPERUSER INHERIT NOCREATEDB NOCREATEROLE NOREPLICATION;'
> psql postgres://postgres:hasura@postgres.example-realm.test:5432/postgres -c 'CREATE DATABASE testdb WITH OWNER = "testuser@EXAMPLE-REALM.TEST";'
```

### Create the ticket cache file containing the ticket-granting ticket (TGT) for our user principal
The (admittedly weird) scenario we're proving here is that if we provide a ticket cache containing our user principal's TGT to
Hasura we will be able to authenticate using that user principal to Postgres.

To create the ticket cache file, we'll use the debian container in the Docker Compose project:

```
> docker compose run --build --rm -it debian
```

We'll use [`kinit`](https://web.mit.edu/kerberos/krb5-1.12/doc/user/user_commands/kinit.html) to log in as our user principal
(enter the password we configured for this user principal earlier: `Password123`):

```
kinit testuser
```

This will have created our ticket cache in `/tmp/krb5cc_0`. We can view it by running
[`klist`](https://web.mit.edu/kerberos/krb5-1.12/doc/user/user_commands/klist.html).

```
> klist
Ticket cache: FILE:/tmp/krb5cc_0
Default principal: testuser@EXAMPLE-REALM.TEST

Valid starting     Expires            Service principal
10/27/23 06:22:56  10/27/23 16:22:56  krbtgt/EXAMPLE-REALM.TEST@EXAMPLE-REALM.TEST
        renew until 10/28/23 06:22:52
```

Now we need to copy this ticket cache file to our volume that is shared with our Hasura container:

```
cp /tmp/krb5cc_0 /mnt/ticket-cache/krb5cc_0
```

### Start Hasura and connect to our database
Now we can start Hasura and configure a data source using our user principal. We've configured the Kerberos plumbing
in the container to expect to find the ticket cache in `/mnt/ticket-cache/krb5cc_0` by setting the environment variable
`KRB5CCNAME`. We've also mounted in our Kerberos configuration file into `/etc/krb5.conf`.

```
> docker compose up -d hasura
```

Open http://localhost:8080 in your browser, and create a Postgres source with the following connection string:

```
postgres://testuser%40EXAMPLE-REALM.TEST@postgres.example-realm.test:5432/testdb
```

If it connects and creates successfully, we have been successful in authenticating using our Kerberos user principal!

If you want to see that a ticket for our Postgres DB's service principal has been acquired, you can view the (now changed)
ticket cache file by using the debian container to run `klist` against the file in the shared volume:

```
> docker compose run --build --rm -it debian
> KRB5CCNAME=/mnt/ticket-cache/krb5cc_0 klist
Default principal: testuser@EXAMPLE-REALM.TEST

Valid starting     Expires            Service principal
10/27/23 06:42:50  10/27/23 16:42:50  krbtgt/EXAMPLE-REALM.TEST@EXAMPLE-REALM.TEST
        renew until 10/28/23 06:42:48
10/27/23 06:43:07  10/27/23 16:42:50  postgres/postgres.example-realm.test@EXAMPLE-REALM.TEST
        renew until 10/28/23 06:42:48
```

You can see the new ticket for `postgres/postgres.example-realm.test@EXAMPLE-REALM.TEST`.
