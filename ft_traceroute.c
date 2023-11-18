#include "ft_traceroute.h"

#define MAX_HOPS 30
#define PACKET_SIZE 68

int help(void)
{
	printf("Usage:\n  ft_traceroute host\n");
	printf("Options:\n  --help\t\tRead this help and exit\n");
	printf("Arguments:\n+    host\t\tThe host to trace the route to\n");
	return (EXIT_SUCCESS);
}

int bad_option(char *option, int argc)
{
	printf("Bad option `%s' (argc %d)\n", option, argc);
	return (EXIT_FAILURE);
}

int main(int argc, char **argv)
{
	int udpfd, icmpfd = -1;
	int ttl = 0;
	char *destination = argv[1];
	struct sockaddr_in dest_addr;
	struct hostent *host;
    struct addrinfo hints, *result;

	for (int i = 1; i < argc; i++)
		if (ft_strncmp(argv[i], "--help", 6) == 0)
			return (help());
		else if (argv[i][0] == '-')
			return (bad_option(argv[i], i));
	if (argc != 2)
		return (help());
	if (getuid() != 0)
	{
		printf("ft_traceroute: Lacking privilege for icmp socket.\n");
		return (1);
	}

	ft_memset(&dest_addr, 0, sizeof(dest_addr));
	dest_addr.sin_family = AF_INET;
	dest_addr.sin_port = htons(33434);

	ft_memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET;		// IPv4
	hints.ai_socktype = SOCK_DGRAM; // Use SOCK_DGRAM for UDP

	if (getaddrinfo(destination, NULL, &hints, &result) != 0) // Get the IPv4 address of the destination
	{
		printf("ft_traceroute: %s: Name or service not known\n", destination);
		exit(EXIT_FAILURE);
	}

	ft_memcpy(&(dest_addr.sin_addr), &((struct sockaddr_in *)result->ai_addr)->sin_addr, sizeof(struct in_addr));
	freeaddrinfo(result);
	inet_pton(PF_INET, destination, &(dest_addr.sin_addr));

	if ((udpfd = socket(PF_INET, SOCK_DGRAM, 0)) == -1) // Create the UDP socket
	{
		perror("socket");
		exit(EXIT_FAILURE);
	}
	if ((icmpfd = socket(PF_INET, SOCK_RAW, IPPROTO_ICMP)) < 0) // Create the ICMP socket
{
		perror("socket");
		exit(EXIT_FAILURE);
	}
	

	printf("ft_traceroute to %s (%s), %d hops max, %d byte packets\n", destination, inet_ntoa(dest_addr.sin_addr), MAX_HOPS, PACKET_SIZE - 8);

	while (ttl++ < MAX_HOPS)
	{
		struct timeval start[3], end[3];
		struct sockaddr_in last_addr;

		ft_memset(&last_addr, 0, sizeof(last_addr));
		if (setsockopt(udpfd, IPPROTO_IP, IP_TTL, &ttl, sizeof(int))) // Set the TTL
		{
			perror("setsockopt");
			exit(EXIT_FAILURE);
		}

		printf("%c%d ", (ttl < 10) ? ' ' : '\0', ttl);
		for (int i = 0; i < 3; i++) // Send the packet 3 times
		{
			fd_set read_set;
			struct timeval timeout;
			char data[PACKET_SIZE];

			ft_memset(data, 0, sizeof(data));
			gettimeofday(&start[i], NULL);
			dest_addr.sin_port = htons(33434 + ttl + i);

			if (sendto(udpfd, data, sizeof(data), 0, (struct sockaddr *)&dest_addr, sizeof(struct sockaddr)) == -1)
			{
				if (errno != ECONNRESET)
				{
					perror("sendto");
					exit(EXIT_FAILURE);
				}
				else
					break;
			}

			timeout.tv_sec = 1;
			timeout.tv_usec = 0;
			FD_ZERO(&read_set);
			FD_SET(icmpfd, &read_set);

			int select_result = select(icmpfd + 1, &read_set, NULL, NULL, &timeout); // Wait for the response

			if (select_result == -1)
			{
				perror("select");
				exit(EXIT_FAILURE);
			}
			else if (select_result > 0)
			{
				if (FD_ISSET(icmpfd, &read_set)) // If available to read
				{
					char recv_buffer[PACKET_SIZE];
					struct sockaddr_in recv_addr;
					socklen_t addr_len = sizeof(recv_addr);
					ssize_t recv_len = recvfrom(icmpfd, recv_buffer, sizeof(recv_buffer), 0, (struct sockaddr *)&recv_addr, &addr_len);
					char *host_name;

					if (recv_len == -1)
					{
						if (errno == EHOSTUNREACH)
							printf(" * (no response)");
						else
						{
							perror("recvfrom");
							exit(EXIT_FAILURE);
						}
					}

					gettimeofday(&end[i], NULL);
					if ((host = gethostbyaddr(&(recv_addr.sin_addr), sizeof(struct in_addr), AF_INET)))
						host_name = host->h_name;
					else
						host_name = inet_ntoa(recv_addr.sin_addr);
					if (last_addr.sin_addr.s_addr == recv_addr.sin_addr.s_addr)
						printf(" %.3f ms ", (end[i].tv_sec - start[i].tv_sec) * 1000.0 + (end[i].tv_usec - start[i].tv_usec) / 1000.0);
					else
						printf(" %s (%s)  %.3f ms ", host_name, inet_ntoa(recv_addr.sin_addr), (end[i].tv_sec - start[i].tv_sec) * 1000.0 + (end[i].tv_usec - start[i].tv_usec) / 1000.0);
					last_addr = recv_addr;
				}
			}
			else
				printf(" * ");
		}
		printf("\n");
		if (last_addr.sin_addr.s_addr == dest_addr.sin_addr.s_addr)
		{
			close(udpfd);
			close(icmpfd);
			exit(EXIT_SUCCESS);
		}
	}

	close(udpfd);
	close(icmpfd);
	return (0);
}
