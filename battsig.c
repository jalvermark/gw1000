#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/endian.h>
#include <errno.h>
#include "sensors.h"


int main(void)
{
  int s;
  short *msglen;
  struct sockaddr_in sin;
  char cmd[5];
  unsigned char buf[1024],sum=0;
  size_t readlen;

  s=socket(PF_INET,SOCK_STREAM,0);

  if(s<0)
  {
    printf("socket error\n");
    printf("%s\n",strerror(errno));
    return 1;
  }
  // printf("s=%d\n",s);

  sin.sin_family=AF_INET;
  sin.sin_port = htons(45000);
  //if (inet_pton(AF_INET,"127.0.0.1",&sin.sin_addr) <= 0)
  if (inet_pton(AF_INET,"192.168.67.19",&sin.sin_addr) <= 0)
  {
    perror("Invalid address/ Address not supported");
    close(s);
    return 1;
  }

  if (connect(s,(struct sockaddr *) &sin,sizeof(sin)) < 0)
  {
    perror("Connection failed");
    close(s);
    return 1;
  }

  // Header
  cmd[0]=0xff;
  cmd[1]=0xff;

  // Get live data
  cmd[2]=0x27;
  cmd[3]=0x03;
  cmd[4]=0x2a;

  // Read rain data
  cmd[2]=0x57;
  cmd[3]=0x03;
  cmd[4]=0x5a;

  // Read sensor ID new
  cmd[2]=0x3c;
  cmd[3]=0x03;
  cmd[4]=0x3f;

  // send request
  if(send(s,&cmd,5,0) < 5)
  {
    perror("Send error");
    close(s);
    return 1;
  }

  // Receive header
  if(recv(s,&buf,5,0) != 5)
  {
    perror("Receive error");
    close(s);
    return 1;
  }
  if(buf[0] != 0xff || buf[1] != 0xff)
  {
    printf("Invalid header preamble received\n");
    close(s);
    return 1;
  }
  if(buf[2]!=cmd[2])
  {
    printf("Invalid header ID received\n");
    close(s);
    return 1;
  }

  
  msglen=(void *)(buf+3);
  // printf("msglen=%hd\n",bswap16(*msglen));
 
  readlen=bswap16(*msglen)-3;
  // printf("readlen=%zu\n",readlen);

  if(recv(s,&buf[5],readlen,0) != readlen)
  {
    perror("Recieve error\n");
    close(s);
    return 1;
  }

  // Calculate checksum
  for(int f=2; f < (readlen + 4); f++)
    sum+=buf[f];

  // printf("Calculated sum: %0x\n",sum);
  // printf("Recieved sum: %0x\n",buf[readlen+4]);

  if(sum != buf[readlen+4])
  {
    printf("Checksum error\n");
    close(s);
    return 1;
  }


  
  // for(int f=0; f < (readlen + 5); f++)
  //   printf("%c",buf[f]);

  // decoder
  for(int f=5; f < (readlen + 4); f++)
  {
    float t;
    unsigned int *id;

    printf("\nSensor 0x%0x\n",buf[f]);
    id=(void *)(&buf[f+1]);
    if(bswap32(*id) == 0xfffffffe)
    {
      printf("N/C\n");
      f+=6;
      continue;
    }
    else
      printf("Sensor ID: %0x\n",bswap32(*id));
    f+=4;

    switch (buf[f-4])
    {
      case 0x0:
        printf("Battery: %s\n",buf[++f] ? "Low" : "OK");
        printf("Signal: %0x\n",buf[++f]);
        break;
      case 0x30:
        t=(float)buf[++f] * .02;
        printf("Battery: %.02f V\n",t);
        printf("Signal: %0x\n",buf[++f]);
        break;

      default:
        printf("Unimplemented sensor: 0x%0x\n",buf[f-4]);
        f+=2;
    }
  } 


  close(s);
  return 0;

}
