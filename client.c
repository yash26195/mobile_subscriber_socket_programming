// Programming Assignment 2
// Name: Yash Parekh
// SCU ID: W1607346

#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <string.h>

#define TIMER 3 // server timeout for resending the packet again
#define PORT 48000

// PROTOCOL PRIMITIVES
enum _PROTOCOL_PRIMITIVES
{
	START_OF_PACKET_ID = 0XFFFF,
    END_OF_PACKET_ID = 0XFFFF,
    CLIENT_ID = 0XFF,
    ACCESS_PER = 0xFFF8,
    LENGTH = 0XFF,
    SOURCE_SUBSCRIBER_NUMBER = 0xFFFFFFFF
};

// TECHNOLOGIES
enum _TECHNOLOGY
{
	_2G_ = 02,
	_3G_ = 03,
	_4G_ = 04,
	_5G_ = 05
};

// STATUS CODES
enum _SUBSCRIBER_STATUS_CODE
{
	NOT_PAID = 0xFFF9,
	NOT_EXIST = 0xFFFA,
	ACCESS_OK = 0xFFFB
};

struct  _REQUEST_PACKET
{
	uint16_t start_of_packet_id;
	uint8_t client_id;
	uint16_t access_permission;
	uint8_t seqno;
	uint8_t length;
	uint8_t technology;
	unsigned int source_subscriber_number;
	uint16_t end_of_packet_id;
};

struct  _RESPONSE_PACKET
{
	uint16_t start_of_packet_id;
	uint8_t client_id;
	uint16_t status_code;
	uint8_t seqno;
	uint8_t length;
	uint8_t technology;
	unsigned int source_subscriber_number;
	uint16_t end_of_packet_id;
};

// to print packet for verification at client's end
void print_packet(struct _REQUEST_PACKET request_packet)
{
	printf("\n");
	printf("|----------------------------------------------------------\n");
	printf("| Start of Packet ID: %x\n",request_packet.start_of_packet_id);
	printf("| Client ID: %hhx\n",request_packet.client_id);
	printf("| Access Permission: %x\n",request_packet.access_permission);
	printf("| Segment Number: %d\n",request_packet.seqno);
	printf("| Length: %d\n",request_packet.length);
	printf("| Technology: %u\n",request_packet.technology);
	printf("| Source Subscriber Number: %u\n",request_packet.source_subscriber_number);
	printf("| End of Packet ID: %x\n",request_packet.end_of_packet_id);
	printf("|----------------------------------------------------------\n\n");
}

// sets request packet before sending to the server
struct _REQUEST_PACKET generate_request_packet()
{
	struct _REQUEST_PACKET request_packet;
	request_packet.start_of_packet_id = START_OF_PACKET_ID;
	request_packet.client_id = CLIENT_ID;
	request_packet.access_permission = ACCESS_PER;
	request_packet.end_of_packet_id = END_OF_PACKET_ID;

	return request_packet;
}

int main(int argc, char const *argv[])
{
	FILE *fp;
	struct _REQUEST_PACKET request_packet;
	struct _RESPONSE_PACKET response_packet;

	// socket primitives to define socket
	struct sockaddr_in socket_address;
	socklen_t address_size;
	
	// record in request file
	char record[255];
	
	int socket_descriptor;
	int n = 0;
	int counter = 0;
	int seqno = 1;


	// set socket descriptor
	socket_descriptor = socket(AF_INET,SOCK_DGRAM,0);
	
	if(socket_descriptor < 0) 
	{
		perror("[ERROR] Failed To Create Socket!\n");
	}

	// sets zero-valued bytes to socket address
	bzero(&socket_address,sizeof(socket_address));
	socket_address.sin_family = AF_INET;
	socket_address.sin_addr.s_addr = htonl(INADDR_ANY);
	socket_address.sin_port=htons(PORT);
	address_size = sizeof(socket_address);

	// configuring the socket to timeout in 3 seconds
	struct timeval time_value;
	time_value.tv_sec = TIMER; 
	time_value.tv_usec = 0;

	// sets optional values to socket. here it's timeout
	setsockopt(socket_descriptor, SOL_SOCKET, SO_RCVTIMEO, (const char*)&time_value,sizeof(struct timeval));

	request_packet = generate_request_packet();

	// read requests from the request file
	fp = fopen("request.txt", "rt");
	if(fp == NULL)
	{
		perror("[ERROR] Cannot Open File!\n");
		exit(0);
	}

	while(fgets(record, sizeof(record), fp) != NULL) 
	{
		counter = 0;
		n = 0;
		char * token = NULL;
		// splitting the record in to token
		token = strtok(record," ");
		request_packet.length = strlen(token);
		request_packet.source_subscriber_number = (unsigned) atoi(token);

		token = strtok(NULL," ");
		request_packet.length += strlen(token);
		request_packet.technology = atoi(token);
		
		token = strtok(NULL," ");
		
		request_packet.seqno = seqno;
		
		// printing the contents of the packet
		print_packet(request_packet);
		
		while(n <= 0 && counter < 3) 
		{ 
			//  sending request packet to the server 
			sendto(socket_descriptor,&request_packet,sizeof(struct _REQUEST_PACKET),0,(struct sockaddr *)&socket_address,address_size);
			
			// receiving the response from the server
			n = recvfrom(socket_descriptor,&response_packet,sizeof(struct _RESPONSE_PACKET),0,NULL,NULL);
			
			if(n <= 0 ) 
			{
				// if no response recieved from server in three seconds send packet again
				printf("No response from server for three seconds. Sending the packet again!\n");
				counter++;
			}
			
			else 
			{
				// if subscriber has not paid
				if(response_packet.status_code == NOT_PAID) 
				{
					printf("[STATUS] Subscriber has Not Paid!\n");
				}

				// either the subscriber doesn't eixst or technology doesn't match
				else if(response_packet.status_code == NOT_EXIST ) 
				{
					printf("[STATUS] Subscriber Does Not Exist or technology Does Not Match!\n");
				}

				// susscess. access granted.
				else if(response_packet.status_code == ACCESS_OK) 
				{
					printf("[STATUS] Subscriber has been granted Access to the Network!\n");

				}
			}
		}
		// server doesn't respond in three tries
		if(counter >= 3 ) 
		{
			printf("\n");
			perror("[ERROR] Server does not respond!");
			printf("\n");
			exit(0);
		}
		
		seqno++;
		printf("\n###########################################################\n");
	}
	fclose(fp);

	return 0;
}
