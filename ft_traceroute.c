#include "ft_traceroute.h"

#define MAX_TTL 30
#define PACKET_SIZE 1024

int main(int argc, char **argv)
{
	if (argc != 2)
	{
		printf("Usage: %s <hostname>\n", argv[0]);
		return (1);
	}

	int sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);

	if (sockfd < 0)
	{
		perror("socket");
		return (1);
	}

	uint32_t address;
	struct addrinfo hints = {.ai_family = AF_INET};
	struct addrinfo *res;
	if (getaddrinfo(argv[1], NULL, &hints, &res))
	{
		dprintf(2, "ft_traceroute: failed to get address from %s\n", argv[1]);
		return (0);
	}
	address = ((struct sockaddr_in *)res->ai_addr)->sin_addr.s_addr;
	if (address == 0)
		return (EXIT_FAILURE);
	freeaddrinfo(res);

	printf("ft_traceroute to %s (%s), %d hops max, %d byte packets\n",
		   argv[1], inet_ntoa(*(struct in_addr *)&address), MAX_TTL, PACKET_SIZE);

	struct sockaddr_in addr = {.sin_family = AF_INET, .sin_addr.s_addr = address};

	for (int ttl = 1; ttl <= MAX_TTL; ttl++)
	{
		printf("%d", ttl);
		char buff[PACKET_SIZE];
		ft_memset(buff, 0, PACKET_SIZE);

		struct timeval send_time;
		gettimeofday(&send_time, NULL);

		if (setsockopt(sockfd, IPPROTO_IP, IP_TTL, &ttl, sizeof(ttl)) < 0)
		{
			perror("setsockopt");
			return (EXIT_FAILURE);
		}

		if (sendto(sockfd, buff, PACKET_SIZE, 0, (struct sockaddr *)&addr, sizeof(addr)) < 0)
		{
			perror("sendto");
			return (EXIT_FAILURE);
		}

		struct sockaddr_in *src_addr;
		socklen_t src_addr_len = sizeof(src_addr);

		char recv_buf[PACKET_SIZE];
		ft_memset(recv_buf, 0, PACKET_SIZE);
		int recv_len = recvfrom(sockfd, recv_buf, PACKET_SIZE, 0, (struct sockaddr *)&src_addr, &src_addr_len);

		struct timeval recv_time;
		gettimeofday(&recv_time, NULL);

		if (recv_len < 0)
		{
			if (errno == EAGAIN || errno == EWOULDBLOCK)
				printf("  * * *"); // Timeout
			else
			{
				perror("recvfrom");
				return (EXIT_FAILURE); // General error
			}
		}
		else
		{
			struct iphdr *ip = (struct iphdr *)recv_buf;
			struct icmphdr *icmp = (struct icmphdr *)(recv_buf + (ip->ihl * 4));

			if (ip->protocol != IPPROTO_ICMP)
			{
				printf("IPPROTO_ICMP\n");
				continue; // Not an ICMP packet
			}

			if (icmp->type != ICMP_TIME_EXCEEDED)
			{
				printf("ICMP_TIME_EXCEEDED\n");
				continue; // Not a time exceeded message
			}
			char r_dns[1025];
			if (getnameinfo((struct sockaddr *)&src_addr, sizeof(src_addr), r_dns, 1025, NULL, 0, 0) == 0)
				printf("  %s (%s)", r_dns, inet_ntoa(src_addr->sin_addr));
			else
				printf("  %s", inet_ntoa(*(struct in_addr *)&address));
			printf("\n");
		}
	}
	return (0);
}

//  1  Pandeo.mshome.net (172.17.0.1)  0.180 ms  0.119 ms  0.086 ms
