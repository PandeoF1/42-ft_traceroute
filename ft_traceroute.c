#include "ft_traceroute.h"

#define MAX_HOPS 12 // 30
#define PACKET_SIZE 64

unsigned short checksum(void *b, int len)
{
	unsigned short *buf = b;
	unsigned int sum = 0;
	unsigned short result;

	for (sum = 0; len > 1; len -= 2)
		sum += *buf++;

	if (len == 1)
		sum += *(unsigned char *)buf;

	sum = (sum >> 16) + (sum & 0xFFFF);
	sum += (sum >> 16);
	result = ~sum;

	return result;
}

int main(int argc, char **argv)
{
	if (argc != 2)
	{
		printf("Usage: %s <hostname>\n", argv[0]);
		return (1);
	}

	char *destination = argv[1];
	struct sockaddr_in dest_addr;
	struct sockaddr_in recv_addr;
	struct iphdr *ip_header;
	struct icmphdr *icmp_header;
	char packet[PACKET_SIZE];
	char recv_packet[PACKET_SIZE];
	int sockfd;
	int ttl = 0;

	// Hostname to IPv4
	struct hostent *host;
	if ((host = gethostbyname(destination)) == NULL) {
		perror("gethostbyname");
		exit(EXIT_FAILURE);
	}

	ft_memset(&dest_addr, 0, sizeof(dest_addr));
	dest_addr.sin_family = AF_INET;
	ft_memcpy(&(dest_addr.sin_addr.s_addr), host->h_addr, host->h_length);

	inet_pton(AF_INET, destination, &(dest_addr.sin_addr));

	// Create a raw socket
	if ((sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP)) == -1) {
		perror("socket");
		exit(EXIT_FAILURE);
	}

	// Set the TTL for the socket
	setsockopt(sockfd, IPPROTO_IP, IP_TTL, &ttl, sizeof(int));

	printf("ft_traceroute to %s (%s), %d hops max, %d byte packets\n", destination, inet_ntoa(dest_addr.sin_addr), MAX_HOPS, PACKET_SIZE - 4);

	while (ttl++ < MAX_HOPS) {
		// Set up the ICMP packet
		ft_memset(packet, 0, sizeof(packet));
		icmp_header = (struct icmphdr *)packet;
		icmp_header->type = ICMP_ECHO;
		icmp_header->code = 0;
		icmp_header->checksum = 0;
		icmp_header->un.echo.id = getpid();
		icmp_header->un.echo.sequence = ttl;
		struct timeval tv;
		gettimeofday(&tv, NULL);
		icmp_header->checksum = checksum(packet, sizeof(struct icmphdr) + sizeof(struct timeval));

		// Send the ICMP packet
		struct timeval start[3];
		struct timeval end[3];
		for (int i = 0; i < 3; i++){
			gettimeofday(&start[i], NULL);
			if (sendto(sockfd, packet, sizeof(packet), 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr)) == -1) {
				perror("sendto");
				exit(EXIT_FAILURE);
			}

			// Receive the ICMP reply
			socklen_t addr_len = sizeof(recv_addr);
			if (recvfrom(sockfd, recv_packet, sizeof(recv_packet), 0, (struct sockaddr *)&recv_addr, &addr_len) == -1) {
				perror("recvfrom");
				exit(EXIT_FAILURE);
			}
			gettimeofday(&end[i], NULL);
		}


		// Extract the IP header from the received packet
		ip_header = (struct iphdr *)recv_packet;

		icmp_header = (struct icmphdr *)(recv_packet + (ip_header->ihl << 2));
		printf("icmp_header->type: %d\n", icmp_header->type);
		// Print the information about the hop
		char *host_name;
		if ((host = gethostbyaddr(&(recv_addr.sin_addr), sizeof(struct in_addr), AF_INET)))
			host_name = host->h_name;

		printf("%c%d %s (%s)  %.3f ms  %.3f ms  %.3f ms\n", (ttl < 10) ? ' ' : '\0', ttl, host_name, inet_ntoa(recv_addr.sin_addr), (end[0].tv_sec - start[0].tv_sec) * 1000.0 + (end[0].tv_usec - start[0].tv_usec) / 1000.0, (end[1].tv_sec - start[1].tv_sec) * 1000.0 + (end[1].tv_usec - start[1].tv_usec) / 1000.0, (end[2].tv_sec - start[2].tv_sec) * 1000.0 + (end[2].tv_usec - start[2].tv_usec) / 1000.0);

		// Check if the destination is reached
		if (ip_header->daddr == dest_addr.sin_addr.s_addr) {
			printf("Destination reached!\n");
			break;
		}

		// Set the TTL for the next hop
		setsockopt(sockfd, IPPROTO_IP, IP_TTL, &ttl, sizeof(int));
	}

	// Close the socket
	close(sockfd);
	return (0);
}

//  1  Pandeo.mshome.net (172.17.0.1)  0.180 ms  0.119 ms  0.086 ms
