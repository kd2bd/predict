/*
 * fodtrack.c - fodtrack 'driver' for predict
 */

#define __USE_GNU

#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <linux/parport.h>
#include <linux/ppdev.h>

#define false 0
#define true 1

#define PSEUDODEVICEFILE "/tmp/fodtrackdevice"
#define CONFIGFILE "/etc/fodtrack.conf"

static int fodtrackfd;
static int azimuthcorrection;
static int elevationcorrection;
static int maxazimuth;
static char fodtrackdev[200];

/*
 * this function fills the global configuration variables
 * with the data from the config file
 */

void readconfig() {
   char buffer[255];
   FILE* f=NULL;
   printf("Reading configuration data from file %s.\n", CONFIGFILE);
   f=fopen(CONFIGFILE, "r");
   if (!f) {
      fprintf(stderr, "Unable to open configuration file - exiting.\n");
      exit(-1);
   }
   /* file reading loop */

   while(fgets(buffer, 255, f)) {
      if(!strncmp(buffer, "PARDEV", 6))
         sscanf(buffer, "PARDEV=%s\n", fodtrackdev);
      if(!strncmp(buffer, "MAXAZIMUTH", 10))
         sscanf(buffer, "MAXAZIMUTH=%i\n", &maxazimuth);
      if(!strncmp(buffer, "ELEVATIONCORRECTION", 17))
         sscanf(buffer, "ELEVATIONCORRECTION=%i\n", &elevationcorrection);
      if(!strncmp(buffer, "AZIMUTHCORRECTION", 17))
         sscanf(buffer, "AZIMUTHCORRECTION=%i\n", &azimuthcorrection);
   }
   fclose(f);
}


/*
 * functions to control the rotator
 */

/** Opens the parallel port. returns true on success and false on error */
int openfodtrack(const char* device)
{
  fodtrackfd=-1;
  fodtrackfd=open(device, O_RDWR);
  if(fodtrackfd==-1) {
    perror("Parport open");
    return false;
  }
  if(ioctl (fodtrackfd, PPCLAIM)) {
    perror("Parport claim");
    return false;
  }
  return true;
}

/** outputs an direction to the interface */
void setDirection(int angle, int direction){
  char outputvalue;
  char outputstatus;
  if (fodtrackfd==-1) return;
  if (direction)
    outputvalue=(char)(angle/(float)maxazimuth*255.0);
  else
    outputvalue=(char)(angle/180.0*255.0);
  // set the data bits
  ioctl(fodtrackfd, PPWDATA, &outputvalue);
  // autofd=true --> azimuth otherwhise elevation
  if(direction)
    outputstatus = PARPORT_CONTROL_AUTOFD;
  else
    outputstatus=0;
  ioctl(fodtrackfd, PPWCONTROL, &outputstatus);
  // and now the strobe impulse
  usleep(1);
  if(direction)
    outputstatus = PARPORT_CONTROL_AUTOFD | PARPORT_CONTROL_STROBE;
  else
    outputstatus = PARPORT_CONTROL_STROBE;
  ioctl(fodtrackfd, PPWCONTROL, &outputstatus);
  usleep(1);
  if (direction)
    outputstatus= PARPORT_CONTROL_AUTOFD;
  else
    outputstatus=0;
  ioctl(fodtrackfd, PPWCONTROL, &outputstatus);
}

/** sets the elevation */
void setElevation(int angle){
  int i;
  i = angle + elevationcorrection;
  if (i<0) i=0;
  if (i>180) i=180;
  setDirection(i, false);
}

/** Sets Azimuth */
void setAzimuth(int angle){
  int i;
  i = angle + azimuthcorrection;
  if (i>360) i=i-360;
  if (i<0) i=i+360;
  setDirection(i, true);
}

/*
 * main functions
 */

int main() {
   char buffer[200];
   int devfilefd, fd, i;
   float elevation, azimuth;

   readconfig();

   /* open the pseudo terminal device */
   fd=getpt();
   if (fd<0) {
      fprintf(stderr, "Unable to open serial line!\n");
      return -1;
   }
   unlockpt(fd);

   /* write the device file */

   devfilefd = open(PSEUDODEVICEFILE, O_WRONLY|O_CREAT, 0666);
   write(devfilefd, (char*)ptsname(fd), strlen((char*)ptsname(fd)));
   close(devfilefd);


   /* open the fodtrack device */

   openfodtrack(fodtrackdev);

   /* print informations about the devices */

   printf("Using %s as pseudo terminal device.\n", ptsname(fd));
   printf("Using %s as port for the fodtrack device.\n", fodtrackdev);

   /* going for the background */

   daemon(0,0);


   /* Main loop - reads from the device, and sets the antenna on the output */

   for(;;) {
      usleep(100);
      i=read(fd, buffer, 199);
      if(i>0) {
          sscanf(buffer, "AZ%f  EL%f", &azimuth, &elevation);
          /*printf("New Data:\nElevation: %f\nAzimuth: %f\n\n", elevation, azimuth);*/
          setElevation(elevation);
          setAzimuth(azimuth);
      }
   }
}
