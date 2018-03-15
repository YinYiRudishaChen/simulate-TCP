#define SRV_IP "127.0.0.1"
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
#define BUF_SIZE 1000

#define SLOW_START 1
#define C_AVOID 2
#define Fast_Recovery 3

 /* diep(), #includes and #defines like in the server */

struct sockaddr_in initial(){
	struct sockaddr_in si_other;

	memset((char *) &si_other, 0, sizeof(si_other));
    	si_other.sin_family = AF_INET;
    	si_other.sin_port = htons(PORT);
    	if (inet_aton(SRV_IP, &si_other.sin_addr)==0){
    		fprintf(stderr, "inet_aton() failed\n");
    		return si_other;
    	}

	return si_other;
}

 
int connect_socket(void){
     	int s;
     
    	if ((s=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP))==-1)
    		perror("socket");
	
    	return s;
}

void close_socket(int s){
	close(s);
}

void print_cwnd(int cwnd){
	printf("CWND = %d\n", cwnd);
}

void print_duplicate(){
	printf("3 duplicate ack\n");
}

void print_timeout(){
	printf("Time out\n");
}

int main(void){
	struct sockaddr_in si_other, recv_ip;
	int s, i, j, slen=sizeof(si_other), rlen = sizeof(recv_ip);
    char buf[BUFLEN];

	struct timeval tv;
  	fd_set readfds;

	si_other = initial();
	s = connect_socket();

	int send_buffer[NPACK + 1];
	int cwnd = 1;
	double cwnd_double = 1.0;
	int lastByteSent = 0;
	int lastByteAcked = -1;
	int ssthresh = 10;

	int pre_acked = 0;
	int acked = 0;
	int dupACKcount = 0;
	int ackrcrd;
	int newACKcount = 0;

	int state = SLOW_START;

	for(i = 0; i < NPACK + 1; i++){
		send_buffer[i] = i;
	}

	//int test;

/* Insert your codes below */

	// send 1st packet (buffer[0])
	memset(buf, 0, BUFLEN);
	sprintf(buf, "%d", send_buffer[0]);
	sendto(s, buf, BUFLEN, 0, (struct sockaddr*) &si_other, (socklen_t)slen);

	while (lastByteSent < BUFLEN + 1)
	{
		//scanf("%d", &test);
		if (state == SLOW_START)  //=============================SLOW_START==========================
		{
			// timeout()
			tv.tv_sec = 1;
			tv.tv_usec = 0;
			FD_ZERO(&readfds);
			FD_SET(s, &readfds);

			select(s + 1, &readfds, NULL, NULL, &tv);
			if (!FD_ISSET(s, &readfds))
			{
				print_timeout();  //print_timeout
				ssthresh = cwnd / 2;
				cwnd = 1;
				dupACKcount = 0;

				memset(buf, 0, BUFLEN);
				sprintf(buf, "%d", send_buffer[acked]);  // acked initial = 0
				sendto(s, buf, BUFLEN, 0, (struct sockaddr*) &si_other, (socklen_t)slen);
			}
			else
			{
				if (recvfrom(s, buf, BUFLEN, 0, (struct sockaddr*) &si_other, (socklen_t*)&slen) != -1)
				{
					acked = atoi(buf);  // receive ack!!
					if (acked > pre_acked)  // if ack is new
					{
						cwnd++; print_cwnd(cwnd);
						//cwnd = cwnd + (acked - lastByteAcked - 1);  print_cwnd(cwnd);
						// 上面那行應該是錯的, 因為有可能回來的ack隔了好幾個sequence(這幾個sequence很可能之前
						// 被用來傳duplicate ACKs), 那這樣一次cwnd就加了很多. 
						lastByteAcked = acked - 1;
						dupACKcount = 0;

						pre_acked = acked; //update 前一個ack

						for (j = lastByteSent + 1; j < cwnd + acked; j++)  
						{
							memset(buf, 0, BUFLEN);
							sprintf(buf, "%d", send_buffer[j]);  
							sendto(s, buf, BUFLEN, 0, (struct sockaddr*) &si_other, (socklen_t)slen);
						}
						lastByteSent = j - 1;

						if (cwnd >= ssthresh)
						{   state = C_AVOID;   }
					}
					else  //acked <= pre_acked (receive duplicate ack)
					{
						dupACKcount++;
						if (dupACKcount < 3)
						{   print_cwnd(cwnd);   }
						else  // (dupACKcount >= 3)
						{
							state = Fast_Recovery;
							print_duplicate();  //print_duplicate
							ssthresh = cwnd / 2;
							cwnd = ssthresh + 3;  print_cwnd(cwnd);
							dupACKcount = 0;

							// retransmit missing segment
							memset(buf, 0, BUFLEN);
							sprintf(buf, "%d", send_buffer[acked]);
							sendto(s, buf, BUFLEN, 0, (struct sockaddr*) &si_other, (socklen_t)slen);
						}
					}
				}
				else // recvfrom() error;
					printf("recvfrom() fail!\n");
			}
		}
		else if (state == C_AVOID)  //==============================C_AVOID=============================
		{
			// timeout()
			tv.tv_sec = 1;
			tv.tv_usec = 0;
			FD_ZERO(&readfds);
			FD_SET(s, &readfds);

			select(s + 1, &readfds, NULL, NULL, &tv);
			if (!FD_ISSET(s, &readfds))
			{
				state = SLOW_START;
				print_timeout();  //print_timeout
				ssthresh = cwnd / 2;
				cwnd = 1;
				dupACKcount = 0;

				memset(buf, 0, BUFLEN);
				sprintf(buf, "%d", send_buffer[acked]);  // acked initial = 0
				sendto(s, buf, BUFLEN, 0, (struct sockaddr*) &si_other, (socklen_t)slen);
			}
			else
			{
				if (recvfrom(s, buf, BUFLEN, 0, (struct sockaddr*) &si_other, (socklen_t*)&slen) != -1)
				{
					acked = atoi(buf);  // receive ack!!
					if (acked > pre_acked)  // if ack is new
					{
						newACKcount++;
						lastByteAcked = acked - 1;
						//ackrcrd = lastByteAcked + cwnd;
						if (newACKcount == cwnd)
						{   
							cwnd++;  print_cwnd(cwnd);   
							newACKcount = 0;
						}
						else
						{   print_cwnd(cwnd);   }
						dupACKcount = 0;

						pre_acked = acked; //update 前一個ack

						for (j = lastByteSent + 1; j < cwnd + acked; j++)
						{
							memset(buf, 0, BUFLEN);
							sprintf(buf, "%d", send_buffer[lastByteSent]);
							sendto(s, buf, BUFLEN, 0, (struct sockaddr*) &si_other, (socklen_t)slen);
						}
						lastByteSent = j - 1;

						//if (acked >= ackrcrd)
						//{   ackrcrd = lastByteAcked + cwnd;   }
					}
					else  //acked <= pre_acked (receive duplicate ack)
					{
						//ackrcrd = lastByteAcked + cwnd;  //ackrcrd會比接收到new ACK時"小1", 因為cwnd沒++
						dupACKcount++;
						if (dupACKcount < 3)
						{   print_cwnd(cwnd);   }
						else  // (dupACKcount >= 3)
						{
							state = Fast_Recovery;
							print_duplicate();  //print_duplicate
							ssthresh = cwnd / 2;
							cwnd = ssthresh + 3;  print_cwnd(cwnd);
							dupACKcount = 0;

							// retransmit missing segment
							memset(buf, 0, BUFLEN);
							sprintf(buf, "%d", send_buffer[acked]);
							sendto(s, buf, BUFLEN, 0, (struct sockaddr*) &si_other, (socklen_t)slen);
						}
					}
				}
				else // recvfrom() error;
					printf("recvfrom() fail!\n");
			}
		}
		else if (state == Fast_Recovery)  //==========================Fast_Recovery========================
		{
			// timeout()
			tv.tv_sec = 1;
			tv.tv_usec = 0;
			FD_ZERO(&readfds);
			FD_SET(s, &readfds);

			select(s + 1, &readfds, NULL, NULL, &tv);
			if (!FD_ISSET(s, &readfds))
			{
				state = SLOW_START;
				print_timeout();  //print_timeout
				ssthresh = cwnd / 2;
				cwnd = 1;
				dupACKcount = 0;

				memset(buf, 0, BUFLEN);
				sprintf(buf, "%d", send_buffer[acked]);  // acked initial = 0
				sendto(s, buf, BUFLEN, 0, (struct sockaddr*) &si_other, (socklen_t)slen);
			}
			else
			{
				if (recvfrom(s, buf, BUFLEN, 0, (struct sockaddr*) &si_other, (socklen_t*)&slen) != -1)
				{
					acked = atoi(buf);  // receive ack!!
					if (acked > pre_acked)  // if ack is new
					{
						state = C_AVOID;
						lastByteAcked = acked - 1;
						cwnd = ssthresh;  print_cwnd(cwnd);  //print_cwnd
						dupACKcount = 0;

						pre_acked = acked;
					}
					else  //acked <= pre_acked (receive duplicate ack)
					{
						cwnd++;  print_cwnd(cwnd);

						for (j = lastByteSent + 1; j < cwnd + acked; j++)
						{
							memset(buf, 0, BUFLEN);
							sprintf(buf, "%d", send_buffer[lastByteSent]);
							sendto(s, buf, BUFLEN, 0, (struct sockaddr*) &si_other, (socklen_t)slen);
						}
						lastByteSent = j - 1;
					}

				}
				else // recvfrom() error;
					printf("recvfrom() fail!\n");
			}
		}
		else
			printf("state error!\n");
	}

/* Insert your codes above */

	close_socket(s);
	return 0;
}
