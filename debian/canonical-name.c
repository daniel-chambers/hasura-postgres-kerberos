/*
 * This this little C program can be used to try to determine what Kerberos is considering
 * the canonical name of a particular hostname. It performs what happens when
 * dns_canonicalize_hostname is set to true
 * See https://web.mit.edu/kerberos/krb5-latest/doc/admin/princ_dns.html#service-principal-canonicalization
 * for more information.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>

int main(int argc, char *argv[])
{
    if (argc != 2) {
        printf("Error: Pass the hostname to canonicalise as the first and only command line argument\n");
        return EXIT_FAILURE;
    }

    // resolve the domain name into a list of addresses
    struct addrinfo* result;
    int error;
    error = getaddrinfo(argv[1], NULL, NULL, &result);
    if (error != 0) {
        if (error == EAI_SYSTEM) {
            perror("getaddrinfo");
        } else {
            fprintf(stderr, "error in getaddrinfo: %s\n", gai_strerror(error));
        }
        exit(EXIT_FAILURE);
    }

    // loop over all returned results and do inverse lookup
    struct addrinfo* res;
    for (res = result; res != NULL; res = res->ai_next) {
        // Print out the resolved IP address (IPv4 or IPv6)
        char addrbuf[INET6_ADDRSTRLEN + 1];
        if (res->ai_family == AF_INET) {
            const char* retval;
            retval = inet_ntop(AF_INET, &(((struct sockaddr_in *)res->ai_addr)->sin_addr), addrbuf, sizeof addrbuf);
            if (retval != NULL) {
                printf("%s: IPv4 = %s\n", argv[1], addrbuf);
            } else {
                fprintf(stderr, "%s: %s.\n", argv[1], strerror(errno));
            }
        } else if (res->ai_family == AF_INET6) {
            const char* retval;
            retval = inet_ntop(AF_INET6, &(((struct sockaddr_in6 *)res->ai_addr)->sin6_addr), addrbuf, sizeof addrbuf);
            if (retval != NULL) {
                printf("%s: IPv6 = %s\n", argv[1], addrbuf);
            } else {
                fprintf(stderr, "%s: %s.\n", argv[1], strerror(errno));
            }
        }

        // Perform a reverse DNS lookup on the IP address
        char hostname[NI_MAXHOST];
        error = getnameinfo(res->ai_addr, res->ai_addrlen, hostname, NI_MAXHOST, NULL, 0, 0);
        if (error != 0) {
            fprintf(stderr, "error in getnameinfo: %s\n", gai_strerror(error));
            continue;
        }
        if (*hostname != '\0') {
            printf("%s: hostname: %s\n\n", addrbuf, hostname);
        }
    }

    freeaddrinfo(result);
    return 0;
}
