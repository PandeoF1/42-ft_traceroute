#include "ft_traceroute.h"

#define MAX_HOPS 30
#define PACKET_SIZE 68

int main(int argc, char **argv)
{
	if (argc != 2)
	{
		printf("Usage: %s <hostname>\n", argv[0]);
		return (1);
	}

	char *destination = argv[1];
	struct sockaddr_in dest_addr;
	ft_memset(&dest_addr, 0, sizeof(dest_addr));
	dest_addr.sin_family = AF_INET;
	dest_addr.sin_port = htons(33434);
	int udpfd, icmpfd = -1;
	int ttl = 0;

	// Hostname to IPv4
	struct hostent *host;
	if ((host = gethostbyname(destination)) == NULL)
	{
		perror("gethostbyname");
		exit(EXIT_FAILURE);
	}

	ft_memcpy(&(dest_addr.sin_addr), host->h_addr, host->h_length);

	inet_pton(PF_INET, destination, &(dest_addr.sin_addr));

	// Create a raw socket for UDP
	if ((udpfd = socket(PF_INET, SOCK_DGRAM, 0)) == -1)
	{
		perror("socket");
		exit(EXIT_FAILURE);
	}
	icmpfd = socket(PF_INET, SOCK_RAW, IPPROTO_ICMP);
	if (icmpfd < 0)
	{
		icmpfd = socket(PF_INET, SOCK_DGRAM, IPPROTO_ICMP);
		if (icmpfd < 0)
		{
			perror("socket");
			exit(EXIT_FAILURE);
		}
	}

	if (setsockopt(icmpfd, IPPROTO_IP, IP_HDRINCL, &(int){1}, sizeof(int)) < 0)
	{
		perror("setsockopt");
		exit(EXIT_FAILURE);
	}

	printf("ft_traceroute to %s (%s), %d hops max, %d byte packets\n", destination, inet_ntoa(dest_addr.sin_addr), MAX_HOPS, PACKET_SIZE - 8);

	while (ttl++ < MAX_HOPS)
	{

		// Set the TTL for the socket
		if (setsockopt(udpfd, IPPROTO_IP, IP_TTL, &ttl, sizeof(int)))
		{
			perror("setsockopt");
			exit(EXIT_FAILURE);
		}

		// Send the packet
		printf("%c%d ", (ttl < 10) ? ' ' : '\0', ttl);
		struct timeval start[3], end[3];
		struct sockaddr_in last_addr;
		for (int i = 0; i < 3; i++)
		{
			// Get the current time
			gettimeofday(&start[i], NULL);

			// Send the packet
			char data[PACKET_SIZE];
			ft_memset(data, 0, sizeof(data));
			dest_addr.sin_port = htons(33434 + i);
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

			// Receive the packet
			fd_set read_set;
			struct timeval timeout;
			timeout.tv_sec = 1; // 1 second timeout
			timeout.tv_usec = 0;

			FD_ZERO(&read_set);
			FD_SET(icmpfd, &read_set);

			int select_result = select(icmpfd + 1, &read_set, NULL, NULL, &timeout);

			if (select_result == -1)
			{
				perror("select");
				exit(EXIT_FAILURE);
			}
			else if (select_result > 0)
			{
				// Data is available to read
				if (FD_ISSET(icmpfd, &read_set))
				{
					// Receive the packet
					char recv_buffer[PACKET_SIZE];
					struct sockaddr_in recv_addr;
					socklen_t addr_len = sizeof(recv_addr);
					ssize_t recv_len = recvfrom(icmpfd, recv_buffer, sizeof(recv_buffer), 0, (struct sockaddr *)&recv_addr, &addr_len);

					if (recv_len == -1)
					{
						if (errno == EHOSTUNREACH)
						{
							printf(" * (no response)");
						}
						else
						{
							perror("recvfrom");
							exit(EXIT_FAILURE);
						}
					}

					gettimeofday(&end[i], NULL);
					char *host_name;
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
