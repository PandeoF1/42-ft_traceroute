#include "ft_traceroute.h"

#define MAX_HOPS 12 // 30
#define PACKET_SIZE 68

unsigned short checksum(void* b, int len)
{
	unsigned short* buf = b;
	unsigned int sum = 0;
	unsigned short result;

	for (sum = 0; len > 1; len -= 2)
		sum += *buf++;

	if (len == 1)
		sum += *(unsigned char*)buf;

	sum = (sum >> 16) + (sum & 0xFFFF);
	sum += (sum >> 16);
	result = ~sum;

	return result;
}

int main(int argc, char** argv)
{
	if (argc != 2)
	{
		printf("Usage: %s <hostname>\n", argv[0]);
		return (1);
	}

	char* destination = argv[1];
	struct sockaddr_in dest_addr;
	int sockfd;
	int ttl = 0;

	// Hostname to IPv4
	struct hostent* host;
	if ((host = gethostbyname(destination)) == NULL) {
		perror("gethostbyname");
		exit(EXIT_FAILURE);
	}

	ft_memset(&dest_addr, 0, sizeof(dest_addr));
	dest_addr.sin_family = AF_INET;
	ft_memcpy(&(dest_addr.sin_addr.s_addr), host->h_addr, host->h_length);

	inet_pton(AF_INET, destination, &(dest_addr.sin_addr));

	// Create a raw socket for UDP
	if ((sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_UDP)) == -1) {
		perror("socket");
		exit(EXIT_FAILURE);
	}

	printf("ft_traceroute to %s (%s), %d hops max, %d byte packets\n", destination, inet_ntoa(dest_addr.sin_addr), MAX_HOPS, PACKET_SIZE - 8);

	while (ttl++ < MAX_HOPS) {

		// Set the TTL for the socket
		setsockopt(sockfd, IPPROTO_IP, IP_TTL, &ttl, sizeof(int));

		// Create the UDP header
		struct udphdr udp_header;
		udp_header.source = htons(33434);
		udp_header.dest = htons(33434);
		udp_header.len = htons(PACKET_SIZE);
		udp_header.check = 0;


		// Create the IP header
		struct iphdr ip_header;


		// Send the packet
		struct timeval start[3], end[3];
		for (int i = 0; i < 3; i++) {
			// Get the current time
			gettimeofday(&start[i], NULL);

			// Send the packet
			if (sendto(sockfd, &udp_header, PACKET_SIZE, 0, (struct sockaddr*)&dest_addr, sizeof(struct sockaddr)) == -1) {
				perror("sendto");
				exit(EXIT_FAILURE);
			}

			// Receive the packet
			struct sockaddr_in recv_addr;
			socklen_t recv_addr_len = sizeof(struct sockaddr_in);
			char recv_buf[PACKET_SIZE];
			if (recvfrom(sockfd, recv_buf, PACKET_SIZE, 0, (struct sockaddr*)&recv_addr, &recv_addr_len) == -1) {
				perror("recvfrom");
				exit(EXIT_FAILURE);
			}
			// Check if timeout
			

			// Get the current time
			gettimeofday(&end[i], NULL);

		}

		// Print the information about the hop
		/*char *host_name;
		if ((host = gethostbyaddr(&(recv_addr.sin_addr), sizeof(struct in_addr), AF_INET)))
			host_name = host->h_name;

		printf("%c%d %s (%s)  %.3f ms  %.3f ms  %.3f ms\n", (ttl < 10) ? ' ' : '\0', ttl, host_name, inet_ntoa(recv_addr.sin_addr), (end[0].tv_sec - start[0].tv_sec) * 1000.0 + (end[0].tv_usec - start[0].tv_usec) / 1000.0, (end[1].tv_sec - start[1].tv_sec) * 1000.0 + (end[1].tv_usec - start[1].tv_usec) / 1000.0, (end[2].tv_sec - start[2].tv_sec) * 1000.0 + (end[2].tv_usec - start[2].tv_usec) / 1000.0);
		*/
		// Check if the destination is reached
		/*char dest_ip[INET_ADDRSTRLEN];
		inet_ntop(AF_INET, &(ip_header->daddr), dest_ip, INET_ADDRSTRLEN);
		printf("dest_addr: %s <-> ip_header: %s | %08x <-> %08x\n", inet_ntoa(dest_addr.sin_addr), dest_ip, dest_addr.sin_addr.s_addr, ip_header->daddr);
		if (ip_header->daddr == dest_addr.sin_addr.s_addr) {
			printf("Destination reached!\n");
			break;
		}*/

		// Set the TTL for the next hop
		setsockopt(sockfd, IPPROTO_IP, IP_TTL, &ttl, sizeof(int));
	}

	// Close the socket
	close(sockfd);
	return (0);
}

//  1  Pandeo.mshome.net (172.17.0.1)  0.180 ms  0.119 ms  0.086 ms
