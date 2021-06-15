
// COEN 233 Computer Networks
// Programming Assignment 2
// Name: Yash Parekh
// SCU ID: W1607346

#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <strings.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define PORT 48000

// protocol primitives
enum _PROTOCOL_PRIMITIVES
{
	START_OF_PACKET_ID = 0XFFFF,
    END_OF_PACKET_ID = 0XFFFF,
    CLIENT_ID = 0XFF,
    ACCESS_PER = 0xFFF8,
    LENGTH = 0XFF,
    SOURCE_SUBSCRIBER_NUMBER = 0xFFFFFFFF
};

// technology
enum _TECHNOLOGY
{
	_2G_ = 02,
	_3G_ = 03,
	_4G_ = 04,
	_5G_ = 05
};

// subscriber status code
enum _SUBSCRIBER_STATUS_CODE
{
	NOT_PAID = 0xFFF9,
	NOT_EXIST = 0xFFFA,
	ACCESS_OK = 0xFFFB,
	TECHNOLOGY_DOES_NOT_MATCH = 2,
	SUBSCRIBER_DOES_NOT_EXIST = -1
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

struct _SUBSCRIBER_DATA 
{
	unsigned int subscriber_number;
	uint8_t technology;
	uint16_t status;
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

// sets response packet before sending to the server
struct _RESPONSE_PACKET generate_response_packet(struct _REQUEST_PACKET request_packet)
{
	struct _RESPONSE_PACKET response_packet;
	
	response_packet.start_of_packet_id = request_packet.start_of_packet_id;
	response_packet.client_id = request_packet.client_id;
	response_packet.seqno = request_packet.seqno;
	response_packet.length = request_packet.length;
	response_packet.technology = request_packet.technology;
	response_packet.source_subscriber_number = request_packet.source_subscriber_number;
	response_packet.end_of_packet_id = request_packet.end_of_packet_id;

	return response_packet;
}

// search the database for requested user information
int search_database(struct _SUBSCRIBER_DATA subscriber_list[], unsigned int subscriber_number, uint8_t technology)
{
	for(int j=0; j<10; j++)
	{
		if(subscriber_list[j].subscriber_number == subscriber_number && subscriber_list[j].technology == technology) {
			return subscriber_list[j].status;
		}

		else if(subscriber_list[j].subscriber_number == subscriber_number && subscriber_list[j].technology != technology)
		{
			return TECHNOLOGY_DOES_NOT_MATCH;
		}
	}

	return SUBSCRIBER_DOES_NOT_EXIST;
}


int main(int argc, char const *argv[])
{
	struct _REQUEST_PACKET request_packet;
	struct _RESPONSE_PACKET response_packet;
	struct _SUBSCRIBER_DATA subscriber_list[10];

	int socket_descriptor, n;
    int expected_packet = 1;
    char record[30];
    int subscriber_count = 0;

    FILE *fp;
    fp  = fopen("Verification_Database.txt", "rt");
    if(fp == NULL)
	{
		perror("[ERROR] Cannot Open File!\n");
		exit(0);
	}
	while(fgets(record, sizeof(record), fp) != NULL)
	{
		char * token = NULL;
		token = strtok(record," ");
		subscriber_list[subscriber_count].subscriber_number =(unsigned) atol(token);
		token = strtok(NULL," ");
		subscriber_list[subscriber_count].technology = atoi(token);
		token = strtok(NULL," ");
		subscriber_list[subscriber_count].status = atoi(token);
		subscriber_count++;
	}

    struct sockaddr_in socket_address;
    struct sockaddr_storage buffer;
    socklen_t address_size;

    socket_descriptor=socket(AF_INET,SOCK_DGRAM,0);
    bzero(&socket_address,sizeof(socket_address));
    socket_address.sin_family = AF_INET;
    socket_address.sin_addr.s_addr=htonl(INADDR_ANY);
    socket_address.sin_port=htons(PORT);
    bind(socket_descriptor,(struct sockaddr *)&socket_address,sizeof(socket_address));
    address_size = sizeof(socket_address);

    printf("\n");
    printf("[SUCCESS] Starting Server! Press \'Ctrl+C\' to close.\n\n");

    while(1)
	{
		// recieve request packet from the client
		n = recvfrom(socket_descriptor,&request_packet,sizeof(struct _REQUEST_PACKET),0,(struct sockaddr *)&buffer, &address_size);
		print_packet(request_packet);

		if(n > 0)
		{
			response_packet = generate_response_packet(request_packet);

			// check status for requested subscriber
			int status = search_database(subscriber_list, request_packet.source_subscriber_number, request_packet.technology);

			if(status == 0)
			{
				response_packet.status_code = NOT_PAID;
				printf("[STATUS] Subscriber has Not Paid!\n");
			}

			else if(status == 1)
			{
				response_packet.status_code = ACCESS_OK;
				printf("[STATUS] Subscriber has been Granted Access to the Network!\n");
			}

			else if(status == SUBSCRIBER_DOES_NOT_EXIST)
			{
				response_packet.status_code = NOT_EXIST; 
				printf("[STATUS] Subscriber number is Not Found!\n");
			}

			else 
			{
				response_packet.status_code = NOT_EXIST;
				printf("[STATUS] Subscriber number is found, but the technology Does Not Match\n");
			}

			sendto(socket_descriptor,&response_packet,sizeof(struct _RESPONSE_PACKET),0,(struct sockaddr *)&buffer,address_size);
		}
		n = 0;
		printf("\n###########################################################\n");
	}   
	
	fclose(fp);
	return 0;
}
