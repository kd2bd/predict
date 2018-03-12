    /*****************************************/
   /**   Geostationary Satellite Locator   **/
  /**           KD2BD Software            **/
 /**            (c) 1992, 1999           **/
/*****************************************/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#ifndef PI
#define PI 3.141592653589793
#endif

double sgn(x)
double x;
{
	double value=0.0;

	if (x>0.0)
		value=1.0;
	
	if (x<0.0)
		value=-1.0;

	return value;
}

int main()
{
	double l, g, dr, c, dc, ds, j, az, el, cd;
	char qthfile[50], input[7];
	FILE *fd;

	sprintf(qthfile,"%s/.predict/predict.qth",getenv("HOME"));

	/* Read user's lat and long from ~/.predict/predict.qth file */

	fd=fopen(qthfile,"r");

	if (fd!=NULL)
	{
		fscanf(fd,"%s",input);
		fscanf(fd,"%lf",&l);
		fscanf(fd,"%lf",&g);
		fclose(fd);
	}

	else
	{
		printf("%c\n*** ERROR!  \"%s\" not found!  Program aborted!\n",7,qthfile);
		exit(-1);
	}

	input[0]=0;

	dr=PI/180.0;

	l=l*dr;
	g=g*dr;

	printf("\nEnter Longitude of Satellite in Degrees West: ");
	fgets(input,5,stdin);

	while (input[0]!='\n')
	{
		sscanf(input,"%lf",&cd);

		c=cd*dr;
		dc=cos(l)*cos(c-g);  
		ds=sqrt(1.0-(dc*dc));

		j=-sin(l)*dc/(cos(l)*ds);

		if (fabs(j)>0.9999)
			j=sgn(j)*0.9999;

		az=90.0-atan(j/sqrt(1.0-j*j))/dr;

		if (((c>g) && (c<(PI+g))) || ((c>(2.0*PI+g))))
			az=360.0-az;

		el=atan((42171.0*dc-6371.0)/(42171.0*ds))/dr;

		printf("\nAzimuth   : %6.2f degrees\n",az);
		printf("Elevation : %6.2f degrees\n\n",el);

		printf("\nEnter Longitude of Satellite in Degrees West: ");
		fgets(input,5,stdin);
	}

	exit(0);
}
