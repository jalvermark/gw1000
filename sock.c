#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/endian.h>
#include <sys/types.h>
#include <errno.h>
#include <math.h>

double calculate_dew_point(double T, double RH)
{
  // Standard coefficients for liquid water
  const double a = 17.27;
  const double b = 237.3;
  
  // Intermediate gamma calculation
  double gamma = log(RH / 100.0) + ((a * T) / (b + T));
    
  // Final dew point calculation
  return (b * gamma) / (a - gamma);
}


int main(int argc, char **argv)
{
  int s,ch,rain=0,battsig=0,exitval=0,debug=0,all=0,knots=0;
  short *msglen;
  struct sockaddr_in sin;
  char cmd[5];
  unsigned char buf[1024],sum=0;
  char *host;
  size_t readlen;
  double inT=-400,inH=-400,outT=-400,outH=-400;
  //struct timeval tcptimeout;
  u_int tcptimeout=5;

  //tcptimeout.tv_sec=5;
  //tcptimeout.tv_usec=0;


  while ((ch = getopt(argc, argv, "d:bri:ak")) != -1)
  {
    switch(ch)
    {
      case 'i':
        host=optarg;
        break;
      case 'r':
        rain=1;
        break;
      case 'b':
        battsig=1;
        break;
      case 'a':
        all=1;
        break;
      case 'k':
        knots=1;
        break;
      case 'd':
        debug=atoi(optarg);
        break;
      default:
        ;
    }

  }

  // printf("host: %s\n",host);

  start:

  s=socket(PF_INET,SOCK_STREAM,0);

  if(s<0)
  {
    printf("socket error\n");
    printf("%s\n",strerror(errno));
    return 1;
  }

  if(setsockopt(s,IPPROTO_TCP,TCP_KEEPINIT,&tcptimeout,sizeof(tcptimeout)))
  {
    printf("setsockopt error\n");
    printf("%s\n",strerror(errno));
    return 1;
  }

  sin.sin_family=AF_INET;
  sin.sin_port = htons(45000);
  //if (inet_pton(AF_INET,"192.168.0.104",&sin.sin_addr) <= 0)
  if (inet_pton(AF_INET,host,&sin.sin_addr) <= 0)
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
  if(rain)
  {
    cmd[2]=0x57;
    cmd[3]=0x03;
    cmd[4]=0x5a;
  }

  // Read sensor ID new
  //cmd[2]=0x3c;
  //cmd[3]=0x03;
  //cmd[4]=0x3f;

  if(battsig)
  {
  cmd[2]=0x3c;
  cmd[3]=0x03;
  cmd[4]=0x3f;

  }

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
  //printf("msglen=%hd\n",(short)bswap16(*msglen));
  if((short)bswap16(*msglen) > 1000)
  {
    printf("Too big msglen=%hd!\n",(short)bswap16(*msglen));
    close(s);
    return(1);
  }
 
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

  // yes I know
  if (battsig) goto battsig;
  // decoder for sensor data or rain data
  for(int f=5; f < (readlen + 4); f++)
  {
    short *short_int;
    int *normal_int;
    float t;

    switch (buf[f])
    {
      case 0x1:
        short_int=(void *)(&buf[f+1]);
        f+=sizeof(short);
        t=(float)(short)bswap16(*short_int)/10;
        inT=(double)t;
        printf("InTemp: %.01f\n",t);
        break;
      case 0x2:
        short_int=(void *)(&buf[f+1]);
        f+=sizeof(short);
        t=(float)(short)bswap16(*short_int)/10;
        outT=(double)t;
        printf("OutTemp: %.01f\n",t);
        break;
      case 0x6:
        inH=(double)buf[f+1];
        printf("InHumi: %d\n",(char) buf[++f]);
        break;
      case 0x7:
        outH=(double)buf[f+1];
        printf("OutHumi: %d\n",(char) buf[++f]);
        break;
      case 0x8:
        short_int=(void *)(&buf[f+1]);
        f+=sizeof(short);
        t=(float)bswap16(*short_int)/10;
        printf("RelBaro: %.01f\n",t);
        break;
      case 0x9:
        short_int=(void *)(&buf[f+1]);
        f+=sizeof(short);
        t=(float)bswap16(*short_int)/10;
        printf("AbsBaro: %.01f\n",t);
        break;
      case 0xa:
        short_int=(void *)(&buf[f+1]);
        f+=sizeof(short);
        printf("WindDir: %d\n",bswap16(*short_int));
        break;
      case 0xb:
        short_int=(void *)(&buf[f+1]);
        f+=sizeof(short);
        t=(float)bswap16(*short_int)/10;
        if(knots)
          t=(float)bswap16(*short_int)/10*1.94384;
        printf("WindSpeed: %.01f\n",t);
        break;
      case 0xc:
        short_int=(void *)(&buf[f+1]);
        f+=sizeof(short);
        t=(float)bswap16(*short_int)/10;
        printf("WindGust: %.01f\n",t);
        break;
      case 0xd:
        short_int=(void *)(&buf[f+1]);
        f+=sizeof(short);
        t=(float)bswap16(*short_int)/10;
        if(knots)
          t=(float)bswap16(*short_int)/10*1.94384;
        printf("RainRate: %.01f\n",t);
        break;
      case 0xe:
        short_int=(void *)(&buf[f+1]);
        f+=sizeof(short);
        t=(float)bswap16(*short_int)/10;
        printf("RainEvent: %.01f\n",t);
        break;
      case 0xf:
        short_int=(void *)(&buf[f+1]);
        f+=sizeof(short);
        t=(float)bswap16(*short_int)/10;
        printf("RainHour: %.01f\n",t);
        break;
      case 0x10:
        if(rain)
        {
          normal_int=(void *)(&buf[f+1]);
          f+=sizeof(int);
          t=(float)bswap32(*normal_int)/10;
          printf("RainDay: %.01f\n",t);
        }
        else
        {
          short_int=(void *)(&buf[f+1]);
          f+=sizeof(short);
          t=(float)bswap16(*short_int)/10;
          printf("RainDay: %.01f\n",t);
        }
        break;
      case 0x11:
        if(rain)
        {
          normal_int=(void *)(&buf[f+1]);
          f+=sizeof(int);
          t=(float)bswap32(*normal_int)/10;
          printf("RainWeek: %.01f\n",t);
        }
        else
        {
          short_int=(void *)(&buf[f+1]);
          f+=sizeof(short);
          t=(float)bswap16(*short_int)/10;
          printf("RainWeek: %.01f\n",t);
        }
        break;
      case 0x12:
        normal_int=(void *)(&buf[f+1]);
        f+=sizeof(int);
        t=(float)bswap32(*normal_int)/10;
        printf("RainMonth: %.01f\n",t);
        break;
      case 0x13:
        normal_int=(void *)(&buf[f+1]);
        f+=sizeof(int);
        t=(float)bswap32(*normal_int)/10;
        printf("RainYear: %.01f\n",t);
        break;
      case 0x15:
        normal_int=(void *)(&buf[f+1]);
        f+=sizeof(int);
        t=(float)bswap32(*normal_int)/10;
        printf("Light: %.01f\n",t);
        break;
      case 0x16:
        short_int=(void *)(&buf[f+1]);
        f+=sizeof(short);
        t=(float)bswap16(*short_int)/10;
        printf("UVLight: %.01f\n",t);
        break;
      case 0x17:
        printf("UVIndex: %d\n",(char) buf[++f]);
        break;
      case 0x19:
        short_int=(void *)(&buf[f+1]);
        f+=sizeof(short);
        t=(float)bswap16(*short_int)/10;
        printf("WindDayMax: %.01f\n",t);
        break;
      case 0x6c:
        normal_int=(void *)(&buf[f+1]);
        f+=sizeof(int);
        printf("HeapFree: %d\n",bswap32(*normal_int));
        break;
      case 0x7a:
        printf("Unknown 7a: -later-\n");
        f++;
        break;
      case 0x7b:
        printf("Unknown 7b: -later-\n");
        f++;
        break;
      case 0x80:
        short_int=(void *)(&buf[f+1]);
        f+=sizeof(short);
        t=(float)bswap16(*short_int)/10;
        printf("PRainRate: %.01f\n",t);
        break;
      case 0x81:
        short_int=(void *)(&buf[f+1]);
        f+=sizeof(short);
        t=(float)bswap16(*short_int)/10;
        printf("PRainEvent: %.01f\n",t);
        break;
      case 0x82:
        short_int=(void *)(&buf[f+1]);
        f+=sizeof(short);
        t=(float)bswap16(*short_int)/10;
        printf("PRainHour: %.01f\n",t);
        break;
      case 0x83:
        normal_int=(void *)(&buf[f+1]);
        f+=sizeof(int);
        t=(float)bswap32(*normal_int)/10;
        printf("PRainDay: %.01f\n",t);
        break;
      case 0x84:
        normal_int=(void *)(&buf[f+1]);
        f+=sizeof(int);
        t=(float)bswap32(*normal_int)/10;
        printf("PRainWeek: %.01f\n",t);
        break;
      case 0x85:
        normal_int=(void *)(&buf[f+1]);
        f+=sizeof(int);
        t=(float)bswap32(*normal_int)/10;
        printf("PRainMonth: %.01f\n",t);
        break;
      case 0x86:
        normal_int=(void *)(&buf[f+1]);
        f+=sizeof(int);
        t=(float)bswap32(*normal_int)/10;
        printf("PRainYear: %.01f\n",t);
        break;
      case 0x87:
        for(int item=0; item<10; item++)
        {
          short_int=(void *)(&buf[f+1]);
          f+=sizeof(short);
          t=(float)bswap16(*short_int)/100;
          printf("PGain%02d: %.02f\n",item+1,t);
        }
        break;
      case 0x88:
        printf("PRSTTime: -later-\n");
        f+=3;
        break;



      default:
        printf("Unknown code: 0x%0x\n",buf[f]);
    }
  } 
  if (inT > -400 && inH > -400)
    printf("InDewPoint: %.01f\n",calculate_dew_point(inT,inH));
  if (outT > -400 && outH > -400)
    printf("OutDewPoint: %.01f\n",calculate_dew_point(outT,outH));

  battsig:
  if(!battsig) goto end;
  // printf("batt\n");

  // battsig decoder
  for(int f=5; f < (readlen + 4); f++)
  {
    float t;
    unsigned int *id;

    id=(void *)(&buf[f+1]);
    if(bswap32(*id) == 0xfffffffe)
    {
      // printf("N/C\n");
      f+=6;
      continue;
    }
    else
    {
      printf("Sensor 0x%0x\n",buf[f]);
      printf("Sensor ID: %0x\n",bswap32(*id));
    }
    f+=4;

    switch (buf[f-4])
    {
      case 0x0:
        printf("Battery: %s\n",buf[++f] ? "Low" : "OK");
        if(buf[f])
          exitval=3;
        printf("Signal: %0x",buf[++f]);
        if(buf[f] < 2)
        {
          printf(" BAD");
          exitval=4;
        }
        printf("\n");
        break;
      case 0x30:
        t=(float)buf[++f] * .02;
        printf("Battery: %.02f V",t);
        if(t < 2.4)
        {
          printf(" Low");
          exitval=3;
        }
        printf("\n");
        printf("Signal: %0x",buf[++f]);
        if(buf[f] < 2)
        {
          printf(" BAD");
          exitval=4;
        }
        printf("\n");
        break;

      default:
        printf("Unimplemented sensor: 0x%0x\n",buf[f-4]);
        f+=2;
    }
  }

  end:

  if(all)
  {
    rain=1;
    close(s);
    sum=0;
    all=0;
    goto start;
  }

  if(debug==1)
  {
    for(int bufpos=0; bufpos<(readlen+4); bufpos++)
    {
      if(bufpos % 8 == 0)
        printf("\n");
      printf("%02x ",buf[bufpos]);
    }
    printf("\n");
  }

  if(debug>1)
    exitval=debug;



  close(s);
  return exitval;

}
