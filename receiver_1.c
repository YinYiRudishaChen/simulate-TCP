#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string.h>
     
#define BUFLEN 512
#define NPACK 50
#define PORT 9930
    
int main(void){
	struct sockaddr_in si_me, si_other;
	int s, i, slen=sizeof(si_other);
	char buf[BUFLEN];
	bool recv_buf[NPACK + 1];
	int recv_index = 1;
	bool lose = true;

	recv_buf[0] = true;
	for(i = 1; i < NPACK + 1; i++) recv_buf[i] = false;
    
	if ((s=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP))==-1)
		perror("socket");
    
    	memset((char *) &si_me, 0, sizeof(si_me));
    	si_me.sin_family = AF_INET;
    	si_me.sin_port = htons(PORT);
    	si_me.sin_addr.s_addr = htonl(INADDR_ANY);
    	if (bind(s, (const struct sockaddr*) &si_me, sizeof(si_me))==-1)
    		perror("bind");
    
    	//for (i=0; i<NPACK; i++) {
	while(recv_index < NPACK + 1){
    		if (recvfrom(s, buf, BUFLEN, 0, (struct sockaddr*) &si_other, (socklen_t*) &slen)==-1)
    			perror("recvfrom()");

		int ack = atoi(buf);

		/*
		if(ack == 2 && lose){
			lose = false;
			continue;
		}
		*/
		

		/*
		if(ack == 4) lose = true;
		if(ack == 5 && lose){
			lose = false;
			//return 0;
			continue;
		}
		*/


		recv_buf[ack] = true;

		while(recv_buf[recv_index] && recv_index < NPACK + 1) recv_index++;

    		printf("Received packet from %s:%d\nSeq: %d\n\n", 
    		inet_ntoa(si_other.sin_addr), ntohs(si_other.sin_port), ack);

		memset(buf, 0, BUFLEN);
		sprintf(buf, "%d", recv_index);

		sendto(s, buf, BUFLEN, 0, (struct sockaddr*) &si_other, (socklen_t) slen);	
    	}
    
    	close(s);
    	return 0;
}
