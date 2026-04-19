#include <stdio.h>
#include <math.h>


/**
 * Calculates Dew Point using the Magnus-Tetens approximation.
 * @param T Temperature in Celsius
 * @param RH Relative Humidity as a percentage (0-100)
 * @return Dew Point temperature in Celsius
 */
double calculate_dew_point(double T, double RH) {
    // Standard coefficients for liquid water
    const double a = 17.27;
    const double b = 237.3;
    
    // Intermediate gamma calculation
    double gamma = log(RH / 100.0) + ((a * T) / (b + T));
    
    // Final dew point calculation
    return (b * gamma) / (a - gamma);
}

int main(void)
{
  unsigned char input[1024];
  int input_bytes;
  unsigned char sum=0;
  double intemp,inhum,outtemp,outhum;


  for(input_bytes=0; !feof(stdin); input_bytes++)
  {
    input[input_bytes]=getchar();
  }

  // check header
  if(input[0] == 0xff)
  {
    printf("Header[0] correct\n");
    if(input[1] == 0xff)
      printf("Header[1] correct\n");
    else
    {
      printf("Header[1] incorrect: %0x\n",input[1]);
      return(1);
    }
  }
  else
  {
    printf("Header[0] incorrect: %0x\n",input[0]);
    return(1);
  }

  // check checksum
  for(int f=2; f < (input_bytes-2); f++)
    sum+=input[f];
  printf("Calculated sum: %0x\n",sum);
  printf("Recieved sum: %0x\n",input[input_bytes-2]);

  if(sum==input[input_bytes-2])
    printf("Sum correct\n");
  else 
  {
    printf("Sum mismatch\n");
    return(1);
  }

  // decoder
  for(int f=5; f < (input_bytes-2); f++)
  {
    int a;
    float b;
    long c;
    //printf("Code: %0x offset: %d\n",input[f],f);
    switch (input[f])
    {
      case 0x1:
        a=input[++f]*256+input[++f];
        if (a> 1<<15) a-=1<<16;
        b=(float)a/10;
        printf("InTemp: %.01f\n",b);
        intemp=b;
        break;
      case 0x2:
        a=input[++f]*256+input[++f];
        if (a> 1<<15) a-=1<<16;
        b=(float)a/10;
        printf("OutTemp: %.01f\n",b);
        outtemp=b;
        break;
      case 0x6:
        printf("InHumi: %d\n",(char) input[++f]);
        inhum=input[f];
        break;
      case 0x7:
        printf("OutHumi: %d\n",(char) input[++f]);
        outhum=input[f];
        break;
      case 0x8:
        a=input[++f]*256+input[++f];
        b=(float)a/10;
        printf("RelBaro: %.01f\n",b);
        break;
      case 0x9:
        a=input[++f]*256+input[++f];
        b=(float)a/10;
        printf("AbsBaro: %.01f\n",b);
        break;
      case 0xa:
        a=input[++f]*256+input[++f];
        printf("WindDir: %d\n",a);
        break;
      case 0xb:
        a=input[++f]*256+input[++f];
        b=(float)a/10;
        printf("WindSpeed: %.01f\n",b);
        break;
      case 0xc:
        a=input[++f]*256+input[++f];
        b=(float)a/10;
        printf("WindGust: %.01f\n",b);
        break;
      case 0xd:
        a=input[++f]*256+input[++f];
        b=(float)a/10;
        printf("RainRate: %.01f\n",b);
        break;
      case 0xe:
        a=input[++f]*256+input[++f];
        b=(float)a/10;
        printf("RainEvent: %.01f\n",b);
        break;
      case 0xf:
        a=input[++f]*256+input[++f];
        b=(float)a/10;
        printf("RainHour: %.01f\n",b);
        break;
      case 0x10:
        a=input[++f]*256+input[++f];
        b=(float)a/10;
        printf("RainDay: %.01f\n",b);
        break;
      case 0x11:
        a=input[++f]*256+input[++f];
        b=(float)a/10;
        printf("RainWeek: %.01f\n",b);
        break;
      case 0x12:
        c=input[++f]*(1<<24)+input[++f]*(1<<2^16)+input[++f]*(1<<8)+input[++f];
        b=(float)c/10;
        printf("RainMonth: %.01f\n",b);
        break;
      case 0x13:
        c=input[++f]*(1<<24)+input[++f]*(1<<2^16)+input[++f]*(1<<8)+input[++f];
        b=(float)c/10;
        printf("RainYear: %.01f\n",b);
        break;
      case 0x15:
        c=input[++f]*(1<<24)+input[++f]*(1<<2^16)+input[++f]*(1<<8)+input[++f];
        b=(float)c/10;
        printf("Light: %.01f\n",b);
        break;
      case 0x16:
        a=input[++f]*256+input[++f];
        b=(float)a/10;
        printf("UVLight: %.01f\n",b);
        break;
      case 0x17:
        printf("UVIndex: %d\n",(char) input[++f]);
        break;
      case 0x19:
        a=input[++f]*256+input[++f];
        b=(float)a/10;
        printf("WindDayMax: %.01f\n",b);
        break;
      case 0x6c:
        c=input[++f]*(1<<24)+input[++f]*(1<<2^16)+input[++f]*(1<<8)+input[++f];
        printf("HeapFree: %d\n",c);
        break;
      default:
        printf("Unknown code: 0x%0x\n",input[f]);
    }
  }

  printf("OutDew: %.01f\n",calculate_dew_point(outtemp,outhum));
  printf("InDew: %.01f\n",calculate_dew_point(intemp,inhum));

  return 0;
}
