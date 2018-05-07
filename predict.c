/***************************************************************************\
*          PREDICT: A satellite tracking/orbital prediction program         *
*          Project started 26-May-1991 by John A. Magliacane, KD2BD         *
*                        Last update: 04-May-2018                           *
*****************************************************************************
*                                                                           *
* This program is free software; you can redistribute it and/or modify it   *
* under the terms of the GNU General Public License as published by the     *
* Free Software Foundation; either version 2 of the License or any later    *
* version.                                                                  *
*                                                                           *
* This program is distributed in the hope that it will be useful,           *
* but WITHOUT ANY WARRANTY; without even the implied warranty of            *
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU         *
* General Public License for more details.                                  *
*                                                                           *
*****************************************************************************
*          See the "CREDITS" file for the names of those who have           *
*   generously contributed their time, talent, and effort to this project.  *
\***************************************************************************/

#include <math.h>
#include <time.h>
#include <sys/time.h>
#include <curses.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>

#include "predict.h"

/* Constants used by SGP4/SDP4 code */

#define	km2mi		0.621371		/* km to miles */
#define deg2rad		1.745329251994330E-2	/* Degrees to radians */
#define pi		3.14159265358979323846	/* Pi */
#define pio2		1.57079632679489656	/* Pi/2 */
#define x3pio2		4.71238898038468967	/* 3*Pi/2 */
#define twopi		6.28318530717958623	/* 2*Pi  */
#define e6a		1.0E-6
#define tothrd		6.6666666666666666E-1	/* 2/3 */
#define xj2		1.0826158E-3		/* J2 Harmonic (WGS '72) */
#define xj3		-2.53881E-6		/* J3 Harmonic (WGS '72) */   
#define xj4		-1.65597E-6		/* J4 Harmonic (WGS '72) */
#define xke		7.43669161E-2
#define xkmper		6.378137E3		/* WGS 84 Earth radius km */
#define xmnpda		1.44E3			/* Minutes per day */
#define ae		1.0
#define ck2		5.413079E-4
#define ck4		6.209887E-7
#define f		3.35281066474748E-3	/* Flattening factor */
#define ge		3.986008E5 	/* Earth gravitational constant (WGS '72) */
#define s		1.012229
#define qoms2t		1.880279E-09
#define secday		8.6400E4	/* Seconds per day */
#define omega_E		1.00273790934	/* Earth rotations/siderial day */
#define omega_ER	6.3003879	/* Earth rotations, rads/siderial day */
#define zns		1.19459E-5
#define c1ss		2.9864797E-6
#define zes		1.675E-2
#define znl		1.5835218E-4
#define c1l		4.7968065E-7
#define zel		5.490E-2
#define zcosis		9.1744867E-1
#define zsinis		3.9785416E-1
#define zsings		-9.8088458E-1
#define zcosgs		1.945905E-1
#define zcoshs		1
#define zsinhs		0
#define q22		1.7891679E-6
#define q31		2.1460748E-6
#define q33		2.2123015E-7
#define g22		5.7686396
#define g32		9.5240898E-1
#define g44		1.8014998
#define g52		1.0508330
#define g54		4.4108898
#define root22		1.7891679E-6
#define root32		3.7393792E-7
#define root44		7.3636953E-9
#define root52		1.1428639E-7
#define root54		2.1765803E-9
#define thdt		4.3752691E-3
#define rho		1.5696615E-1
#define mfactor		7.292115E-5
#define sr		6.96000E5	/* Solar radius - km (IAU 76) */
#define AU		1.49597870691E8	/* Astronomical unit - km (IAU 76) */

/* Entry points of Deep() */

#define dpinit   1 /* Deep-space initialization code */
#define dpsec    2 /* Deep-space secular code        */
#define dpper    3 /* Deep-space periodic code       */

/* Flow control flag definitions */

#define ALL_FLAGS              -1
#define SGP_INITIALIZED_FLAG   0x000001	/* not used */
#define SGP4_INITIALIZED_FLAG  0x000002
#define SDP4_INITIALIZED_FLAG  0x000004
#define SGP8_INITIALIZED_FLAG  0x000008	/* not used */
#define SDP8_INITIALIZED_FLAG  0x000010	/* not used */
#define SIMPLE_FLAG            0x000020
#define DEEP_SPACE_EPHEM_FLAG  0x000040
#define LUNAR_TERMS_DONE_FLAG  0x000080
#define NEW_EPHEMERIS_FLAG     0x000100	/* not used */
#define DO_LOOP_FLAG           0x000200
#define RESONANCE_FLAG         0x000400
#define SYNCHRONOUS_FLAG       0x000800
#define EPOCH_RESTART_FLAG     0x001000
#define VISIBLE_FLAG           0x002000
#define SAT_ECLIPSED_FLAG      0x004000

struct	{  char line1[70];
	   char line2[70];
	   char name[25];
 	   long catnum;
	   long setnum;
	   char designator[10];
 	   int year;
	   double refepoch;
	   double incl;
	   double raan;
	   double eccn;
	   double argper;
	   double meanan;
	   double meanmo;
	   double drag;
	   double nddot6;
  	   double bstar;
	   long orbitnum;
	}  sat[24];

struct	{  char callsign[17];
	   double stnlat;
	   double stnlong;
	   int stnalt;
	}  qth;

struct	{  char name[25];
	   long catnum;
	   char squintflag;
	   double alat;
	   double alon;
	   unsigned char transponders;
	   char transponder_name[10][80];
	   double uplink_start[10];
	   double uplink_end[10];
	   double downlink_start[10];
	   double downlink_end[10];
	   unsigned char dayofweek[10];
	   int phase_start[10];
	   int phase_end[10];
	}  sat_db[24];

/* Global variables for sharing data among functions... */

double	tsince, jul_epoch, jul_utc, eclipse_depth=0,
	sat_azi, sat_ele, sat_range, sat_range_rate,
	sat_lat, sat_lon, sat_alt, sat_vel, phase,
	sun_azi, sun_ele, daynum, fm, fk, age, aostime,
	lostime, ax, ay, az, rx, ry, rz, squint, alat, alon,
	sun_ra, sun_dec, sun_lat, sun_lon, sun_range, sun_range_rate,
	moon_az, moon_el, moon_dx, moon_ra, moon_dec, moon_gha, moon_dv;

char	qthfile[50], tlefile[50], dbfile[50], temp[80], output[25],
	serial_port[15], resave=0, reload_tle=0, netport[7],
	once_per_second=0, ephem[5], sat_sun_status, findsun,
	calc_squint, database=0, xterm, io_lat='N', io_lon='W';

int	indx, antfd, iaz, iel, ma256, isplat, isplong, socket_flag=0,
	Flags=0;

long	rv, irk;

unsigned char val[256];

/* The following variables are used by the socket server.  They
   are updated in the MultiTrack() and SingleTrack() functions. */

char	visibility_array[24], tracking_mode[30];

float	az_array[24], el_array[24], long_array[24], lat_array[24],
	footprint_array[24], range_array[24], altitude_array[24],
	velocity_array[24], eclipse_depth_array[24], phase_array[24],
	squint_array[24];

double	doppler[24], nextevent[24];

long	aos_array[24], orbitnum_array[24];

unsigned short portbase=0;

/** Type definitions **/

/* Two-line-element satellite orbital data
   structure used directly by the SGP4/SDP4 code. */

typedef struct	{
		   double  epoch, xndt2o, xndd6o, bstar, xincl,
			   xnodeo, eo, omegao, xmo, xno;
 		   int	   catnr, elset, revnum;
 		   char	   sat_name[25], idesg[9];
		}  tle_t; 

/* Geodetic position structure used by SGP4/SDP4 code. */

typedef struct	{
		   double lat, lon, alt, theta;
		}  geodetic_t;

/* General three-dimensional vector structure used by SGP4/SDP4 code. */

typedef struct	{
		   double x, y, z, w;
		}  vector_t;

/* Common arguments between deep-space functions used by SGP4/SDP4 code. */

typedef struct	{
		   	   /* Used by dpinit part of Deep() */
		   double  eosq, sinio, cosio, betao, aodp, theta2,
			   sing, cosg, betao2, xmdot, omgdot, xnodot, xnodp;
	   
			   /* Used by dpsec and dpper parts of Deep() */
		   double  xll, omgadf, xnode, em, xinc, xn, t;
    
		 	   /* Used by thetg and Deep() */
		   double  ds50;
		}  deep_arg_t;

/* Global structure used by SGP4/SDP4 code. */

geodetic_t obs_geodetic;

/* Two-line Orbital Elements for the satellite used by SGP4/SDP4 code. */

tle_t tle;

/* Functions for testing and setting/clearing flags used in SGP4/SDP4 code */

int isFlagSet(int flag)
{
	return (Flags&flag);
}

int isFlagClear(int flag)
{
	return (~Flags&flag);
}

void SetFlag(int flag)
{
	Flags|=flag;
}

void ClearFlag(int flag)
{
	Flags&=~flag;
}

/* Remaining SGP4/SDP4 code follows... */

int Sign(double arg)
{
	/* Returns sign of a double */

	if (arg>0)
		return 1;
		
	else if (arg<0)
		return -1;
		
	else
		return 0;
}

double Sqr(double arg)
{
	/* Returns square of a double */
	return (arg*arg);
}

double Cube(double arg)
{
	/* Returns cube of a double */
	return (arg*arg*arg);
}

double Radians(double arg)
{
	/* Returns angle in radians from argument in degrees */
	return (arg*deg2rad);
}

double Degrees(double arg)
{
	/* Returns angle in degrees from argument in radians */
	return (arg/deg2rad);
}

double ArcSin(double arg)
{
	/* Returns the arcsine of the argument */

	if (fabs(arg)>=1.0)
		return(Sign(arg)*pio2);
	else

	return(atan(arg/sqrt(1.0-arg*arg)));
}

double ArcCos(double arg)
{
	/* Returns arccosine of argument */
	return(pio2-ArcSin(arg));
}

void Magnitude(vector_t *v)
{
	/* Calculates scalar magnitude of a vector_t argument */
	v->w=sqrt(Sqr(v->x)+Sqr(v->y)+Sqr(v->z));
}

void Vec_Add(vector_t *v1, vector_t *v2, vector_t *v3)
{
	/* Adds vectors v1 and v2 together to produce v3 */
	v3->x=v1->x+v2->x;
	v3->y=v1->y+v2->y;
	v3->z=v1->z+v2->z;
	Magnitude(v3);
}

void Vec_Sub(vector_t *v1, vector_t *v2, vector_t *v3)
{
	/* Subtracts vector v2 from v1 to produce v3 */
	v3->x=v1->x-v2->x;
	v3->y=v1->y-v2->y;
	v3->z=v1->z-v2->z;
	Magnitude(v3);
}

void Scalar_Multiply(double k, vector_t *v1, vector_t *v2)
{
	/* Multiplies the vector v1 by the scalar k to produce the vector v2 */
	v2->x=k*v1->x;
	v2->y=k*v1->y;
	v2->z=k*v1->z;
	v2->w=fabs(k)*v1->w;
}

void Scale_Vector(double k, vector_t *v)
{ 
	/* Multiplies the vector v1 by the scalar k */
	v->x*=k;
	v->y*=k;
	v->z*=k;
	Magnitude(v);
}

double Dot(vector_t *v1, vector_t *v2)
{
	/* Returns the dot product of two vectors */
	return (v1->x*v2->x+v1->y*v2->y+v1->z*v2->z);
}

double Angle(vector_t *v1, vector_t *v2)
{
	/* Calculates the angle between vectors v1 and v2 */
	Magnitude(v1);
	Magnitude(v2);
	return(ArcCos(Dot(v1,v2)/(v1->w*v2->w)));
}

void Cross(vector_t *v1, vector_t *v2 ,vector_t *v3)
{
	/* Produces cross product of v1 and v2, and returns in v3 */
	v3->x=v1->y*v2->z-v1->z*v2->y;
	v3->y=v1->z*v2->x-v1->x*v2->z;
	v3->z=v1->x*v2->y-v1->y*v2->x;
	Magnitude(v3);
}

void Normalize(vector_t *v)
{
	/* Normalizes a vector */
	v->x/=v->w;
	v->y/=v->w;
	v->z/=v->w;
}

double AcTan(double sinx, double cosx)
{
	/* Four-quadrant arctan function */

	if (cosx==0.0)
	{
		if (sinx>0.0)
			return (pio2);
		else
			return (x3pio2);
	}

	else
	{
		if (cosx>0.0)
		{
			if (sinx>0.0)
				return (atan(sinx/cosx));
			else
				return (twopi+atan(sinx/cosx));
		}

		else
			return (pi+atan(sinx/cosx));
	}
}

double FMod2p(double x)
{
	/* Returns mod 2PI of argument */

	int i;
	double ret_val;

	ret_val=x;
	i=ret_val/twopi;
	ret_val-=i*twopi;

	if (ret_val<0.0)
		ret_val+=twopi;

	return ret_val;
}

double Modulus(double arg1, double arg2)
{
	/* Returns arg1 mod arg2 */

	int i;
	double ret_val;

	ret_val=arg1;
	i=ret_val/arg2;
	ret_val-=i*arg2;

	if (ret_val<0.0)
		ret_val+=arg2;

	return ret_val;
}

double Frac(double arg)
{
	/* Returns fractional part of double argument */
	return(arg-floor(arg));
}

int Round(double arg)
{
	/* Returns argument rounded up to nearest integer */
	return((int)floor(arg+0.5));
}

double Int(double arg)
{
	/* Returns the floor integer of a double arguement, as double */
	return(floor(arg));
}

void Convert_Sat_State(vector_t *pos, vector_t *vel)
{
	/* Converts the satellite's position and velocity  */
	/* vectors from normalized values to km and km/sec */ 
	Scale_Vector(xkmper, pos);
	Scale_Vector(xkmper*xmnpda/secday, vel);
}

double Julian_Date_of_Year(double year)
{
	/* The function Julian_Date_of_Year calculates the Julian Date  */
	/* of Day 0.0 of {year}. This function is used to calculate the */
	/* Julian Date of any date by using Julian_Date_of_Year, DOY,   */
	/* and Fraction_of_Day. */

	/* Astronomical Formulae for Calculators, Jean Meeus, */
	/* pages 23-25. Calculate Julian Date of 0.0 Jan year */

	long A, B, i;
	double jdoy;

	year=year-1;
	i=year/100;
	A=i;
	i=A/4;
	B=2-A+i;
	i=365.25*year;
	i+=30.6001*14;
	jdoy=i+1720994.5+B;

	return jdoy;
}

double Julian_Date_of_Epoch(double epoch)
{ 
	/* The function Julian_Date_of_Epoch returns the Julian Date of     */
	/* an epoch specified in the format used in the NORAD two-line      */
	/* element sets. It has been modified to support dates beyond       */
	/* the year 1999 assuming that two-digit years in the range 00-56   */
	/* correspond to 2000-2056. Until the two-line element set format   */
	/* is changed, it is only valid for dates through 2056 December 31. */

	double year, day;

	/* Modification to support Y2K */
	/* Valid 1957 through 2056     */

	day=modf(epoch*1E-3, &year)*1E3;

	if (year<57)
		year=year+2000;
	else
		year=year+1900;

	return (Julian_Date_of_Year(year)+day);
}

int DOY (int yr, int mo, int dy)
{
	/* The function DOY calculates the day of the year for the specified */
	/* date. The calculation uses the rules for the Gregorian calendar   */
	/* and is valid from the inception of that calendar system.          */

	const int days[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
	int i, day;

	day=0;
	
	for (i=0; i<mo-1; i++ )
	    day+=days[i];

	day=day+dy;

	/* Leap year correction */

	if ((yr%4==0) && ((yr%100!=0) || (yr%400==0)) && (mo>2))
		day++;

	return day;
}

double Fraction_of_Day(int hr, int mi, double se)
{
	/* Fraction_of_Day calculates the fraction of */
	/* a day passed at the specified input time.  */

	double dhr, dmi;

	dhr=(double)hr;
	dmi=(double)mi;

	return ((dhr+(dmi+se/60.0)/60.0)/24.0);
}

double Julian_Date(struct tm *cdate)
{
	/* The function Julian_Date converts a standard calendar   */
	/* date and time to a Julian Date. The procedure Date_Time */
	/* performs the inverse of this function. */

	double julian_date;

	julian_date=Julian_Date_of_Year(cdate->tm_year)+DOY(cdate->tm_year,cdate->tm_mon,cdate->tm_mday)+Fraction_of_Day(cdate->tm_hour,cdate->tm_min,cdate->tm_sec)+5.787037e-06; /* Round up to nearest 1 sec */

	return julian_date;
}

void Date_Time(double julian_date, struct tm *cdate)
{
	/* The function Date_Time() converts a Julian Date to
	standard calendar date and time. The function
	Julian_Date() performs the inverse of this function. */

	time_t jtime;

	jtime=(julian_date-2440587.5)*86400.0;
	*cdate=*gmtime(&jtime);
}

double Delta_ET(double year)
{
	/* The function Delta_ET has been added to allow calculations on   */
	/* the position of the sun.  It provides the difference between UT */
	/* (approximately the same as UTC) and ET (now referred to as TDT).*/
	/* This function is based on a least squares fit of data from 1950 */
	/* to 1991 and will need to be updated periodically. */

	/* Values determined using data from 1950-1991 in the 1990 
	Astronomical Almanac.  See DELTA_ET.WQ1 for details. */

	double delta_et;

	delta_et=26.465+0.747622*(year-1950)+1.886913*sin(twopi*(year-1975)/33);

	return delta_et;
}

double ThetaG(double epoch, deep_arg_t *deep_arg)
{
	/* The function ThetaG calculates the Greenwich Mean Sidereal Time */
	/* for an epoch specified in the format used in the NORAD two-line */
	/* element sets. It has now been adapted for dates beyond the year */
	/* 1999, as described above. The function ThetaG_JD provides the   */
	/* same calculation except that it is based on an input in the     */
	/* form of a Julian Date. */

	/* Reference:  The 1992 Astronomical Almanac, page B6. */

	double year, day, UT, jd, TU, GMST, ThetaG;

	/* Modification to support Y2K */
	/* Valid 1957 through 2056     */

	day=modf(epoch*1E-3,&year)*1E3;

	if (year<57)
		year+=2000;
	else
		year+=1900;

	UT=modf(day,&day);
	jd=Julian_Date_of_Year(year)+day;
	TU=(jd-2451545.0)/36525;
	GMST=24110.54841+TU*(8640184.812866+TU*(0.093104-TU*6.2E-6));
	GMST=Modulus(GMST+secday*omega_E*UT,secday);
	ThetaG=twopi*GMST/secday;
	deep_arg->ds50=jd-2433281.5+UT;
	ThetaG=FMod2p(6.3003880987*deep_arg->ds50+1.72944494);

	return ThetaG;
}

double ThetaG_JD(double jd)
{
	/* Reference:  The 1992 Astronomical Almanac, page B6. */

	double UT, TU, GMST;

	UT=Frac(jd+0.5);
	jd=jd-UT;
	TU=(jd-2451545.0)/36525;
	GMST=24110.54841+TU*(8640184.812866+TU*(0.093104-TU*6.2E-6));
	GMST=Modulus(GMST+secday*omega_E*UT,secday);

	return (twopi*GMST/secday);
}

void Calculate_Solar_Position(double time, vector_t *solar_vector)
{
	/* Calculates solar position vector */

	double mjd, year, T, M, L, e, C, O, Lsa, nu, R, eps;

	mjd=time-2415020.0;
	year=1900+mjd/365.25;
	T=(mjd+Delta_ET(year)/secday)/36525.0;
	M=Radians(Modulus(358.47583+Modulus(35999.04975*T,360.0)-(0.000150+0.0000033*T)*Sqr(T),360.0));
	L=Radians(Modulus(279.69668+Modulus(36000.76892*T,360.0)+0.0003025*Sqr(T),360.0));
	e=0.01675104-(0.0000418+0.000000126*T)*T;
	C=Radians((1.919460-(0.004789+0.000014*T)*T)*sin(M)+(0.020094-0.000100*T)*sin(2*M)+0.000293*sin(3*M));
	O=Radians(Modulus(259.18-1934.142*T,360.0));
	Lsa=Modulus(L+C-Radians(0.00569-0.00479*sin(O)),twopi);
	nu=Modulus(M+C,twopi);
	R=1.0000002*(1.0-Sqr(e))/(1.0+e*cos(nu));
	eps=Radians(23.452294-(0.0130125+(0.00000164-0.000000503*T)*T)*T+0.00256*cos(O));
	R=AU*R;
	solar_vector->x=R*cos(Lsa);
	solar_vector->y=R*sin(Lsa)*cos(eps);
	solar_vector->z=R*sin(Lsa)*sin(eps);
	solar_vector->w=R;
}

int Sat_Eclipsed(vector_t *pos, vector_t *sol, double *depth)
{
	/* Calculates satellite's eclipse status and depth */

	double sd_sun, sd_earth, delta;
	vector_t Rho, earth;

	/* Determine partial eclipse */

	sd_earth=ArcSin(xkmper/pos->w);
	Vec_Sub(sol,pos,&Rho);
	sd_sun=ArcSin(sr/Rho.w);
	Scalar_Multiply(-1,pos,&earth);
	delta=Angle(sol,&earth);
	*depth=sd_earth-sd_sun-delta;

	if (sd_earth<sd_sun)
		return 0;
	else
		if (*depth>=0)
			return 1;
	else
		return 0;
}

void select_ephemeris(tle_t *tle)
{
	/* Selects the apropriate ephemeris type to be used */
	/* for predictions according to the data in the TLE */
	/* It also processes values in the tle set so that  */
	/* they are apropriate for the sgp4/sdp4 routines   */

	double ao, xnodp, dd1, dd2, delo, temp, a1, del1, r1;

	/* Preprocess tle set */
	tle->xnodeo*=deg2rad;
	tle->omegao*=deg2rad;
	tle->xmo*=deg2rad;
	tle->xincl*=deg2rad;
	temp=twopi/xmnpda/xmnpda;
	tle->xno=tle->xno*temp*xmnpda;
	tle->xndt2o*=temp;
	tle->xndd6o=tle->xndd6o*temp/xmnpda;
	tle->bstar/=ae;

	/* Period > 225 minutes is deep space */
	dd1=(xke/tle->xno);
	dd2=tothrd;
	a1=pow(dd1,dd2);
	r1=cos(tle->xincl);
	dd1=(1.0-tle->eo*tle->eo);
	temp=ck2*1.5f*(r1*r1*3.0-1.0)/pow(dd1,1.5);
	del1=temp/(a1*a1);
	ao=a1*(1.0-del1*(tothrd*.5+del1*(del1*1.654320987654321+1.0)));
	delo=temp/(ao*ao);
	xnodp=tle->xno/(delo+1.0);

	/* Select a deep-space/near-earth ephemeris */

	if (twopi/xnodp/xmnpda>=0.15625)
		SetFlag(DEEP_SPACE_EPHEM_FLAG);
	else
		ClearFlag(DEEP_SPACE_EPHEM_FLAG);
}

void SGP4(double tsince, tle_t * tle, vector_t * pos, vector_t * vel)
{
	/* This function is used to calculate the position and velocity */
	/* of near-earth (period < 225 minutes) satellites. tsince is   */
	/* time since epoch in minutes, tle is a pointer to a tle_t     */
	/* structure with Keplerian orbital elements and pos and vel    */
	/* are vector_t structures returning ECI satellite position and */ 
	/* velocity. Use Convert_Sat_State() to convert to km and km/s. */

	static double aodp, aycof, c1, c4, c5, cosio, d2, d3, d4, delmo,
	omgcof, eta, omgdot, sinio, xnodp, sinmo, t2cof, t3cof, t4cof,
	t5cof, x1mth2, x3thm1, x7thm1, xmcof, xmdot, xnodcf, xnodot, xlcof;

	double cosuk, sinuk, rfdotk, vx, vy, vz, ux, uy, uz, xmy, xmx, cosnok,
	sinnok, cosik, sinik, rdotk, xinck, xnodek, uk, rk, cos2u, sin2u,
	u, sinu, cosu, betal, rfdot, rdot, r, pl, elsq, esine, ecose, epw,
	cosepw, x1m5th, xhdot1, tfour, sinepw, capu, ayn, xlt, aynl, xll,
	axn, xn, beta, xl, e, a, tcube, delm, delomg, templ, tempe, tempa,
	xnode, tsq, xmp, omega, xnoddf, omgadf, xmdf, a1, a3ovk2, ao,
	betao, betao2, c1sq, c2, c3, coef, coef1, del1, delo, eeta, eosq,
	etasq, perigee, pinvsq, psisq, qoms24, s4, temp, temp1, temp2,
	temp3, temp4, temp5, temp6, theta2, theta4, tsi;

	int i;

	/* Initialization */

	if (isFlagClear(SGP4_INITIALIZED_FLAG))
	{
		SetFlag(SGP4_INITIALIZED_FLAG);

		/* Recover original mean motion (xnodp) and   */
		/* semimajor axis (aodp) from input elements. */

		a1=pow(xke/tle->xno,tothrd);
		cosio=cos(tle->xincl);
		theta2=cosio*cosio;
		x3thm1=3*theta2-1.0;
		eosq=tle->eo*tle->eo;
		betao2=1.0-eosq;
		betao=sqrt(betao2);
		del1=1.5*ck2*x3thm1/(a1*a1*betao*betao2);
		ao=a1*(1.0-del1*(0.5*tothrd+del1*(1.0+134.0/81.0*del1)));
		delo=1.5*ck2*x3thm1/(ao*ao*betao*betao2);
		xnodp=tle->xno/(1.0+delo);
		aodp=ao/(1.0-delo);

		/* For perigee less than 220 kilometers, the "simple"     */
		/* flag is set and the equations are truncated to linear  */
		/* variation in sqrt a and quadratic variation in mean    */
		/* anomaly.  Also, the c3 term, the delta omega term, and */
		/* the delta m term are dropped.                          */

		if ((aodp*(1-tle->eo)/ae)<(220/xkmper+ae))
		    SetFlag(SIMPLE_FLAG);

		else
		    ClearFlag(SIMPLE_FLAG);

		/* For perigees below 156 km, the      */
		/* values of s and qoms2t are altered. */

		s4=s;
		qoms24=qoms2t;
		perigee=(aodp*(1-tle->eo)-ae)*xkmper;

		if (perigee<156.0)
		{
			if (perigee<=98.0)
			    s4=20;
			else
		   	 s4=perigee-78.0;

			qoms24=pow((120-s4)*ae/xkmper,4);
			s4=s4/xkmper+ae;
		}

		pinvsq=1/(aodp*aodp*betao2*betao2);
		tsi=1/(aodp-s4);
		eta=aodp*tle->eo*tsi;
		etasq=eta*eta;
		eeta=tle->eo*eta;
		psisq=fabs(1-etasq);
		coef=qoms24*pow(tsi,4);
		coef1=coef/pow(psisq,3.5);
		c2=coef1*xnodp*(aodp*(1+1.5*etasq+eeta*(4+etasq))+0.75*ck2*tsi/psisq*x3thm1*(8+3*etasq*(8+etasq)));
		c1=tle->bstar*c2;
		sinio=sin(tle->xincl);
		a3ovk2=-xj3/ck2*pow(ae,3);
		c3=coef*tsi*a3ovk2*xnodp*ae*sinio/tle->eo;
		x1mth2=1-theta2;

		c4=2*xnodp*coef1*aodp*betao2*(eta*(2+0.5*etasq)+tle->eo*(0.5+2*etasq)-2*ck2*tsi/(aodp*psisq)*(-3*x3thm1*(1-2*eeta+etasq*(1.5-0.5*eeta))+0.75*x1mth2*(2*etasq-eeta*(1+etasq))*cos(2*tle->omegao)));
		c5=2*coef1*aodp*betao2*(1+2.75*(etasq+eeta)+eeta*etasq);

		theta4=theta2*theta2;
		temp1=3*ck2*pinvsq*xnodp;
		temp2=temp1*ck2*pinvsq;
		temp3=1.25*ck4*pinvsq*pinvsq*xnodp;
		xmdot=xnodp+0.5*temp1*betao*x3thm1+0.0625*temp2*betao*(13-78*theta2+137*theta4);
		x1m5th=1-5*theta2;
		omgdot=-0.5*temp1*x1m5th+0.0625*temp2*(7-114*theta2+395*theta4)+temp3*(3-36*theta2+49*theta4);
		xhdot1=-temp1*cosio;
		xnodot=xhdot1+(0.5*temp2*(4-19*theta2)+2*temp3*(3-7*theta2))*cosio;
		omgcof=tle->bstar*c3*cos(tle->omegao);
		xmcof=-tothrd*coef*tle->bstar*ae/eeta;
		xnodcf=3.5*betao2*xhdot1*c1;
		t2cof=1.5*c1;
		xlcof=0.125*a3ovk2*sinio*(3+5*cosio)/(1+cosio);
		aycof=0.25*a3ovk2*sinio;
		delmo=pow(1+eta*cos(tle->xmo),3);
		sinmo=sin(tle->xmo);
		x7thm1=7*theta2-1;

		if (isFlagClear(SIMPLE_FLAG))
		{
			c1sq=c1*c1;
			d2=4*aodp*tsi*c1sq;
			temp=d2*tsi*c1/3;
			d3=(17*aodp+s4)*temp;
			d4=0.5*temp*aodp*tsi*(221*aodp+31*s4)*c1;
			t3cof=d2+2*c1sq;
			t4cof=0.25*(3*d3+c1*(12*d2+10*c1sq));
			t5cof=0.2*(3*d4+12*c1*d3+6*d2*d2+15*c1sq*(2*d2+c1sq));
		}
	}

	/* Update for secular gravity and atmospheric drag. */
	xmdf=tle->xmo+xmdot*tsince;
	omgadf=tle->omegao+omgdot*tsince;
	xnoddf=tle->xnodeo+xnodot*tsince;
	omega=omgadf;
	xmp=xmdf;
	tsq=tsince*tsince;
	xnode=xnoddf+xnodcf*tsq;
	tempa=1-c1*tsince;
	tempe=tle->bstar*c4*tsince;
	templ=t2cof*tsq;
    
	if (isFlagClear(SIMPLE_FLAG))
	{
		delomg=omgcof*tsince;
		delm=xmcof*(pow(1+eta*cos(xmdf),3)-delmo);
		temp=delomg+delm;
		xmp=xmdf+temp;
		omega=omgadf-temp;
		tcube=tsq*tsince;
		tfour=tsince*tcube;
		tempa=tempa-d2*tsq-d3*tcube-d4*tfour;
		tempe=tempe+tle->bstar*c5*(sin(xmp)-sinmo);
		templ=templ+t3cof*tcube+tfour*(t4cof+tsince*t5cof);
	}

	a=aodp*pow(tempa,2);
	e=tle->eo-tempe;
	xl=xmp+omega+xnode+xnodp*templ;
	beta=sqrt(1-e*e);
	xn=xke/pow(a,1.5);

	/* Long period periodics */
	axn=e*cos(omega);
	temp=1/(a*beta*beta);
	xll=temp*xlcof*axn;
	aynl=temp*aycof;
	xlt=xl+xll;
	ayn=e*sin(omega)+aynl;

	/* Solve Kepler's Equation */
	capu=FMod2p(xlt-xnode);
	temp2=capu;
	i=0;

	do
	{
		sinepw=sin(temp2);
		cosepw=cos(temp2);
		temp3=axn*sinepw;
		temp4=ayn*cosepw;
		temp5=axn*cosepw;
		temp6=ayn*sinepw;
		epw=(capu-temp4+temp3-temp2)/(1-temp5-temp6)+temp2;
	  
		if (fabs(epw-temp2)<= e6a)
			break;
	      
		temp2=epw;

	} while (i++<10);

	/* Short period preliminary quantities */
	ecose=temp5+temp6;
	esine=temp3-temp4;
	elsq=axn*axn+ayn*ayn;
	temp=1-elsq;
	pl=a*temp;
	r=a*(1-ecose);
	temp1=1/r;
	rdot=xke*sqrt(a)*esine*temp1;
	rfdot=xke*sqrt(pl)*temp1;
	temp2=a*temp1;
	betal=sqrt(temp);
	temp3=1/(1+betal);
	cosu=temp2*(cosepw-axn+ayn*esine*temp3);
	sinu=temp2*(sinepw-ayn-axn*esine*temp3);
	u=AcTan(sinu,cosu);
	sin2u=2*sinu*cosu;
	cos2u=2*cosu*cosu-1;
	temp=1/pl;
	temp1=ck2*temp;
	temp2=temp1*temp;

	/* Update for short periodics */
	rk=r*(1-1.5*temp2*betal*x3thm1)+0.5*temp1*x1mth2*cos2u;
	uk=u-0.25*temp2*x7thm1*sin2u;
	xnodek=xnode+1.5*temp2*cosio*sin2u;
	xinck=tle->xincl+1.5*temp2*cosio*sinio*cos2u;
	rdotk=rdot-xn*temp1*x1mth2*sin2u;
	rfdotk=rfdot+xn*temp1*(x1mth2*cos2u+1.5*x3thm1);

	/* Orientation vectors */
	sinuk=sin(uk);
	cosuk=cos(uk);
	sinik=sin(xinck);
	cosik=cos(xinck);
	sinnok=sin(xnodek);
	cosnok=cos(xnodek);
	xmx=-sinnok*cosik;
	xmy=cosnok*cosik;
	ux=xmx*sinuk+cosnok*cosuk;
	uy=xmy*sinuk+sinnok*cosuk;
	uz=sinik*sinuk;
	vx=xmx*cosuk-cosnok*sinuk;
	vy=xmy*cosuk-sinnok*sinuk;
	vz=sinik*cosuk;

	/* Position and velocity */
	pos->x=rk*ux;
	pos->y=rk*uy;
	pos->z=rk*uz;
	vel->x=rdotk*ux+rfdotk*vx;
	vel->y=rdotk*uy+rfdotk*vy;
	vel->z=rdotk*uz+rfdotk*vz;

	/* Phase in radians */
	phase=xlt-xnode-omgadf+twopi;
    
	if (phase<0.0)
		phase+=twopi;

	phase=FMod2p(phase);
}

void Deep(int ientry, tle_t * tle, deep_arg_t * deep_arg)
{
	/* This function is used by SDP4 to add lunar and solar */
	/* perturbation effects to deep-space orbit objects.    */

	static double thgr, xnq, xqncl, omegaq, zmol, zmos, savtsn, ee2, e3,
	xi2, xl2, xl3, xl4, xgh2, xgh3, xgh4, xh2, xh3, sse, ssi, ssg, xi3,
	se2, si2, sl2, sgh2, sh2, se3, si3, sl3, sgh3, sh3, sl4, sgh4, ssl,
	ssh, d3210, d3222, d4410, d4422, d5220, d5232, d5421, d5433, del1,
	del2, del3, fasx2, fasx4, fasx6, xlamo, xfact, xni, atime, stepp,
	stepn, step2, preep, pl, sghs, xli, d2201, d2211, sghl, sh1, pinc,
	pe, shs, zsingl, zcosgl, zsinhl, zcoshl, zsinil, zcosil;

	double a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, ainv2, alfdp, aqnv,
	sgh, sini2, sinis, sinok, sh, si, sil, day, betdp, dalf, bfact, c,
	cc, cosis, cosok, cosq, ctem, f322, zx, zy, dbet, dls, eoc, eq, f2,
	f220, f221, f3, f311, f321, xnoh, f330, f441, f442, f522, f523,
	f542, f543, g200, g201, g211, pgh, ph, s1, s2, s3, s4, s5, s6, s7,
	se, sel, ses, xls, g300, g310, g322, g410, g422, g520, g521, g532,
	g533, gam, sinq, sinzf, sis, sl, sll, sls, stem, temp, temp1, x1,
	x2, x2li, x2omi, x3, x4, x5, x6, x7, x8, xl, xldot, xmao, xnddt,
	xndot, xno2, xnodce, xnoi, xomi, xpidot, z1, z11, z12, z13, z2,
	z21, z22, z23, z3, z31, z32, z33, ze, zf, zm, zn, zsing, zsinh,
	zsini, zcosg, zcosh, zcosi, delt=0, ft=0;

	switch (ientry)
	{
		case dpinit:  /* Entrance for deep space initialization */
		thgr=ThetaG(tle->epoch,deep_arg);
		eq=tle->eo;
		xnq=deep_arg->xnodp;
		aqnv=1/deep_arg->aodp;
		xqncl=tle->xincl;
		xmao=tle->xmo;
		xpidot=deep_arg->omgdot+deep_arg->xnodot;
		sinq=sin(tle->xnodeo);
		cosq=cos(tle->xnodeo);
		omegaq=tle->omegao;

		/* Initialize lunar solar terms */
		day=deep_arg->ds50+18261.5;  /* Days since 1900 Jan 0.5 */
	  
		if (day!=preep)
		{
			preep=day;
			xnodce=4.5236020-9.2422029E-4*day;
			stem=sin(xnodce);
			ctem=cos(xnodce);
			zcosil=0.91375164-0.03568096*ctem;
			zsinil=sqrt(1-zcosil*zcosil);
			zsinhl=0.089683511*stem/zsinil;
			zcoshl=sqrt(1-zsinhl*zsinhl);
			c=4.7199672+0.22997150*day;
			gam=5.8351514+0.0019443680*day;
			zmol=FMod2p(c-gam);
			zx=0.39785416*stem/zsinil;
			zy=zcoshl*ctem+0.91744867*zsinhl*stem;
			zx=AcTan(zx,zy);
			zx=gam+zx-xnodce;
			zcosgl=cos(zx);
			zsingl=sin(zx);
			zmos=6.2565837+0.017201977*day;
			zmos=FMod2p(zmos);
		    }

		  /* Do solar terms */
		  savtsn=1E20;
		  zcosg=zcosgs;
		  zsing=zsings;
		  zcosi=zcosis;
		  zsini=zsinis;
		  zcosh=cosq;
		  zsinh= sinq;
		  cc=c1ss;
		  zn=zns;
		  ze=zes;
		  xnoi=1/xnq;

		  /* Loop breaks when Solar terms are done a second */
		  /* time, after Lunar terms are initialized        */
	  
		for (;;)
		{
			/* Solar terms done again after Lunar terms are done */
			a1=zcosg*zcosh+zsing*zcosi*zsinh;
			a3=-zsing*zcosh+zcosg*zcosi*zsinh;
			a7=-zcosg*zsinh+zsing*zcosi*zcosh;
			a8=zsing*zsini;
			a9=zsing*zsinh+zcosg*zcosi*zcosh;
			a10=zcosg*zsini;
			a2=deep_arg->cosio*a7+deep_arg->sinio*a8;
			a4=deep_arg->cosio*a9+deep_arg->sinio*a10;
			a5=-deep_arg->sinio*a7+deep_arg->cosio*a8;
			a6=-deep_arg->sinio*a9+deep_arg->cosio*a10;
			x1=a1*deep_arg->cosg+a2*deep_arg->sing;
			x2=a3*deep_arg->cosg+a4*deep_arg->sing;
			x3=-a1*deep_arg->sing+a2*deep_arg->cosg;
			x4=-a3*deep_arg->sing+a4*deep_arg->cosg;
			x5=a5*deep_arg->sing;
			x6=a6*deep_arg->sing;
			x7=a5*deep_arg->cosg;
			x8=a6*deep_arg->cosg;
			z31=12*x1*x1-3*x3*x3;
			z32=24*x1*x2-6*x3*x4;
			z33=12*x2*x2-3*x4*x4;
			z1=3*(a1*a1+a2*a2)+z31*deep_arg->eosq;
			z2=6*(a1*a3+a2*a4)+z32*deep_arg->eosq;
			z3=3*(a3*a3+a4*a4)+z33*deep_arg->eosq;
			z11=-6*a1*a5+deep_arg->eosq*(-24*x1*x7-6*x3*x5);
			z12=-6*(a1*a6+a3*a5)+deep_arg->eosq*(-24*(x2*x7+x1*x8)-6*(x3*x6+x4*x5));
			z13=-6*a3*a6+deep_arg->eosq*(-24*x2*x8-6*x4*x6);
			z21=6*a2*a5+deep_arg->eosq*(24*x1*x5-6*x3*x7);
			z22=6*(a4*a5+a2*a6)+deep_arg->eosq*(24*(x2*x5+x1*x6)-6*(x4*x7+x3*x8));
			z23=6*a4*a6+deep_arg->eosq*(24*x2*x6-6*x4*x8);
			z1=z1+z1+deep_arg->betao2*z31;
			z2=z2+z2+deep_arg->betao2*z32;
			z3=z3+z3+deep_arg->betao2*z33;
			s3=cc*xnoi;
			s2=-0.5*s3/deep_arg->betao;
			s4=s3*deep_arg->betao;
			s1=-15*eq*s4;
			s5=x1*x3+x2*x4;
			s6=x2*x3+x1*x4;
			s7=x2*x4-x1*x3;
			se=s1*zn*s5;
			si=s2*zn*(z11+z13);
			sl=-zn*s3*(z1+z3-14-6*deep_arg->eosq);
			sgh=s4*zn*(z31+z33-6);
			sh=-zn*s2*(z21+z23);
		
			if (xqncl<5.2359877E-2)
				sh=0;
		    
			ee2=2*s1*s6;
			e3=2*s1*s7;
			xi2=2*s2*z12;
			xi3=2*s2*(z13-z11);
			xl2=-2*s3*z2;
			xl3=-2*s3*(z3-z1);
			xl4=-2*s3*(-21-9*deep_arg->eosq)*ze;
			xgh2=2*s4*z32;
			xgh3=2*s4*(z33-z31);
			xgh4=-18*s4*ze;
			xh2=-2*s2*z22;
			xh3=-2*s2*(z23-z21);

			if (isFlagSet(LUNAR_TERMS_DONE_FLAG))
				break;

			/* Do lunar terms */
			sse=se;
			ssi=si;
			ssl=sl;
			ssh=sh/deep_arg->sinio;
			ssg=sgh-deep_arg->cosio*ssh;
			se2=ee2;
			si2=xi2;
			sl2=xl2;
			sgh2=xgh2;
			sh2=xh2;
			se3=e3;
			si3=xi3;
			sl3=xl3;
			sgh3=xgh3;
			sh3=xh3;
			sl4=xl4;
			sgh4=xgh4;
			zcosg=zcosgl;
			zsing=zsingl;
			zcosi=zcosil;
			zsini=zsinil;
			zcosh=zcoshl*cosq+zsinhl*sinq;
			zsinh=sinq*zcoshl-cosq*zsinhl;
			zn=znl;
			cc=c1l;
			ze=zel;
			SetFlag(LUNAR_TERMS_DONE_FLAG);
		}

		sse=sse+se;
		ssi=ssi+si;
		ssl=ssl+sl;
		ssg=ssg+sgh-deep_arg->cosio/deep_arg->sinio*sh;
		ssh=ssh+sh/deep_arg->sinio;

		/* Geopotential resonance initialization for 12 hour orbits */
		ClearFlag(RESONANCE_FLAG);
		ClearFlag(SYNCHRONOUS_FLAG);

		if (!((xnq<0.0052359877) && (xnq>0.0034906585)))
		{
			if ((xnq<0.00826) || (xnq>0.00924))
			    return;
	
			if (eq<0.5)
			    return;
	
			SetFlag(RESONANCE_FLAG);
			eoc=eq*deep_arg->eosq;
			g201=-0.306-(eq-0.64)*0.440;
		
			if (eq<=0.65)
			{
				g211=3.616-13.247*eq+16.290*deep_arg->eosq;
				g310=-19.302+117.390*eq-228.419*deep_arg->eosq+156.591*eoc;
				g322=-18.9068+109.7927*eq-214.6334*deep_arg->eosq+146.5816*eoc;
				g410=-41.122+242.694*eq-471.094*deep_arg->eosq+313.953*eoc;
				g422=-146.407+841.880*eq-1629.014*deep_arg->eosq+1083.435 * eoc;
				g520=-532.114+3017.977*eq-5740*deep_arg->eosq+3708.276*eoc;
			}
		
			else
			{
				g211=-72.099+331.819*eq-508.738*deep_arg->eosq+266.724*eoc;
				g310=-346.844+1582.851*eq-2415.925*deep_arg->eosq+1246.113*eoc;
				g322=-342.585+1554.908*eq-2366.899*deep_arg->eosq+1215.972*eoc;
				g410=-1052.797+4758.686*eq-7193.992*deep_arg->eosq+3651.957*eoc;
				g422=-3581.69+16178.11*eq-24462.77*deep_arg->eosq+12422.52*eoc;
		      
				if (eq<=0.715)
					g520=1464.74-4664.75*eq+3763.64*deep_arg->eosq;
			  
				else
					g520=-5149.66+29936.92*eq-54087.36*deep_arg->eosq+31324.56*eoc;
			}

			if (eq<0.7)
			{
				g533=-919.2277+4988.61*eq-9064.77*deep_arg->eosq+5542.21*eoc;
				g521=-822.71072+4568.6173*eq-8491.4146*deep_arg->eosq+5337.524*eoc;
				g532=-853.666+4690.25*eq-8624.77*deep_arg->eosq+5341.4*eoc;
			}
		
			else
			{
				g533=-37995.78+161616.52*eq-229838.2*deep_arg->eosq+109377.94*eoc;
				g521 =-51752.104+218913.95*eq-309468.16*deep_arg->eosq+146349.42*eoc;
				g532 =-40023.88+170470.89*eq-242699.48*deep_arg->eosq+115605.82*eoc;
			}

			sini2=deep_arg->sinio*deep_arg->sinio;
			f220=0.75*(1+2*deep_arg->cosio+deep_arg->theta2);
			f221=1.5*sini2;
			f321=1.875*deep_arg->sinio*(1-2*deep_arg->cosio-3*deep_arg->theta2);
			f322=-1.875*deep_arg->sinio*(1+2*deep_arg->cosio-3*deep_arg->theta2);
			f441=35*sini2*f220;
			f442=39.3750*sini2*sini2;
			f522=9.84375*deep_arg->sinio*(sini2*(1-2*deep_arg->cosio-5*deep_arg->theta2)+0.33333333*(-2+4*deep_arg->cosio+6*deep_arg->theta2));
			f523=deep_arg->sinio*(4.92187512*sini2*(-2-4*deep_arg->cosio+10*deep_arg->theta2)+6.56250012*(1+2*deep_arg->cosio-3*deep_arg->theta2));
			f542=29.53125*deep_arg->sinio*(2-8*deep_arg->cosio+deep_arg->theta2*(-12+8*deep_arg->cosio+10*deep_arg->theta2));
			f543=29.53125*deep_arg->sinio*(-2-8*deep_arg->cosio+deep_arg->theta2*(12+8*deep_arg->cosio-10*deep_arg->theta2));
			xno2=xnq*xnq;
			ainv2=aqnv*aqnv;
			temp1=3*xno2*ainv2;
			temp=temp1*root22;
			d2201=temp*f220*g201;
			d2211=temp*f221*g211;
			temp1=temp1*aqnv;
			temp=temp1*root32;
			d3210=temp*f321*g310;
			d3222=temp*f322*g322;
			temp1=temp1*aqnv;
			temp=2*temp1*root44;
			d4410=temp*f441*g410;
			d4422=temp*f442*g422;
			temp1=temp1*aqnv;
			temp=temp1*root52;
			d5220=temp*f522*g520;
			d5232=temp*f523*g532;
			temp=2*temp1*root54;
			d5421=temp*f542*g521;
			d5433=temp*f543*g533;
			xlamo=xmao+tle->xnodeo+tle->xnodeo-thgr-thgr;
			bfact=deep_arg->xmdot+deep_arg->xnodot+deep_arg->xnodot-thdt-thdt;
			bfact=bfact+ssl+ssh+ssh;
		}
	
		else
		{
			SetFlag(RESONANCE_FLAG);
			SetFlag(SYNCHRONOUS_FLAG);
	
			/* Synchronous resonance terms initialization */
			g200=1+deep_arg->eosq*(-2.5+0.8125*deep_arg->eosq);
			g310=1+2*deep_arg->eosq;
			g300=1+deep_arg->eosq*(-6+6.60937*deep_arg->eosq);
			f220=0.75*(1+deep_arg->cosio)*(1+deep_arg->cosio);
			f311=0.9375*deep_arg->sinio*deep_arg->sinio*(1+3*deep_arg->cosio)-0.75*(1+deep_arg->cosio);
			f330=1+deep_arg->cosio;
			f330=1.875*f330*f330*f330;
			del1=3*xnq*xnq*aqnv*aqnv;
			del2=2*del1*f220*g200*q22;
			del3=3*del1*f330*g300*q33*aqnv;
			del1=del1*f311*g310*q31*aqnv;
			fasx2=0.13130908;
			fasx4=2.8843198;
			fasx6=0.37448087;
			xlamo=xmao+tle->xnodeo+tle->omegao-thgr;
			bfact=deep_arg->xmdot+xpidot-thdt;
			bfact=bfact+ssl+ssg+ssh;
		}

		xfact=bfact-xnq;

		/* Initialize integrator */
		xli=xlamo;
		xni=xnq;
		atime=0;
		stepp=720;
		stepn=-720;
		step2=259200;

		return;

		case dpsec:  /* Entrance for deep space secular effects */
		deep_arg->xll=deep_arg->xll+ssl*deep_arg->t;
		deep_arg->omgadf=deep_arg->omgadf+ssg*deep_arg->t;
		deep_arg->xnode=deep_arg->xnode+ssh*deep_arg->t;
		deep_arg->em=tle->eo+sse*deep_arg->t;
		deep_arg->xinc=tle->xincl+ssi*deep_arg->t;
	  
		if (deep_arg->xinc<0)
		{
			deep_arg->xinc=-deep_arg->xinc;
			deep_arg->xnode=deep_arg->xnode+pi;
			deep_arg->omgadf=deep_arg->omgadf-pi;
		}
	
		if (isFlagClear(RESONANCE_FLAG))
		      return;

		do
		{
			if ((atime==0) || ((deep_arg->t>=0) && (atime<0)) || ((deep_arg->t<0) && (atime>=0)))
			{
				/* Epoch restart */

				if (deep_arg->t>=0)
					delt=stepp;
				else
					delt=stepn;

				atime=0;
				xni=xnq;
				xli=xlamo;
			}

			else
			{
				if (fabs(deep_arg->t)>=fabs(atime))
				{
					if (deep_arg->t>0)
						delt=stepp;
					else
						delt=stepn;
				}
			}
	    
			do
			{
				if (fabs(deep_arg->t-atime)>=stepp)
				{
					SetFlag(DO_LOOP_FLAG);
					ClearFlag(EPOCH_RESTART_FLAG);
				}
		
				else
				{
					ft=deep_arg->t-atime;
					ClearFlag(DO_LOOP_FLAG);
				}

				if (fabs(deep_arg->t)<fabs(atime))
				{
					if (deep_arg->t>=0)
						delt=stepn;
					else
						delt=stepp;

					SetFlag(DO_LOOP_FLAG | EPOCH_RESTART_FLAG);
				}

				/* Dot terms calculated */
				if (isFlagSet(SYNCHRONOUS_FLAG))
				{
					xndot=del1*sin(xli-fasx2)+del2*sin(2*(xli-fasx4))+del3*sin(3*(xli-fasx6));
					xnddt=del1*cos(xli-fasx2)+2*del2*cos(2*(xli-fasx4))+3*del3*cos(3*(xli-fasx6));
				}
		
				else
				{
					xomi=omegaq+deep_arg->omgdot*atime;
					x2omi=xomi+xomi;
					x2li=xli+xli;
					xndot=d2201*sin(x2omi+xli-g22)+d2211*sin(xli-g22)+d3210*sin(xomi+xli-g32)+d3222*sin(-xomi+xli-g32)+d4410*sin(x2omi+x2li-g44)+d4422*sin(x2li-g44)+d5220*sin(xomi+xli-g52)+d5232*sin(-xomi+xli-g52)+d5421*sin(xomi+x2li-g54)+d5433*sin(-xomi+x2li-g54);
					xnddt=d2201*cos(x2omi+xli-g22)+d2211*cos(xli-g22)+d3210*cos(xomi+xli-g32)+d3222*cos(-xomi+xli-g32)+d5220*cos(xomi+xli-g52)+d5232*cos(-xomi+xli-g52)+2*(d4410*cos(x2omi+x2li-g44)+d4422*cos(x2li-g44)+d5421*cos(xomi+x2li-g54)+d5433*cos(-xomi+x2li-g54));
				}

				xldot=xni+xfact;
				xnddt=xnddt*xldot;

				if (isFlagSet(DO_LOOP_FLAG))
				{
					xli=xli+xldot*delt+xndot*step2;
					xni=xni+xndot*delt+xnddt*step2;
					atime=atime+delt;
				}
			} while (isFlagSet(DO_LOOP_FLAG) && isFlagClear(EPOCH_RESTART_FLAG));
		} while (isFlagSet(DO_LOOP_FLAG) && isFlagSet(EPOCH_RESTART_FLAG));

		deep_arg->xn=xni+xndot*ft+xnddt*ft*ft*0.5;
		xl=xli+xldot*ft+xndot*ft*ft*0.5;
		temp=-deep_arg->xnode+thgr+deep_arg->t*thdt;

		if (isFlagClear(SYNCHRONOUS_FLAG))
			deep_arg->xll=xl+temp+temp;
   		else
			deep_arg->xll=xl-deep_arg->omgadf+temp;

		return;

		case dpper:	 /* Entrance for lunar-solar periodics */
		sinis=sin(deep_arg->xinc);
		cosis=cos(deep_arg->xinc);

		if (fabs(savtsn-deep_arg->t)>=30)
		{
			savtsn=deep_arg->t;
			zm=zmos+zns*deep_arg->t;
			zf=zm+2*zes*sin(zm);
			sinzf=sin(zf);
			f2=0.5*sinzf*sinzf-0.25;
			f3=-0.5*sinzf*cos(zf);
			ses=se2*f2+se3*f3;
			sis=si2*f2+si3*f3;
			sls=sl2*f2+sl3*f3+sl4*sinzf;
			sghs=sgh2*f2+sgh3*f3+sgh4*sinzf;
			shs=sh2*f2+sh3*f3;
			zm=zmol+znl*deep_arg->t;
			zf=zm+2*zel*sin(zm);
			sinzf=sin(zf);
			f2=0.5*sinzf*sinzf-0.25;
			f3=-0.5*sinzf*cos(zf);
			sel=ee2*f2+e3*f3;
			sil=xi2*f2+xi3*f3;
			sll=xl2*f2+xl3*f3+xl4*sinzf;
			sghl=xgh2*f2+xgh3*f3+xgh4*sinzf;
			sh1=xh2*f2+xh3*f3;
			pe=ses+sel;
			pinc=sis+sil;
			pl=sls+sll;
		}

		pgh=sghs+sghl;
		ph=shs+sh1;
		deep_arg->xinc=deep_arg->xinc+pinc;
		deep_arg->em=deep_arg->em+pe;

		if (xqncl>=0.2)
		{
			/* Apply periodics directly */
			ph=ph/deep_arg->sinio;
			pgh=pgh-deep_arg->cosio*ph;
			deep_arg->omgadf=deep_arg->omgadf+pgh;
			deep_arg->xnode=deep_arg->xnode+ph;
			deep_arg->xll=deep_arg->xll+pl;
		}
	
		else
		{
			/* Apply periodics with Lyddane modification */
			sinok=sin(deep_arg->xnode);
			cosok=cos(deep_arg->xnode);
			alfdp=sinis*sinok;
			betdp=sinis*cosok;
			dalf=ph*cosok+pinc*cosis*sinok;
			dbet=-ph*sinok+pinc*cosis*cosok;
			alfdp=alfdp+dalf;
			betdp=betdp+dbet;
			deep_arg->xnode=FMod2p(deep_arg->xnode);
			xls=deep_arg->xll+deep_arg->omgadf+cosis*deep_arg->xnode;
			dls=pl+pgh-pinc*deep_arg->xnode*sinis;
			xls=xls+dls;
			xnoh=deep_arg->xnode;
			deep_arg->xnode=AcTan(alfdp,betdp);

			/* This is a patch to Lyddane modification */
			/* suggested by Rob Matson. */
		
			if (fabs(xnoh-deep_arg->xnode)>pi)
			{
			      if (deep_arg->xnode<xnoh)
				  deep_arg->xnode+=twopi;
			      else
				  deep_arg->xnode-=twopi;
			}

			deep_arg->xll=deep_arg->xll+pl;
			deep_arg->omgadf=xls-deep_arg->xll-cos(deep_arg->xinc)*deep_arg->xnode;
		}
		return;
	}
}

void SDP4(double tsince, tle_t * tle, vector_t * pos, vector_t * vel)
{
	/* This function is used to calculate the position and velocity */
	/* of deep-space (period > 225 minutes) satellites. tsince is   */
	/* time since epoch in minutes, tle is a pointer to a tle_t     */
	/* structure with Keplerian orbital elements and pos and vel    */
	/* are vector_t structures returning ECI satellite position and */
	/* velocity. Use Convert_Sat_State() to convert to km and km/s. */

	int i;

	static double x3thm1, c1, x1mth2, c4, xnodcf, t2cof, xlcof,
	aycof, x7thm1;

	double a, axn, ayn, aynl, beta, betal, capu, cos2u, cosepw, cosik,
	cosnok, cosu, cosuk, ecose, elsq, epw, esine, pl, theta4, rdot,
	rdotk, rfdot, rfdotk, rk, sin2u, sinepw, sinik, sinnok, sinu,
	sinuk, tempe, templ, tsq, u, uk, ux, uy, uz, vx, vy, vz, xinck, xl,
	xlt, xmam, xmdf, xmx, xmy, xnoddf, xnodek, xll, a1, a3ovk2, ao, c2,
	coef, coef1, x1m5th, xhdot1, del1, r, delo, eeta, eta, etasq,
	perigee, psisq, tsi, qoms24, s4, pinvsq, temp, tempa, temp1,
	temp2, temp3, temp4, temp5, temp6, bx, by, bz, cx, cy, cz;

	static deep_arg_t deep_arg;

	/* Initialization */

	if (isFlagClear(SDP4_INITIALIZED_FLAG))
	{
		SetFlag(SDP4_INITIALIZED_FLAG);

		/* Recover original mean motion (xnodp) and   */
		/* semimajor axis (aodp) from input elements. */
	  
		a1=pow(xke/tle->xno,tothrd);
		deep_arg.cosio=cos(tle->xincl);
		deep_arg.theta2=deep_arg.cosio*deep_arg.cosio;
		x3thm1=3*deep_arg.theta2-1;
		deep_arg.eosq=tle->eo*tle->eo;
		deep_arg.betao2=1-deep_arg.eosq;
		deep_arg.betao=sqrt(deep_arg.betao2);
		del1=1.5*ck2*x3thm1/(a1*a1*deep_arg.betao*deep_arg.betao2);
		ao=a1*(1-del1*(0.5*tothrd+del1*(1+134/81*del1)));
		delo=1.5*ck2*x3thm1/(ao*ao*deep_arg.betao*deep_arg.betao2);
		deep_arg.xnodp=tle->xno/(1+delo);
		deep_arg.aodp=ao/(1-delo);

		/* For perigee below 156 km, the values */
		/* of s and qoms2t are altered.         */
	  
		s4=s;
		qoms24=qoms2t;
		perigee=(deep_arg.aodp*(1-tle->eo)-ae)*xkmper;
	  
		if (perigee<156.0)
		{
			if (perigee<=98.0)
				s4=20.0;
			else
				s4=perigee-78.0;
	
			qoms24=pow((120-s4)*ae/xkmper,4);
			s4=s4/xkmper+ae;
		}

		pinvsq=1/(deep_arg.aodp*deep_arg.aodp*deep_arg.betao2*deep_arg.betao2);
		deep_arg.sing=sin(tle->omegao);
		deep_arg.cosg=cos(tle->omegao);
		tsi=1/(deep_arg.aodp-s4);
		eta=deep_arg.aodp*tle->eo*tsi;
		etasq=eta*eta;
		eeta=tle->eo*eta;
		psisq=fabs(1-etasq);
		coef=qoms24*pow(tsi,4);
		coef1=coef/pow(psisq,3.5);
		c2=coef1*deep_arg.xnodp*(deep_arg.aodp*(1+1.5*etasq+eeta*(4+etasq))+0.75*ck2*tsi/psisq*x3thm1*(8+3*etasq*(8+etasq)));
		c1=tle->bstar*c2;
		deep_arg.sinio=sin(tle->xincl);
		a3ovk2=-xj3/ck2*pow(ae,3);
		x1mth2=1-deep_arg.theta2;
		c4=2*deep_arg.xnodp*coef1*deep_arg.aodp*deep_arg.betao2*(eta*(2+0.5*etasq)+tle->eo*(0.5+2*etasq)-2*ck2*tsi/(deep_arg.aodp*psisq)*(-3*x3thm1*(1-2*eeta+etasq*(1.5-0.5*eeta))+0.75*x1mth2*(2*etasq-eeta*(1+etasq))*cos(2*tle->omegao)));
		theta4=deep_arg.theta2*deep_arg.theta2;
		temp1=3*ck2*pinvsq*deep_arg.xnodp;
		temp2=temp1*ck2*pinvsq;
		temp3=1.25*ck4*pinvsq*pinvsq*deep_arg.xnodp;
		deep_arg.xmdot=deep_arg.xnodp+0.5*temp1*deep_arg.betao*x3thm1+0.0625*temp2*deep_arg.betao*(13-78*deep_arg.theta2+137*theta4);
		x1m5th=1-5*deep_arg.theta2;
		deep_arg.omgdot=-0.5*temp1*x1m5th+0.0625*temp2*(7-114*deep_arg.theta2+395*theta4)+temp3*(3-36*deep_arg.theta2+49*theta4);
		xhdot1=-temp1*deep_arg.cosio;
		deep_arg.xnodot=xhdot1+(0.5*temp2*(4-19*deep_arg.theta2)+2*temp3*(3-7*deep_arg.theta2))*deep_arg.cosio;
		xnodcf=3.5*deep_arg.betao2*xhdot1*c1;
		t2cof=1.5*c1;
		xlcof=0.125*a3ovk2*deep_arg.sinio*(3+5*deep_arg.cosio)/(1+deep_arg.cosio);
		aycof=0.25*a3ovk2*deep_arg.sinio;
		x7thm1=7*deep_arg.theta2-1;

		/* initialize Deep() */

		Deep(dpinit,tle,&deep_arg);
	}

	/* Update for secular gravity and atmospheric drag */
	xmdf=tle->xmo+deep_arg.xmdot*tsince;
	deep_arg.omgadf=tle->omegao+deep_arg.omgdot*tsince;
	xnoddf=tle->xnodeo+deep_arg.xnodot*tsince;
	tsq=tsince*tsince;
	deep_arg.xnode=xnoddf+xnodcf*tsq;
	tempa=1-c1*tsince;
	tempe=tle->bstar*c4*tsince;
	templ=t2cof*tsq;
	deep_arg.xn=deep_arg.xnodp;

	/* Update for deep-space secular effects */
	deep_arg.xll=xmdf;
	deep_arg.t=tsince;

	Deep(dpsec, tle, &deep_arg);

	xmdf=deep_arg.xll;
	a=pow(xke/deep_arg.xn,tothrd)*tempa*tempa;
	deep_arg.em=deep_arg.em-tempe;
	xmam=xmdf+deep_arg.xnodp*templ;

	/* Update for deep-space periodic effects */
	deep_arg.xll=xmam;

	Deep(dpper,tle,&deep_arg);

	xmam=deep_arg.xll;
	xl=xmam+deep_arg.omgadf+deep_arg.xnode;
	beta=sqrt(1-deep_arg.em*deep_arg.em);
	deep_arg.xn=xke/pow(a,1.5);

	/* Long period periodics */
	axn=deep_arg.em*cos(deep_arg.omgadf);
	temp=1/(a*beta*beta);
	xll=temp*xlcof*axn;
	aynl=temp*aycof;
	xlt=xl+xll;
	ayn=deep_arg.em*sin(deep_arg.omgadf)+aynl;

	/* Solve Kepler's Equation */
	capu=FMod2p(xlt-deep_arg.xnode);
	temp2=capu;
	i=0;

	do
	{
		sinepw=sin(temp2);
		cosepw=cos(temp2);
		temp3=axn*sinepw;
		temp4=ayn*cosepw;
		temp5=axn*cosepw;
		temp6=ayn*sinepw;
		epw=(capu-temp4+temp3-temp2)/(1-temp5-temp6)+temp2;
	  
		if (fabs(epw-temp2)<=e6a)
			break;

		temp2=epw;
	  
	} while (i++<10);

	/* Short period preliminary quantities */
	ecose=temp5+temp6;
	esine=temp3-temp4;
	elsq=axn*axn+ayn*ayn;
	temp=1-elsq;
	pl=a*temp;
	r=a*(1-ecose);
	temp1=1/r;
	rdot=xke*sqrt(a)*esine*temp1;
	rfdot=xke*sqrt(pl)*temp1;
	temp2=a*temp1;
	betal=sqrt(temp);
	temp3=1/(1+betal);
	cosu=temp2*(cosepw-axn+ayn*esine*temp3);
	sinu=temp2*(sinepw-ayn-axn*esine*temp3);
	u=AcTan(sinu,cosu);
	sin2u=2*sinu*cosu;
	cos2u=2*cosu*cosu-1;
	temp=1/pl;
	temp1=ck2*temp;
	temp2=temp1*temp;

	/* Update for short periodics */
	rk=r*(1-1.5*temp2*betal*x3thm1)+0.5*temp1*x1mth2*cos2u;
	uk=u-0.25*temp2*x7thm1*sin2u;
	xnodek=deep_arg.xnode+1.5*temp2*deep_arg.cosio*sin2u;
	xinck=deep_arg.xinc+1.5*temp2*deep_arg.cosio*deep_arg.sinio*cos2u;
	rdotk=rdot-deep_arg.xn*temp1*x1mth2*sin2u;
	rfdotk=rfdot+deep_arg.xn*temp1*(x1mth2*cos2u+1.5*x3thm1);

	/* Orientation vectors */
	sinuk=sin(uk);
	cosuk=cos(uk);
	sinik=sin(xinck);
	cosik=cos(xinck);
	sinnok=sin(xnodek);
	cosnok=cos(xnodek);
	xmx=-sinnok*cosik;
	xmy=cosnok*cosik;
	ux=xmx*sinuk+cosnok*cosuk;
	uy=xmy*sinuk+sinnok*cosuk;
	uz=sinik*sinuk;
	vx=xmx*cosuk-cosnok*sinuk;
	vy=xmy*cosuk-sinnok*sinuk;
	vz=sinik*cosuk;

	/* Position and velocity */
	pos->x=rk*ux;
	pos->y=rk*uy;
	pos->z=rk*uz;
	vel->x=rdotk*ux+rfdotk*vx;
	vel->y=rdotk*uy+rfdotk*vy;
	vel->z=rdotk*uz+rfdotk*vz;

	/* Calculations for squint angle begin here... */

	if (calc_squint)
	{
		bx=cos(alat)*cos(alon+deep_arg.omgadf);
		by=cos(alat)*sin(alon+deep_arg.omgadf);
		bz=sin(alat);
		cx=bx;
		cy=by*cos(xinck)-bz*sin(xinck);
		cz=by*sin(xinck)+bz*cos(xinck);
		ax=cx*cos(xnodek)-cy*sin(xnodek);
		ay=cx*sin(xnodek)+cy*cos(xnodek);
		az=cz;
	}
	
	/* Phase in radians */
	phase=xlt-deep_arg.xnode-deep_arg.omgadf+twopi;
    
	if (phase<0.0)
		phase+=twopi;

	phase=FMod2p(phase);
}

void Calculate_User_PosVel(double time, geodetic_t *geodetic, vector_t *obs_pos, vector_t *obs_vel)
{
	/* Calculate_User_PosVel() passes the user's geodetic position
	   and the time of interest and returns the ECI position and
	   velocity of the observer.  The velocity calculation assumes
	   the geodetic position is stationary relative to the earth's
	   surface. */

	/* Reference:  The 1992 Astronomical Almanac, page K11. */

	double c, sq, achcp;

	geodetic->theta=FMod2p(ThetaG_JD(time)+geodetic->lon); /* LMST */
	c=1/sqrt(1+f*(f-2)*Sqr(sin(geodetic->lat)));
	sq=Sqr(1-f)*c;
	achcp=(xkmper*c+geodetic->alt)*cos(geodetic->lat);
	obs_pos->x=achcp*cos(geodetic->theta); /* kilometers */
	obs_pos->y=achcp*sin(geodetic->theta);
	obs_pos->z=(xkmper*sq+geodetic->alt)*sin(geodetic->lat);
	obs_vel->x=-mfactor*obs_pos->y; /* kilometers/second */
	obs_vel->y=mfactor*obs_pos->x;
	obs_vel->z=0;
	Magnitude(obs_pos);
	Magnitude(obs_vel);
}

void Calculate_LatLonAlt(double time, vector_t *pos,  geodetic_t *geodetic)
{
	/* Procedure Calculate_LatLonAlt will calculate the geodetic  */
	/* position of an object given its ECI position pos and time. */
	/* It is intended to be used to determine the ground track of */
	/* a satellite.  The calculations  assume the earth to be an  */
	/* oblate spheroid as defined in WGS '72.                     */

	/* Reference:  The 1992 Astronomical Almanac, page K12. */

	double r, e2, phi, c;

	geodetic->theta=AcTan(pos->y,pos->x); /* radians */
	geodetic->lon=FMod2p(geodetic->theta-ThetaG_JD(time)); /* radians */
	r=sqrt(Sqr(pos->x)+Sqr(pos->y));
	e2=f*(2-f);
	geodetic->lat=AcTan(pos->z,r); /* radians */

	do
	{
		phi=geodetic->lat;
		c=1/sqrt(1-e2*Sqr(sin(phi)));
		geodetic->lat=AcTan(pos->z+xkmper*c*e2*sin(phi),r);

	} while (fabs(geodetic->lat-phi)>=1E-10);

	geodetic->alt=r/cos(geodetic->lat)-xkmper*c; /* kilometers */

	if (geodetic->lat>pio2)
		geodetic->lat-=twopi;
}

void Calculate_Obs(double time, vector_t *pos, vector_t *vel, geodetic_t *geodetic, vector_t *obs_set)
{
	/* The procedures Calculate_Obs and Calculate_RADec calculate         */
	/* the *topocentric* coordinates of the object with ECI position,     */
	/* {pos}, and velocity, {vel}, from location {geodetic} at {time}.    */
	/* The {obs_set} returned for Calculate_Obs consists of azimuth,      */
	/* elevation, range, and range rate (in that order) with units of     */
	/* radians, radians, kilometers, and kilometers/second, respectively. */
	/* The WGS '72 geoid is used and the effect of atmospheric refraction */
	/* (under standard temperature and pressure) is incorporated into the */
	/* elevation calculation; the effect of atmospheric refraction on     */
	/* range and range rate has not yet been quantified.                  */

	/* The {obs_set} for Calculate_RADec consists of right ascension and  */
	/* declination (in that order) in radians.  Again, calculations are   */
	/* based on *topocentric* position using the WGS '72 geoid and        */
	/* incorporating atmospheric refraction.                              */

	double sin_lat, cos_lat, sin_theta, cos_theta, el, azim, top_s, top_e, top_z;

	vector_t obs_pos, obs_vel, range, rgvel;

	Calculate_User_PosVel(time, geodetic, &obs_pos, &obs_vel);

	range.x=pos->x-obs_pos.x;
	range.y=pos->y-obs_pos.y;
	range.z=pos->z-obs_pos.z;

	/* Save these values globally for calculating squint angles later... */

	rx=range.x;
	ry=range.y;
	rz=range.z;

	rgvel.x=vel->x-obs_vel.x;
	rgvel.y=vel->y-obs_vel.y;
	rgvel.z=vel->z-obs_vel.z;

	Magnitude(&range);

	sin_lat=sin(geodetic->lat);
	cos_lat=cos(geodetic->lat);
	sin_theta=sin(geodetic->theta);
	cos_theta=cos(geodetic->theta);
	top_s=sin_lat*cos_theta*range.x+sin_lat*sin_theta*range.y-cos_lat*range.z;
	top_e=-sin_theta*range.x+cos_theta*range.y;
	top_z=cos_lat*cos_theta*range.x+cos_lat*sin_theta*range.y+sin_lat*range.z;
	azim=atan(-top_e/top_s); /* Azimuth */

	if (top_s>0.0) 
		azim=azim+pi;

	if (azim<0.0)
		azim=azim+twopi;

	el=ArcSin(top_z/range.w);
	obs_set->x=azim;	/* Azimuth (radians)   */
	obs_set->y=el;		/* Elevation (radians) */
	obs_set->z=range.w;	/* Range (kilometers)  */

	/* Range Rate (kilometers/second) */

	obs_set->w=Dot(&range,&rgvel)/range.w;

	/* Corrections for atmospheric refraction */
	/* Reference:  Astronomical Algorithms by Jean Meeus, pp. 101-104    */
	/* Correction is meaningless when apparent elevation is below horizon */

	/*** The following adjustment for
		 atmospheric refraction is bypassed ***/

	/* obs_set->y=obs_set->y+Radians((1.02/tan(Radians(Degrees(el)+10.3/(Degrees(el)+5.11))))/60); */

	obs_set->y=el;

	/**** End bypass ****/

	if (obs_set->y>=0.0)
		SetFlag(VISIBLE_FLAG);
	else
	{
		obs_set->y=el;  /* Reset to true elevation */
		ClearFlag(VISIBLE_FLAG);
	}
}

void Calculate_RADec(double time, vector_t *pos, vector_t *vel, geodetic_t *geodetic, vector_t *obs_set)
{
	/* Reference:  Methods of Orbit Determination by  */
	/*             Pedro Ramon Escobal, pp. 401-402   */

	double	phi, theta, sin_theta, cos_theta, sin_phi, cos_phi, az, el,
		Lxh, Lyh, Lzh, Sx, Ex, Zx, Sy, Ey, Zy, Sz, Ez, Zz, Lx, Ly,
		Lz, cos_delta, sin_alpha, cos_alpha;

	Calculate_Obs(time,pos,vel,geodetic,obs_set);

	az=obs_set->x;
	el=obs_set->y;
	phi=geodetic->lat;
	theta=FMod2p(ThetaG_JD(time)+geodetic->lon);
	sin_theta=sin(theta);
	cos_theta=cos(theta);
	sin_phi=sin(phi);
	cos_phi=cos(phi);
	Lxh=-cos(az)*cos(el);
	Lyh=sin(az)*cos(el);
	Lzh=sin(el);
	Sx=sin_phi*cos_theta;
	Ex=-sin_theta;
	Zx=cos_theta*cos_phi;
	Sy=sin_phi*sin_theta;
	Ey=cos_theta;
	Zy=sin_theta*cos_phi;
	Sz=-cos_phi;
	Ez=0.0;
	Zz=sin_phi;
	Lx=Sx*Lxh+Ex*Lyh+Zx*Lzh;
	Ly=Sy*Lxh+Ey*Lyh+Zy*Lzh;
	Lz=Sz*Lxh+Ez*Lyh+Zz*Lzh;
	obs_set->y=ArcSin(Lz);  /* Declination (radians) */
	cos_delta=sqrt(1.0-Sqr(Lz));
	sin_alpha=Ly/cos_delta;
	cos_alpha=Lx/cos_delta;
	obs_set->x=AcTan(sin_alpha,cos_alpha); /* Right Ascension (radians) */
	obs_set->x=FMod2p(obs_set->x);
}

/* .... SGP4/SDP4 functions end .... */

void bailout(string)
char *string;
{
	/* This function quits ncurses, resets and "beeps"
	   the terminal, and displays an error message (string)
	   when we need to bail out of the program in a hurry. */

	beep();	
	curs_set(1);
	bkgdset(COLOR_PAIR(1));
	clear();
	refresh();
	endwin();
	fprintf(stderr,"*** predict: %s!\n",string);
}

void TrackDataOut(antfd, elevation, azimuth)
int antfd;
double elevation, azimuth;
{
	/* This function sends Azimuth and Elevation data
	   to an antenna tracker connected to the serial port */

	int n, port;
	char message[30]="\n";

	port=antfd;

	sprintf(message, "AZ%3.1f EL%3.1f \x0D\x0A", azimuth,elevation);
	n=write(port,message,strlen(message));

	if (n<0)
	{
		bailout("Error Writing To Antenna Port");
		exit(-1);
	}
}

int passivesock(char *service, char *protocol, int qlen)
{
	/* This function opens the socket port */

	struct servent *pse;
	struct protoent *ppe;
	struct sockaddr_in sin;
	int sd, type;
	
	memset((char *)&sin, 0, sizeof(struct sockaddr_in));
	sin.sin_family=AF_INET;
	sin.sin_addr.s_addr=INADDR_ANY;
	
	if ((pse=getservbyname(service,protocol)))
		sin.sin_port=htons(ntohs((unsigned short)pse->s_port)+portbase);

	else if ((sin.sin_port=htons((unsigned short)atoi(service)))==0)
	{
		bailout("Can't get service");
		exit(-1);
	}
	
	if ((ppe=getprotobyname(protocol))==0)
	{
		bailout("Can't get protocol");
		exit(-1);
	}
	
	if (strcmp(protocol,"udp")==0)
		type=SOCK_DGRAM;
	else
		type=SOCK_STREAM;
	
	sd=socket(PF_INET,type, ppe->p_proto);

	if (sd<0)
	{
		bailout("Can't open socket");
		exit(-1);
	}
	
	if (bind(sd,(struct sockaddr *)&sin,sizeof(sin))<0)
	{
		bailout("Can't bind");
		exit(-1);
	}
	
	if ((type=SOCK_STREAM && listen(s,qlen))<0)
	{
		bailout("Listen fail");
		exit(-1);
	}

	return sd;
}

void socket_server(predict_name)
char *predict_name;
{
	/* This is the socket server code */

	int i, j, n, sock;
	socklen_t alen;
	struct sockaddr_in fsin;
	char buf[80], buff[1000], satname[50], tempname[30], ok;
	time_t t;
	long nxtevt;
	FILE *fd=NULL;

	/* Open a socket port at "predict" or netport if defined */

	if (netport[0]==0)
		strncpy(netport,"predict",7);

	sock=passivesock(netport,"udp",10);
 	alen=sizeof(fsin);
	
	/* This is the main loop for monitoring the socket
	   port and sending back replies to clients */

	while (1)
	{
		/* Get datagram from socket port */
		
		if ((n=recvfrom(sock,buf,sizeof(buf),0,(struct sockaddr *)&fsin,&alen)) < 0)
			exit (-1);

		buf[n]=0;
		ok=0;

		/* Parse the command in the datagram */
		if ((strncmp("GET_SAT",buf,7)==0) && (strncmp("GET_SAT_POS",buf,11)!=0))
		{
			/* Parse "buf" for satellite name */
			for (i=0; buf[i]!=32 && buf[i]!=0 && i<39; i++);

			for (j=++i; buf[j]!='\n' && buf[j]!=0 && (j-i)<25; j++)
				satname[j-i]=buf[j];

			satname[j-i]=0;

			/* Do a simple search for the matching satellite name */

			for (i=0; i<24; i++)
			{
				if ((strncmp(satname,sat[i].name,25)==0) || (atol(satname)==sat[i].catnum))
				{
					nxtevt=(long)rint(86400.0*(nextevent[i]+3651.0));

					/* Build text buffer with satellite data */
					sprintf(buff,"%s\n%-7.2f\n%+-6.2f\n%-7.2f\n%+-6.2f\n%ld\n%-7.2f\n%-7.2f\n%-7.2f\n%-7.2f\n%ld\n%c\n%-7.2f\n%-7.2f\n%-7.2f\n",sat[i].name,long_array[i],lat_array[i],az_array[i],el_array[i],nxtevt,footprint_array[i],range_array[i],altitude_array[i],velocity_array[i],orbitnum_array[i],visibility_array[i],phase_array[i],eclipse_depth_array[i],squint_array[i]);

					/* Send buffer back to the client that sent the request */
					sendto(sock,buff,strlen(buff),0,(struct sockaddr*)&fsin,sizeof(fsin));
					ok=1;
					break;
				}
			}
		}

		if (strncmp("GET_TLE",buf,7)==0)
		{
			/* Parse "buf" for satellite name */
			for (i=0; buf[i]!=32 && buf[i]!=0 && i<39; i++);

			for (j=++i; buf[j]!='\n' && buf[j]!=0 && (j-i)<25; j++)
				satname[j-i]=buf[j];

			satname[j-i]=0;

			/* Do a simple search for the matching satellite name */

			for (i=0; i<24; i++)
			{
				if ((strncmp(satname,sat[i].name,25)==0) || (atol(satname)==sat[i].catnum))
				{
					/* Build text buffer with satellite data */

					sprintf(buff,"%s\n%s\n%s\n",sat[i].name,sat[i].line1, sat[i].line2);
					/* Send buffer back to the client that sent the request */
					sendto(sock,buff,strlen(buff),0,(struct sockaddr*)&fsin,sizeof(fsin));
					ok=1;
					break;
				}
			}
		}

		if (strncmp("GET_DOPPLER",buf,11)==0)
		{
			/* Parse "buf" for satellite name */
			for (i=0; buf[i]!=32 && buf[i]!=0 && i<39; i++);

			for (j=++i; buf[j]!='\n' && buf[j]!=0 && (j-i)<25; j++)
				satname[j-i]=buf[j];

			satname[j-i]=0;

			/* Do a simple search for the matching satellite name */

			for (i=0; i<24; i++)
			{
				if ((strncmp(satname,sat[i].name,25)==0) || (atol(satname)==sat[i].catnum))
				{
					/* Get Normalized (100 MHz)
					   Doppler shift for sat[i] */

					sprintf(buff,"%f\n",doppler[i]);

					/* Send buffer back to client who sent request */
					sendto(sock,buff,strlen(buff),0,(struct sockaddr*)&fsin,sizeof(fsin));
					ok=1;
					break;
				}
			}
		}

		if (strncmp("GET_LIST",buf,8)==0)
		{
			buff[0]=0;

			for (i=0; i<24; i++)
			{
				if (sat[i].name[0]!=0)
					strcat(buff,sat[i].name);

				strcat(buff,"\n");
			}

			sendto(sock,buff,strlen(buff),0,(struct sockaddr *)&fsin,sizeof(fsin));
			ok=1;
		}

		if (strncmp("RELOAD_TLE",buf,10)==0)
		{
			buff[0]=0;
			sendto(sock,buff,strlen(buff),0,(struct sockaddr *)&fsin,sizeof(fsin));
			reload_tle=1;
			ok=1;
		}

		if (strncmp("GET_SUN",buf,7)==0)
		{
			buff[0]=0;
			sprintf(buff,"%-7.2f\n%+-6.2f\n%-7.2f\n%-7.2f\n%-7.2f\n",sun_azi, sun_ele, sun_lat, sun_lon, sun_ra);
			sendto(sock,buff,strlen(buff),0,(struct sockaddr *)&fsin,sizeof(fsin));
			ok=1;
		}

		if (strncmp("GET_MOON",buf,8)==0)
		{
			buff[0]=0;
			sprintf(buff,"%-7.2f\n%+-6.2f\n%-7.2f\n%-7.2f\n%-7.2f\n",moon_az, moon_el, moon_dec, moon_gha, moon_ra);
			sendto(sock,buff,strlen(buff),0,(struct sockaddr *)&fsin,sizeof(fsin));
			ok=1;
		}

		if (strncmp("GET_MODE",buf,8)==0)
		{
			sendto(sock,tracking_mode,strlen(tracking_mode),0,(struct sockaddr *)&fsin,sizeof(fsin));
			ok=1;
		}

		if (strncmp("GET_VERSION",buf,11)==0)
		{
			buff[0]=0;
			sprintf(buff,"%s\n",version);
			sendto(sock,buff,strlen(buff),0,(struct sockaddr *)&fsin,sizeof(fsin));
			ok=1;
		}

		if (strncmp("GET_QTH",buf,7)==0)
		{
			buff[0]=0;
			sprintf(buff,"%s\n%g\n%g\n%d\n",qth.callsign, qth.stnlat, qth.stnlong, qth.stnalt);
			sendto(sock,buff,strlen(buff),0,(struct sockaddr *)&fsin,sizeof(fsin));
			ok=1;
		}

		if (strncmp("GET_TIME$",buf,9)==0)
		{
			buff[0]=0;
			t=time(NULL);
			sprintf(buff,"%s",asctime(gmtime(&t)));

			if (buff[8]==32)
				buff[8]='0';

			sendto(sock,buff,strlen(buff),0,(struct sockaddr *)&fsin,sizeof(fsin));
			buf[0]=0;
			ok=1;
		}

		if (strncmp("GET_TIME",buf,8)==0)
		{
			buff[0]=0;
			t=time(NULL);
			sprintf(buff,"%lu\n",(unsigned long)t);
			sendto(sock,buff,strlen(buff),0,(struct sockaddr *)&fsin,sizeof(fsin));
			ok=1;
		}

		if (strncmp("GET_SAT_POS",buf,11)==0)
		{
			/* Parse "buf" for satellite name and arguments */
			for (i=0; buf[i]!=32 && buf[i]!=0 && i<39; i++);

			for (j=++i; buf[j]!='\n' && buf[j]!=0 && (j-i)<49; j++)
				satname[j-i]=buf[j];

			satname[j-i]=0;

			/* Send request to predict with output
			   directed to a temporary file under /tmp */

			strcpy(tempname,"/tmp/XXXXXX\0");
			i=mkstemp(tempname);

			sprintf(buff,"%s -f %s -t %s -q %s -o %s\n",predict_name,satname,tlefile,qthfile,tempname);
			system(buff);

			/* Append an EOF marker (CNTRL-Z) to the end of file */

			fd=fopen(tempname,"a");
			fprintf(fd,"%c\n",26);  /* Control-Z */
			fclose(fd);

			buff[0]=0;

			/* Send the file to the client */

			fd=fopen(tempname,"rb");

			fgets(buff,80,fd);

			do
			{
				sendto(sock,buff,strlen(buff),0,(struct sockaddr *)&fsin,sizeof(fsin));
				fgets(buff,80,fd);
				/* usleep(2);  if needed (for flow-control) */

			} while (feof(fd)==0);

			fclose(fd);
			unlink(tempname);
			close(i);
			ok=1;
		}

		if (strncmp("PREDICT",buf,7)==0)
		{
			/* Parse "buf" for satellite name and arguments */
			for (i=0; buf[i]!=32 && buf[i]!=0 && i<39; i++);

			for (j=++i; buf[j]!='\n' && buf[j]!=0 && (j-i)<49; j++)
				satname[j-i]=buf[j];

			satname[j-i]=0;

			/* Send request to predict with output
			   directed to a temporary file under /tmp */

			strcpy(tempname,"/tmp/XXXXXX\0");
			i=mkstemp(tempname);

			sprintf(buff,"%s -p %s -t %s -q %s -o %s\n",predict_name, satname,tlefile,qthfile,tempname);
			system(buff);

			/* Append an EOF marker (CNTRL-Z) to the end of file */

			fd=fopen(tempname,"a");
			fprintf(fd,"%c\n",26);  /* Control-Z */
			fclose(fd);

			buff[0]=0;

			/* Send the file to the client */

			fd=fopen(tempname,"rb");

			fgets(buff,80,fd);

			do
			{
				sendto(sock,buff,strlen(buff),0,(struct sockaddr *)&fsin,sizeof(fsin));
				fgets(buff,80,fd);
				/* usleep(2);  if needed (for flow-control) */

			} while (feof(fd)==0);

			fclose(fd);
			unlink(tempname);
			close(i);
			ok=1;
		}

		if (ok==0)
			sendto(sock,"Huh?\n",5,0,(struct sockaddr *)&fsin,sizeof(fsin));
	} 	
}

void Banner()
{
	curs_set(0);
	bkgdset(COLOR_PAIR(3));
	clear();
	refresh();

	attrset(COLOR_PAIR(6)|A_REVERSE|A_BOLD);
	mvprintw(3,18,"                                           ");
	mvprintw(4,18,"         --== PREDICT  v%s ==--         ",version);
	mvprintw(5,18,"   Released by John A. Magliacane, KD2BD   ");
	mvprintw(6,18,"                  May 2018                 ");
	mvprintw(7,18,"                                           ");
}

void AnyKey()
{
	mvprintw(23,24,"<< Press Any Key To Continue >>");
	refresh();
	getch();
}

double FixAngle(x)
double x;
{
	/* This function reduces angles greater than
	   two pi by subtracting two pi from the angle */

	while (x>twopi)
		x-=twopi;

	return x;
}

double PrimeAngle(x)
double x;
{
	/* This function is used in the FindMoon() function. */

	x=x-360.0*floor(x/360.0);
	return x;
}

char *SubString(string,start,end)
char *string, start, end;
{
	/* This function returns a substring based on the starting
	   and ending positions provided.  It is used heavily in
	   the AutoUpdate function when parsing 2-line element data. */

	unsigned x, y;

	if (end>=start)
	{
		for (x=start, y=0; x<=end && string[x]!=0; x++)
			if (string[x]!=' ')
			{
				temp[y]=string[x];
				y++;
			}

		temp[y]=0;
		return temp;
	}
	else
		return NULL;
}

void CopyString(source, destination, start, end)
char *source, *destination, start, end;
{
	/* This function copies elements of the string "source"
	   bounded by "start" and "end" into the string "destination". */

	unsigned j, k=0;

	for (j=start; j<=end; j++)
		if (source[k]!=0)
		{
			destination[j]=source[k];
			k++;
		}
}

char *Abbreviate(string,n)
char *string;
int n;
{
	/* This function returns an abbreviated substring of the original,
	   including a '~' character if a non-blank character is chopped
	   out of the generated substring.  n is the length of the desired
	   substring.  It is used for abbreviating satellite names. */

	strncpy(temp,string,79);

	if (temp[n]!=0 && temp[n]!=32)
	{
		temp[n-2]='~';
		temp[n-1]=temp[strlen(temp)-1];
	}

	temp[n]=0;

	return temp;
}

char KepCheck(line1,line2)
char *line1, *line2;
{
	/* This function scans line 1 and line 2 of a NASA 2-Line element
	   set and returns a 1 if the element set appears to be valid or
	   a 0 if it does not.  If the data survives this torture test,
	   it's a pretty safe bet we're looking at a valid 2-line
	   element set and not just some random text that might pass
	   as orbital data based on a simple checksum calculation alone. */

	int x;
	unsigned sum1, sum2;

	/* Compute checksum for each line */

	for (x=0, sum1=0, sum2=0; x<=67; sum1+=val[(int)line1[x]], sum2+=val[(int)line2[x]], x++);

	/* Perform a "torture test" on the data */

	x=(val[(int)line1[68]]^(sum1%10)) | (val[(int)line2[68]]^(sum2%10)) |
	  (line1[0]^'1')  | (line1[1]^' ')  | (line1[7]^'U')  |
	  (line1[8]^' ')  | (line1[17]^' ') | (line1[23]^'.') |
	  (line1[32]^' ') | (line1[34]^'.') | (line1[43]^' ') |
	  (line1[52]^' ') | (line1[61]^' ') | (line1[62]^'0') |
	  (line1[63]^' ') | (line2[0]^'2')  | (line2[1]^' ')  |
	  (line2[7]^' ')  | (line2[11]^'.') | (line2[16]^' ') |
	  (line2[20]^'.') | (line2[25]^' ') | (line2[33]^' ') |
	  (line2[37]^'.') | (line2[42]^' ') | (line2[46]^'.') |
	  (line2[51]^' ') | (line2[54]^'.') | (line1[2]^line2[2]) |
	  (line1[3]^line2[3]) | (line1[4]^line2[4]) |
	  (line1[5]^line2[5]) | (line1[6]^line2[6]) |
	  (isdigit(line1[68]) ? 0 : 1) | (isdigit(line2[68]) ? 0 : 1) |
	  (isdigit(line1[18]) ? 0 : 1) | (isdigit(line1[19]) ? 0 : 1) |
	  (isdigit(line2[31]) ? 0 : 1) | (isdigit(line2[32]) ? 0 : 1);

	return (x ? 0 : 1);
}

void InternalUpdate(x)
int x;
{
	/* Updates data in TLE structure based on
	   line1 and line2 stored in structure. */

	double tempnum;

	strncpy(sat[x].designator,SubString(sat[x].line1,9,16),8);
	sat[x].designator[9]=0;
	sat[x].catnum=atol(SubString(sat[x].line1,2,6));
	sat[x].year=atoi(SubString(sat[x].line1,18,19));
	sat[x].refepoch=atof(SubString(sat[x].line1,20,31));
	tempnum=1.0e-5*atof(SubString(sat[x].line1,44,49));
	sat[x].nddot6=tempnum/pow(10.0,(sat[x].line1[51]-'0'));
	tempnum=1.0e-5*atof(SubString(sat[x].line1,53,58));
	sat[x].bstar=tempnum/pow(10.0,(sat[x].line1[60]-'0'));
	sat[x].setnum=atol(SubString(sat[x].line1,64,67));
	sat[x].incl=atof(SubString(sat[x].line2,8,15));
	sat[x].raan=atof(SubString(sat[x].line2,17,24));
	sat[x].eccn=1.0e-07*atof(SubString(sat[x].line2,26,32));
	sat[x].argper=atof(SubString(sat[x].line2,34,41));
	sat[x].meanan=atof(SubString(sat[x].line2,43,50));
	sat[x].meanmo=atof(SubString(sat[x].line2,52,62));
	sat[x].drag=atof(SubString(sat[x].line1,33,42));
	sat[x].orbitnum=atof(SubString(sat[x].line2,63,67));
}

char *noradEvalue(value)
double value;
{
	/* Converts numeric values to E notation used in NORAD TLEs */

	char string[15];

	sprintf(string,"%11.4e",value*10.0);

	output[0]=string[0];
	output[1]=string[1];
	output[2]=string[3];
	output[3]=string[4];
	output[4]=string[5];
	output[5]=string[6];
	output[6]='-';
	output[7]=string[10];
	output[8]=0;

	return output;
}

void Data2TLE(x)
int x;
{
	/* This function converts orbital data held in the numeric
	   portion of the sat tle structure to ASCII TLE format,
	   and places the result in ASCII portion of the structure. */
 
	int i;
	char string[15], line1[70], line2[70];
	unsigned sum;

	/* Fill lines with blanks */

	for (i=0; i<70; line1[i]=32, line2[i]=32, i++);

	line1[69]=0;
	line2[69]=0;

	/* Insert static characters */

	line1[0]='1';
	line1[7]='U'; /* Unclassified */
	line2[0]='2';

	line1[62]='0'; /* For publically released TLEs */

	/* Insert orbital data */

	sprintf(string,"%05ld",sat[x].catnum);
	CopyString(string,line1,2,6);
	CopyString(string,line2,2,6);

	CopyString(sat[x].designator,line1,9,16);

	sprintf(string,"%02d",sat[x].year);
	CopyString(string,line1,18,19);

	sprintf(string,"%12.8f",sat[x].refepoch);
	CopyString(string,line1,20,32);

	sprintf(string,"%.9f",fabs(sat[x].drag));

	CopyString(string,line1,33,42);

	if (sat[x].drag<0.0)
		line1[33]='-';
	else
		line1[33]=32;

	CopyString(noradEvalue(sat[x].nddot6),line1,44,51);
	CopyString(noradEvalue(sat[x].bstar),line1,53,60);

	sprintf(string,"%4lu",sat[x].setnum);
	CopyString(string,line1,64,67);

	sprintf(string,"%9.4f",sat[x].incl);
	CopyString(string,line2,7,15);
				
	sprintf(string,"%9.4f",sat[x].raan);
	CopyString(string,line2,16,24);

	sprintf(string,"%13.12f",sat[x].eccn);
	
	/* Erase eccentricity's decimal point */

	for (i=2; i<=9; string[i-2]=string[i], i++);

	CopyString(string,line2,26,32);

	sprintf(string,"%9.4f",sat[x].argper);
	CopyString(string,line2,33,41);

	sprintf(string,"%9.5f",sat[x].meanan);
	CopyString(string,line2,43,50);

	sprintf(string,"%12.9f",sat[x].meanmo);
	CopyString(string,line2,52,62);

	sprintf(string,"%5lu",sat[x].orbitnum);
	CopyString(string,line2,63,67);

	/* Compute and insert checksum for line 1 and line 2 */

	for (i=0, sum=0; i<=67; sum+=val[(int)line1[i]], i++);

	line1[68]=(sum%10)+'0';

	for (i=0, sum=0; i<=67; sum+=val[(int)line2[i]], i++);

	line2[68]=(sum%10)+'0';

	line1[69]=0;
	line2[69]=0;

	strcpy(sat[x].line1,line1);
	strcpy(sat[x].line2,line2);
}

double ReadBearing(input)
char *input;
{
	/* This function takes numeric input in the form of a character
	   string, and returns an equivalent bearing in degrees as a
	   decimal number (double).  The input may either be expressed
	   in decimal format (74.2467) or degree, minute, second
	   format (74 14 48).  This function also safely handles
	   extra spaces found either leading, trailing, or
	   embedded within the numbers expressed in the
	   input string.  Decimal seconds are permitted. */
 
	char string[20];
	double bearing=0.0, seconds;
	int a, b, length, degrees, minutes;

	/* Copy "input" to "string", and ignore any extra
	   spaces that might be present in the process. */

	string[0]=0;
	length=strlen(input);

	for (a=0, b=0; a<length && a<18; a++)
	{
		if ((input[a]!=32 && input[a]!='\n') || (input[a]==32 && input[a+1]!=32 && b!=0))
		{
			string[b]=input[a];
			b++;
		}	 
	}

	string[b]=0;

	/* Count number of spaces in the clean string. */

	length=strlen(string);

	for (a=0, b=0; a<length; a++)
		if (string[a]==32)
			b++;

	if (b==0)  /* Decimal Format (74.2467) */
		sscanf(string,"%lf",&bearing);

	if (b==2)  /* Degree, Minute, Second Format (74 14 48) */
	{
		sscanf(string,"%d %d %lf",&degrees, &minutes, &seconds);

		if (degrees<0.0)
		{
			minutes=-minutes;
			seconds=-seconds;
		}

		bearing=(double)degrees+((double)minutes/60)+(seconds/3600);
	}

	/* Bizarre results return a 0.0 */

	if (bearing>360.0 || bearing<-360.0)
		bearing=0.0;

	return bearing;
}

char ReadDataFiles()
{
	/* This function reads "predict.qth", "predict.tle",
	   and (optionally) "predict.db" files into memory.
  	   Return values are as follows:

	   0: Neither the qth nor the tle files were loaded
	   1: Only the qth file was loaded
	   2: Only the tle file was loaded
	   3: The qth and tle files were loaded successfully */

	FILE *fd;
	long catnum;
	unsigned char dayofweek;
	int x=0, y, entry=0, max_entries=10, transponders=0;
	char flag=0, match, name[80], line1[80], line2[80];

	fd=fopen(qthfile,"r");
	
	if (fd!=NULL)
	{
		fgets(qth.callsign,16,fd);
		qth.callsign[strlen(qth.callsign)-1]=0;
		fscanf(fd,"%lf", &qth.stnlat);
		fscanf(fd,"%lf", &qth.stnlong);
		fscanf(fd,"%d", &qth.stnalt);
		fclose(fd);

		obs_geodetic.lat=qth.stnlat*deg2rad;
		obs_geodetic.lon=-qth.stnlong*deg2rad;
		obs_geodetic.alt=((double)qth.stnalt)/1000.0;
		obs_geodetic.theta=0.0;

		flag=1;
	}

	fd=fopen(tlefile,"r");

	if (fd!=NULL)
	{
		while (x<24 && feof(fd)==0)
		{
			/* Initialize variables */

			name[0]=0;
			line1[0]=0;
			line2[0]=0;

			/* Read element set */

			fgets(name,75,fd);
			fgets(line1,75,fd);
			fgets(line2,75,fd);

			if (KepCheck(line1,line2) && (feof(fd)==0))
			{
				/* We found a valid TLE! */

				/* Some TLE sources left justify the sat
				   name in a 24-byte field that is padded
				   with blanks.  The following lines cut
				   out the blanks as well as the line feed
				   character read by the fgets() function. */
 
				y=strlen(name);

				while (name[y]==32 || name[y]==0 || name[y]==10 || name[y]==13 || y==0)
				{
					name[y]=0;
					y--;
				}
				
				/* Copy TLE data into the sat data structure */

				strncpy(sat[x].name,name,24);
				strncpy(sat[x].line1,line1,69);
				strncpy(sat[x].line2,line2,69);

				/* Update individual parameters */

				InternalUpdate(x);

				x++;
			}
		}

		fclose(fd);
		flag+=2;
		resave=0;

		/* Load satellite database file */

		fd=fopen(dbfile,"r");

		if (fd!=NULL)
		{
			database=1;

			fgets(line1,40,fd);

			while (strncmp(line1,"end",3)!=0 && line1[0]!='\n' && feof(fd)==0)
			{
				/* The first line is the satellite
				   name which is ignored here. */

				fgets(line1,40,fd);
				sscanf(line1,"%ld",&catnum);

				/* Search for match */

				for (y=0, match=0; y<24 && match==0; y++)
				{
					if (catnum==sat[y].catnum)
						match=1;
				}

				if (match)
				{
					transponders=0;
					entry=0;
					y--;
				}

				fgets(line1,40,fd);

				if (match)
				{
					if (strncmp(line1,"No",2)!=0)
					{
						sscanf(line1,"%lf, %lf",&sat_db[y].alat, &sat_db[y].alon);
						sat_db[y].squintflag=1;
					}

					else
						sat_db[y].squintflag=0;
				}

				fgets(line1,80,fd);

				while (strncmp(line1,"end",3)!=0 && line1[0]!='\n' && feof(fd)==0)
				{
					if (entry<max_entries)
					{
						if (match)
						{
							if (strncmp(line1,"No",2)!=0)
							{
								line1[strlen(line1)-1]=0;
								strcpy(sat_db[y].transponder_name[entry],line1);
							}
							else
								sat_db[y].transponder_name[entry][0]=0;
						}

						fgets(line1,40,fd);

						if (match)
							sscanf(line1,"%lf, %lf", &sat_db[y].uplink_start[entry], &sat_db[y].uplink_end[entry]);

						fgets(line1,40,fd);

						if (match)
							sscanf(line1,"%lf, %lf", &sat_db[y].downlink_start[entry], &sat_db[y].downlink_end[entry]);

						fgets(line1,40,fd);

						if (match)
						{
							if (strncmp(line1,"No",2)!=0)
							{
								dayofweek=(unsigned char)atoi(line1);
								sat_db[y].dayofweek[entry]=dayofweek;
							}
							else
								sat_db[y].dayofweek[entry]=0;
						}

						fgets(line1,40,fd);

						if (match)
						{
							if (strncmp(line1,"No",2)!=0)
								sscanf(line1,"%d, %d",&sat_db[y].phase_start[entry], &sat_db[y].phase_end[entry]);
							else
							{
								sat_db[y].phase_start[entry]=0;
								sat_db[y].phase_end[entry]=0;
							}

							if (sat_db[y].uplink_start[entry]!=0.0 || sat_db[y].downlink_start[entry]!=0.0)
								transponders++;

							entry++;
						}
					}
					fgets(line1,80,fd);
				}
				fgets(line1,80,fd);

				if (match)
					sat_db[y].transponders=transponders;

				entry=0;
				transponders=0;
			}

			fclose(fd);
		}
	}

	return flag;
}

char CopyFile(source, destination)
char *source;
char *destination;
{
	/* This function copies file "source" to file "destination"
	   in 64k chunks.  The permissions on the destination file
	   are set to rw-r--r--  (0644).  A 0 is returned if no
	   errors are encountered.  A 1 indicates a problem writing
	   to the destination file.  A 2 indicates a problem reading
	   the source file.  */

	int x, sd, dd;
	char error=0, buffer[65536];

	sd=open(source,O_RDONLY);

	if (sd!=-1)
	{
		dd=open(destination,O_WRONLY | O_CREAT| O_TRUNC, 0644);

		if (dd!=-1)
		{
			x=read(sd,&buffer,65536);

			while (x)
			{
				write(dd,&buffer,x);
				x=read(sd,&buffer,65536);
			}

			close(dd);
		}
		else
			error=1;

		close(sd);
	}
	else
		error+=2;

	return error;
}

void SaveQTH()
{
	/* This function saves QTH data to the QTH data file. */

	FILE *fd;	

	fd=fopen(qthfile,"w");

	fprintf(fd,"%s\n",qth.callsign);
	fprintf(fd," %g\n",qth.stnlat);
	fprintf(fd," %g\n",qth.stnlong);
	fprintf(fd," %d\n",qth.stnalt);

	fclose(fd);
}

void SaveTLE()
{
	int x;
	FILE *fd;

 	/* Save orbital data to tlefile */

	fd=fopen(tlefile,"w");

	for (x=0; x<24; x++)
	{
		/* Convert numeric orbital data to ASCII TLE format */

		Data2TLE(x);

		/* Write name, line1, line2 to predict.tle */

		fprintf(fd,"%s\n", sat[x].name);  
		fprintf(fd,"%s\n", sat[x].line1);
		fprintf(fd,"%s\n", sat[x].line2);
	}

	fclose(fd);
}

int AutoUpdate(string)
char *string;
{
	/* This function updates PREDICT's orbital datafile from a NASA
	   2-line element file either through a menu (interactive mode)
	   or via the command line.  string==filename of 2-line element
	   set if this function is invoked via the command line. */

	char line1[80], line2[80], str0[80], str1[80], str2[80],
	     filename[50], saveflag=0, interactive=0, savecount=0;

	float database_epoch=0.0, tle_epoch=0.0, database_year, tle_year;
	int i, success=0, kepcount=0;
	FILE *fd;

	do
	{
		if (string[0]==0)
		{
			interactive=1;
			curs_set(1);
			bkgdset(COLOR_PAIR(3));
			refresh();
			clear();
			echo();

			for (i=5; i<8; i+=2)
				mvprintw(i,19,"------------------------------------------");

			mvprintw(6,19,"* Keplerian Database Auto Update Utility *");
			bkgdset(COLOR_PAIR(2));
			mvprintw(19,18,"Enter NASA Two-Line Element Source File Name");
			mvprintw(13,18,"-=> ");
			refresh();
			wgetnstr(stdscr,filename,49);
			clear();
			curs_set(0);
		}
		else
			strcpy(filename,string);

		/* Prevent "." and ".." from being used as a
		   filename, otherwise strange things happen. */

		if (strlen(filename)==0 || strncmp(filename,".",1)==0 || strncmp(filename,"..",2)==0)
			return 0;

		fd=fopen(filename,"r");

		if (interactive && fd==NULL)
		{
			bkgdset(COLOR_PAIR(5));
			clear();
			move(12,0);

			for (i=47; i>strlen(filename); i-=2)
				printw(" ");

			printw("*** ERROR: File \"%s\" not found! ***\n",filename);
			beep();
			attrset(COLOR_PAIR(7)|A_BOLD);
			AnyKey();
		}

		if (fd!=NULL)
		{
			success=1;

			fgets(str0,75,fd);
			fgets(str1,75,fd);
			fgets(str2,75,fd);
		
			do
			{
				if (KepCheck(str1,str2))
				{
					/* We found a valid TLE!
					   Copy strings str1 and
					   str2 into line1 and line2 */

					strncpy(line1,str1,75);
					strncpy(line2,str2,75);
					kepcount++;

					/* Scan for object number in datafile to see
					   if this is something we're interested in */

					for (i=0; (i<24 && sat[i].catnum!=atol(SubString(line1,2,6))); i++);

					if (i!=24)
					{
						/* We found it!  Check to see if it's more
						   recent than the data we already have. */

						if (sat[i].year<57)
							database_year=365.25*(100.0+(float)sat[i].year);
						else
							database_year=365.25*(float)sat[i].year;

						database_epoch=(float)sat[i].refepoch+database_year;

						tle_year=(float)atof(SubString(line1,18,19));

						if (tle_year<57.0)
							tle_year+=100.0;

						tle_epoch=(float)atof(SubString(line1,20,31))+(tle_year*365.25);

						/* Update only if TLE epoch >= epoch in data file
						   so we don't overwrite current data with older
						   data. */

						if (tle_epoch>=database_epoch)
						{
							if (saveflag==0)
							{
								if (interactive)
								{
									clear();
									bkgdset(COLOR_PAIR(2));
									mvprintw(3,35,"Updating.....");
									refresh();
									move(7,0);
								}
								saveflag=1;
							}

							if (interactive)
							{
								bkgdset(COLOR_PAIR(3));
								printw("     %-15s",sat[i].name);
							}

							savecount++;

							/* Copy TLE data into the sat data structure */

							strncpy(sat[i].line1,line1,69);
							strncpy(sat[i].line2,line2,69);
							InternalUpdate(i);
						}
					}

					 fgets(str0,75,fd);     	
					 fgets(str1,75,fd);
					 fgets(str2,75,fd);
				}

				else
				{
					strcpy(str0,str1);   
					strcpy(str1,str2);   
					fgets(str2,75,fd);
				}
			
			} while (feof(fd)==0);

			fclose(fd);

			if (interactive)
			{
				bkgdset(COLOR_PAIR(2));

				if (kepcount==1)
					mvprintw(18,21,"  Only 1 NASA Two Line Element was found.");
				else
					mvprintw(18,21,"%3u NASA Two Line Elements were read.",kepcount);

				if (saveflag)
				{
					if (savecount==1)
						mvprintw(19,21,"  Only 1 satellite was updated.");
					else
					{
						if (savecount==24)
							mvprintw(19,21,"  All satellites were updated!");
						else
							mvprintw(19,21,"%3u out of 24 satellites were updated.",savecount);
					}
				}

				refresh();
			}
		}

		if (interactive)
		{
			noecho();

			if (strlen(filename) && fd!=NULL) 
			{
				attrset(COLOR_PAIR(4)|A_BOLD);
				AnyKey();
			}
		}

		if (saveflag)
			SaveTLE();
	}
	while (success==0 && interactive);

	return (saveflag ? 0 : -1);
}

int Select()
{
	/* This function displays the names of satellites contained
	   within the program's database and returns an index that
	   corresponds to the satellite selected by the user.  An
	   ESC or CR returns a -1. */

	int x, y, z, key=0;

	clear();

	bkgdset(COLOR_PAIR(2)|A_BOLD);
	printw("\n\n\t\t\t      Select a Satellite:\n\n");

	attrset(COLOR_PAIR(3)|A_BOLD);

	for (x=0, y=8, z=16; y<16; ++x, ++y, ++z)
	{
		printw("\n\t[%c]: %-15s", x+'A', Abbreviate(sat[x].name,15));
		printw("\t[%c]: %-15s", y+'A', Abbreviate(sat[y].name,15));
		printw("\t[%c]: %-15s\n", z+'A', Abbreviate(sat[z].name,15));
	}

	attrset(COLOR_PAIR(4)|A_BOLD);

	printw("\n\n\t\t<< Enter Selection  -  Press [ESC] To Exit >>");
	refresh();

	do
	{
		key=toupper(getch());

		if (key==27 || key=='\n')
			return -1;

	} while (key<'A' || key>'X');

	return(key-'A');
}

long DayNum(m,d,y)
int  m, d, y;
{
	/* This function calculates the day number from m/d/y. */

	long dn;
	double mm, yy;

	if (m<3)
	{ 
		y--; 
		m+=12; 
	}

	if (y<57)
		y+=100;

	yy=(double)y;
	mm=(double)m;
	dn=(long)(floor(365.25*(yy-80.0))-floor(19.0+yy/100.0)+floor(4.75+yy/400.0)-16.0);
	dn+=d+30*m+(long)floor(0.6*mm-0.3);
	return dn;
}

double CurrentDaynum()
{
	/* Read the system clock and return the number
	   of days since 31Dec79 00:00:00 UTC (daynum 0) */

	/* int x; */
	struct timeval tptr;
	double usecs, seconds;

	/* x=gettimeofday(&tptr,NULL); */
	(void)gettimeofday(&tptr,NULL);

	usecs=0.000001*(double)tptr.tv_usec;
	seconds=usecs+(double)tptr.tv_sec;

	return ((seconds/86400.0)-3651.0);
}

char *Daynum2String(daynum)
double daynum;
{
	/* This function takes the given epoch as a fractional number of
	   days since 31Dec79 00:00:00 UTC and returns the corresponding
	   date as a string of the form "Tue 12Oct99 17:22:37". */

	char timestr[26];
	time_t t;
	int x;

	/* Convert daynum to Unix time (seconds since 01-Jan-70) */
	t=(time_t)(86400.0*(daynum+3651.0));

	sprintf(timestr,"%s",asctime(gmtime(&t)));

	if (timestr[8]==' ')
		timestr[8]='0';

	for (x=0; x<=3; output[x]=timestr[x], x++);

	output[4]=timestr[8];
	output[5]=timestr[9];
	output[6]=timestr[4];
	output[7]=timestr[5];
	output[8]=timestr[6];
	output[9]=timestr[22];
	output[10]=timestr[23];
	output[11]=' ';

	for (x=12; x<=19; output[x]=timestr[x-1], x++);

	output[20]=0;
	return output;
}

double GetStartTime(mode)
char mode;
{
	/* This function prompts the user for the time and date
	   the user wishes to begin prediction calculations,
	   and returns the corresponding fractional day number.
	   31Dec79 00:00:00 returns 0.  Default is NOW. */

	int	x, hr, min, sec ,mm=0, dd=0, yy; 
	char	good, mon[5], line[30], string[30], bozo_count=0,
		*month[12]= {"Jan", "Feb", "Mar", "Apr", "May",
		"Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

	do
	{
		bkgdset(COLOR_PAIR(2)|A_BOLD);
		clear();

		if (mode=='m')
			printw("\n\n\n\t     Starting UTC Date and Time for Predictions of the Moon\n\n");

		if (mode=='o')
			printw("\n\n\n\t     Starting UTC Date and Time for Predictions of the Sun\n\n");

		if (mode!='m' && mode!='o')
			printw("\n\n\n\t     Starting UTC Date and Time for Predictions of %-15s\n\n",sat[indx].name);

		bozo_count++;

		strcpy(string,Daynum2String(CurrentDaynum()));

		for (x=4; x<24; x++)
			string[x-4]=string[x];

		attrset(COLOR_PAIR(4)|A_BOLD);
		printw("\t\t    Format: %s -or- ",string);
		string[7]=0;
		printw("%s",string);
		attrset(COLOR_PAIR(2)|A_BOLD);
		mvprintw(21,30,"Default is `NOW'");
		attrset(COLOR_PAIR(3)|A_BOLD);
		mvprintw(13,1,"Enter Start Date & Time >> ");
		curs_set(1);
		refresh();
		echo();
		string[0]=0;
		wgetnstr(stdscr,string,29);
		curs_set(0);
		noecho();
		       
		if (strlen(string)!=0)
			strcpy(line,string);
		else
			/* Select `NOW' */
			return(CurrentDaynum());

		if (strlen(line)==7)
		{
			line[7]=' ';
			line[8]='0';
			line[9]='0';
			line[10]=':';
			line[11]='0';
			line[12]='0';
			line[13]=':';
			line[14]='0';
			line[15]='0';
			line[16]=0;
		}
			
		/* Check Day */
		good = (isdigit(line[0]) && isdigit(line[1])) ? 1 : 0;

		/* Month */
		good = (good && isalpha(line[2]) && isalpha(line[3]) && isalpha(line[4])) ? 1 : 0;

		/* Year */
		good = (good && isdigit(line[5]) && isdigit(line[6]) && (line[7]==' ')) ? 1 : 0;

		/* Hour */
		good = (good && isdigit(line[8]) && isdigit(line[9]) && (line[10]==':')) ? 1 : 0;

		/* Minute */
		good = (good && isdigit(line[11]) && isdigit(line[12]) && (line[13]==':')) ? 1 : 0;

		/* Seconds */
		good = (good && isdigit(line[14]) && isdigit(line[15])) ? 1 : 0;

	     	if (good)
		{ 
			/* Decode Day */
			dd=10*(line[0]-'0')+line[1]-'0';

			/* Decode Month Number */
			line[2]=toupper(line[2]);
			line[3]=tolower(line[3]);
			line[4]=tolower(line[4]);

			mon[0]=line[2];
			mon[1]=line[3];
			mon[2]=line[4];
			mon[3]=0;

			for (mm=0; (mm<12 && strcmp(mon,month[mm])!=0); mm++);

			mm++;
	
			good=(mm>12) ? 0 : 1;
		}

		if (good==0)
			beep();

	} while (good==0 && bozo_count<6);

	if (good==0)
	{
		/* If the user can't enter the starting date/time
		   correctly after several attempts, then the user
		   is a "bozo" and obviously can't follow directions. */

		bailout("Too Many Errors");
		exit(-1);
	}

	/* Decode Year */
	yy=10*(line[5]-'0')+line[6]-'0';

	/* Decode Time */
	for (x=8; x<16; x++)
		string[x-8]=line[x];

	string[8]=0;

	hr=10*(line[8]-'0')+line[9]-'0';
	min=10*(line[11]-'0')+line[12]-'0';
	sec=10*(line[14]-'0')+line[15]-'0';
	
	return ((double)DayNum(mm,dd,yy)+((hr/24.0)+(min/1440.0)+(sec/86400.0)));
}

void FindMoon(daynum)
double daynum;
{
	/* This function determines the position of the moon, including
	   the azimuth and elevation headings, relative to the latitude
	   and longitude of the tracking station.  This code was derived
	   from a Javascript implementation of the Meeus method for
	   determining the exact position of the Moon found at:
	   http://www.geocities.com/s_perona/ingles/poslun.htm. */

	double	jd, ss, t, t1, t2, t3, d, ff, l1, m, m1, ex, om, l,
		b, w1, w2, bt, p, lm, h, ra, dec, z, ob, n, el, az,
		teg, th, mm, dv;

	jd=daynum+2444238.5;

	t=(jd-2415020.0)/36525.0;
	t2=t*t;
	t3=t2*t;
	l1=270.434164+481267.8831*t-0.001133*t2+0.0000019*t3;
	m=358.475833+35999.0498*t-0.00015*t2-0.0000033*t3;
	m1=296.104608+477198.8491*t+0.009192*t2+0.0000144*t3;
	d=350.737486+445267.1142*t-0.001436*t2+0.0000019*t3;
	ff=11.250889+483202.0251*t-0.003211*t2-0.0000003*t3;
	om=259.183275-1934.142*t+0.002078*t2+0.0000022*t3;
	om=om*deg2rad;
	
	/* Additive terms */

	l1=l1+0.000233*sin((51.2+20.2*t)*deg2rad);
	ss=0.003964*sin((346.56+132.87*t-0.0091731*t2)*deg2rad);
	l1=l1+ss+0.001964*sin(om);
	m=m-0.001778*sin((51.2+20.2*t)*deg2rad);
	m1=m1+0.000817*sin((51.2+20.2*t)*deg2rad);
	m1=m1+ss+0.002541*sin(om);
	d=d+0.002011*sin((51.2+20.2*t)*deg2rad);
	d=d+ss+0.001964*sin(om);
	ff=ff+ss-0.024691*sin(om);
	ff=ff-0.004328*sin(om+(275.05-2.3*t)*deg2rad);
	ex=1.0-0.002495*t-0.00000752*t2;
	om=om*deg2rad;

	l1=PrimeAngle(l1);
	m=PrimeAngle(m);
	m1=PrimeAngle(m1);
	d=PrimeAngle(d);
	ff=PrimeAngle(ff);
	om=PrimeAngle(om);

	m=m*deg2rad;
	m1=m1*deg2rad;
	d=d*deg2rad;
	ff=ff*deg2rad;

	/* Ecliptic Longitude */

	l=l1+6.28875*sin(m1)+1.274018*sin(2.0*d-m1)+0.658309*sin(2.0*d);
	l=l+0.213616*sin(2.0*m1)-ex*0.185596*sin(m)-0.114336*sin(2.0*ff);
	l=l+0.058793*sin(2.0*d-2.0*m1)+ex*0.057212*sin(2.0*d-m-m1)+0.05332*sin(2.0*d+m1);
	l=l+ex*0.045874*sin(2.0*d-m)+ex*0.041024*sin(m1-m)-0.034718*sin(d);
	l=l-ex*0.030465*sin(m+m1)+0.015326*sin(2.0*d-2.0*ff)-0.012528*sin(2.0*ff+m1);
	
	l=l-0.01098*sin(2.0*ff-m1)+0.010674*sin(4.0*d-m1)+0.010034*sin(3.0*m1);
	l=l+0.008548*sin(4.0*d-2.0*m1)-ex*0.00791*sin(m-m1+2.0*d)-ex*0.006783*sin(2.0*d+m);
	
	l=l+0.005162*sin(m1-d)+ex*0.005*sin(m+d)+ex*0.004049*sin(m1-m+2.0*d);
	l=l+0.003996*sin(2.0*m1+2.0*d)+0.003862*sin(4.0*d)+0.003665*sin(2.0*d-3.0*m1);

	l=l+ex*0.002695*sin(2.0*m1-m)+0.002602*sin(m1-2.0*ff-2.0*d)+ex*0.002396*sin(2.0*d-m-2.0*m1);

	l=l-0.002349*sin(m1+d)+ex*ex*0.002249*sin(2.0*d-2.0*m)-ex*0.002125*sin(2.0*m1+m);

	l=l-ex*ex*0.002079*sin(2.0*m)+ex*ex*0.002059*sin(2.0*d-m1-2.0*m)-0.001773*sin(m1+2.0*d-2.0*ff);

	l=l+ex*0.00122*sin(4.0*d-m-m1)-0.00111*sin(2.0*m1+2.0*ff)+0.000892*sin(m1-3.0*d);

	l=l-ex*0.000811*sin(m+m1+2.0*d)+ex*0.000761*sin(4.0*d-m-2.0*m1)+ex*ex*.000717*sin(m1-2.0*m);

	l=l+ex*ex*0.000704*sin(m1-2.0*m-2.0*d)+ex*0.000693*sin(m-2.0*m1+2.0*d)+ex*0.000598*sin(2.0*d-m-2.0*ff)+0.00055*sin(m1+4.0*d);

	l=l+0.000538*sin(4.0*m1)+ex*0.000521*sin(4.0*d-m)+0.000486*sin(2.0*m1-d);

	l=l-0.001595*sin(2.0*ff+2.0*d);

	/* Ecliptic latitude */

	b=5.128189*sin(ff)+0.280606*sin(m1+ff)+0.277693*sin(m1-ff)+0.173238*sin(2.0*d-ff);
	b=b+0.055413*sin(2.0*d+ff-m1)+0.046272*sin(2.0*d-ff-m1)+0.032573*sin(2.0*d+ff);

	b=b+0.017198*sin(2.0*m1+ff)+9.266999e-03*sin(2.0*d+m1-ff)+0.008823*sin(2.0*m1-ff);
	b=b+ex*0.008247*sin(2.0*d-m-ff)+0.004323*sin(2.0*d-ff-2.0*m1)+0.0042*sin(2.0*d+ff+m1);

	b=b+ex*0.003372*sin(ff-m-2.0*d)+ex*0.002472*sin(2.0*d+ff-m-m1)+ex*0.002222*sin(2.0*d+ff-m);

	b=b+0.002072*sin(2.0*d-ff-m-m1)+ex*0.001877*sin(ff-m+m1)+0.001828*sin(4.0*d-ff-m1);

	b=b-ex*0.001803*sin(ff+m)-0.00175*sin(3.0*ff)+ex*0.00157*sin(m1-m-ff)-0.001487*sin(ff+d)-ex*0.001481*sin(ff+m+m1)+ex*0.001417*sin(ff-m-m1)+ex*0.00135*sin(ff-m)+0.00133*sin(ff-d);

	b=b+0.001106*sin(ff+3.0*m1)+0.00102*sin(4.0*d-ff)+0.000833*sin(ff+4.0*d-m1);

	b=b+0.000781*sin(m1-3.0*ff)+0.00067*sin(ff+4.0*d-2.0*m1)+0.000606*sin(2.0*d-3.0*ff);

	b=b+0.000597*sin(2.0*d+2.0*m1-ff)+ex*0.000492*sin(2.0*d+m1-m-ff)+0.00045*sin(2.0*m1-ff-2.0*d);

	b=b+0.000439*sin(3.0*m1-ff)+0.000423*sin(ff+2.0*d+2.0*m1)+0.000422*sin(2.0*d-ff-3.0*m1);

	b=b-ex*0.000367*sin(m+ff+2.0*d-m1)-ex*0.000353*sin(m+ff+2.0*d)+0.000331*sin(ff+4.0*d);

	b=b+ex*0.000317*sin(2.0*d+ff-m+m1)+ex*ex*0.000306*sin(2.0*d-2.0*m-ff)-0.000283*sin(m1+3.0*ff);

	w1=0.0004664*cos(om*deg2rad);
	w2=0.0000754*cos((om+275.05-2.3*t)*deg2rad);
	bt=b*(1.0-w1-w2);

	/* Parallax calculations */

	p=0.950724+0.051818*cos(m1)+0.009531*cos(2.0*d-m1)+0.007843*cos(2.0*d)+0.002824*cos(2.0*m1)+0.000857*cos(2.0*d+m1)+ex*0.000533*cos(2.0*d-m)+ex*0.000401*cos(2.0*d-m-m1);

	p=p+0.000173*cos(3.0*m1)+0.000167*cos(4.0*d-m1)-ex*0.000111*cos(m)+0.000103*cos(4.0*d-2.0*m1)-0.000084*cos(2.0*m1-2.0*d)-ex*0.000083*cos(2.0*d+m)+0.000079*cos(2.0*d+2.0*m1);

	p=p+0.000072*cos(4.0*d)+ex*0.000064*cos(2.0*d-m+m1)-ex*0.000063*cos(2.0*d+m-m1);

	p=p+ex*0.000041*cos(m+d)+ex*0.000035*cos(2.0*m1-m)-0.000033*cos(3.0*m1-2.0*d);

	p=p-0.00003*cos(m1+d)-0.000029*cos(2.0*ff-2.0*d)-ex*0.000029*cos(2.0*m1+m);

	p=p+ex*ex*0.000026*cos(2.0*d-2.0*m)-0.000023*cos(2.0*ff-2.0*d+m1)+ex*0.000019*cos(4.0*d-m-m1);

	b=bt*deg2rad;
	lm=l*deg2rad;
	moon_dx=3.0/(pi*p);

	/* Semi-diameter calculation */
	/* sem=10800.0*asin(0.272488*p*deg2rad)/pi; */

	/* Convert ecliptic coordinates to equatorial coordinates */

	z=(jd-2415020.5)/365.2422;
	ob=23.452294-(0.46845*z+5.9e-07*z*z)/3600.0;
	ob=ob*deg2rad;
	dec=asin(sin(b)*cos(ob)+cos(b)*sin(ob)*sin(lm));
	ra=acos(cos(b)*cos(lm)/cos(dec));
	
	if (lm>pi)
		ra=twopi-ra;

	/* ra = right ascension */
	/* dec = declination */

	n=qth.stnlat*deg2rad;    /* North latitude of tracking station */

	/* Find siderial time in radians */

	t=(jd-2451545.0)/36525.0;
	teg=280.46061837+360.98564736629*(jd-2451545.0)+(0.000387933*t-t*t/38710000.0)*t;

	while (teg>360.0)
		teg-=360.0;

	th=FixAngle((teg-qth.stnlong)*deg2rad);
	h=th-ra;

	az=atan2(sin(h),cos(h)*sin(n)-tan(dec)*cos(n))+pi;
	el=asin(sin(n)*sin(dec)+cos(n)*cos(dec)*cos(h));

	moon_az=az/deg2rad;
	moon_el=el/deg2rad;

	/* Radial velocity approximation.  This code was derived
	   from "Amateur Radio Software", by John Morris, GM4ANB,
	   published by the RSGB in 1985. */

	mm=FixAngle(1.319238+daynum*0.228027135);  /* mean moon position */
	t2=0.10976;
	t1=mm+t2*sin(mm);
	dv=0.01255*moon_dx*moon_dx*sin(t1)*(1.0+t2*cos(mm));
	dv=dv*4449.0;
	t1=6378.0;
	t2=384401.0;
	t3=t1*t2*(cos(dec)*cos(n)*sin(h));
	t3=t3/sqrt(t2*t2-t2*t1*sin(el));
	moon_dv=dv+t3*0.0753125;

	moon_dec=dec/deg2rad;
	moon_ra=ra/deg2rad;
	moon_gha=teg-moon_ra;

	if (moon_gha<0.0)
		moon_gha+=360.0;
}

void FindSun(daynum)
double daynum;
{
	/* This function finds the position of the Sun */

	/* Zero vector for initializations */
	vector_t zero_vector={0,0,0,0};

	/* Solar ECI position vector  */
	vector_t solar_vector=zero_vector;

	/* Solar observed azi and ele vector  */
	vector_t solar_set=zero_vector;

	/* Solar right ascension and declination vector */
	vector_t solar_rad=zero_vector;

	/* Solar lat, long, alt vector */
	geodetic_t solar_latlonalt;

	jul_utc=daynum+2444238.5;

	Calculate_Solar_Position(jul_utc, &solar_vector);
	Calculate_Obs(jul_utc, &solar_vector, &zero_vector, &obs_geodetic, &solar_set);
	sun_azi=Degrees(solar_set.x); 
	sun_ele=Degrees(solar_set.y);
	sun_range=1.0+((solar_set.z-AU)/AU);
	sun_range_rate=1000.0*solar_set.w;

	Calculate_LatLonAlt(jul_utc, &solar_vector, &solar_latlonalt);

	sun_lat=Degrees(solar_latlonalt.lat);
	sun_lon=360.0-Degrees(solar_latlonalt.lon);

	Calculate_RADec(jul_utc, &solar_vector, &zero_vector, &obs_geodetic, &solar_rad);

	sun_ra=Degrees(solar_rad.x);
	sun_dec=Degrees(solar_rad.y);
}

void PreCalc(x)
int x;
{
	/* This function copies TLE data from PREDICT's sat structure
	   to the SGP4/SDP4's single dimensioned tle structure, and
	   prepares the tracking code for the update. */

	strcpy(tle.sat_name,sat[x].name);
	strcpy(tle.idesg,sat[x].designator);
	tle.catnr=sat[x].catnum;
	tle.epoch=(1000.0*(double)sat[x].year)+sat[x].refepoch;
	tle.xndt2o=sat[x].drag;
	tle.xndd6o=sat[x].nddot6;
	tle.bstar=sat[x].bstar;
	tle.xincl=sat[x].incl;
	tle.xnodeo=sat[x].raan;
	tle.eo=sat[x].eccn;
	tle.omegao=sat[x].argper;
	tle.xmo=sat[x].meanan;
	tle.xno=sat[x].meanmo;
	tle.revnum=sat[x].orbitnum;

	if (sat_db[x].squintflag)
	{
		calc_squint=1;
		alat=deg2rad*sat_db[x].alat;
		alon=deg2rad*sat_db[x].alon;
	}
	else
		calc_squint=0;
 
	/* Clear all flags */

	ClearFlag(ALL_FLAGS);

	/* Select ephemeris type.  This function will set or clear the
	   DEEP_SPACE_EPHEM_FLAG depending on the TLE parameters of the
	   satellite.  It will also pre-process tle members for the
	   ephemeris functions SGP4 or SDP4, so this function must
	   be called each time a new tle set is used. */

	select_ephemeris(&tle);
}

void Calc()
{
	/* This is the stuff we need to do repetitively while tracking. */

	/* Zero vector for initializations */
	vector_t zero_vector={0,0,0,0};

	/* Satellite position and velocity vectors */
	vector_t vel=zero_vector;
	vector_t pos=zero_vector;

	/* Satellite Az, El, Range, Range rate */
	vector_t obs_set;

	/* Solar ECI position vector  */
	vector_t solar_vector=zero_vector;

	/* Solar observed azi and ele vector  */
	vector_t solar_set;

	/* Satellite's predicted geodetic position */
	geodetic_t sat_geodetic;

	jul_utc=daynum+2444238.5;

	/* Convert satellite's epoch time to Julian  */
	/* and calculate time since epoch in minutes */

	jul_epoch=Julian_Date_of_Epoch(tle.epoch);
	tsince=(jul_utc-jul_epoch)*xmnpda;
	age=jul_utc-jul_epoch;

	/* Copy the ephemeris type in use to ephem string. */

		if (isFlagSet(DEEP_SPACE_EPHEM_FLAG))
			strcpy(ephem,"SDP4");
		else
			strcpy(ephem,"SGP4");

	/* Call NORAD routines according to deep-space flag. */

	if (isFlagSet(DEEP_SPACE_EPHEM_FLAG))
		SDP4(tsince, &tle, &pos, &vel);
	else
		SGP4(tsince, &tle, &pos, &vel);

	/* Scale position and velocity vectors to km and km/sec */

	Convert_Sat_State(&pos, &vel);

	/* Calculate velocity of satellite */

	Magnitude(&vel);
	sat_vel=vel.w;

	/** All angles in rads. Distance in km. Velocity in km/s **/
	/* Calculate satellite Azi, Ele, Range and Range-rate */

	Calculate_Obs(jul_utc, &pos, &vel, &obs_geodetic, &obs_set);

	/* Calculate satellite Lat North, Lon East and Alt. */

	Calculate_LatLonAlt(jul_utc, &pos, &sat_geodetic);

	/* Calculate squint angle */

	if (calc_squint)
		squint=(acos(-(ax*rx+ay*ry+az*rz)/obs_set.z))/deg2rad;

	/* Calculate solar position and satellite eclipse depth. */
	/* Also set or clear the satellite eclipsed flag accordingly. */

	Calculate_Solar_Position(jul_utc, &solar_vector);
	Calculate_Obs(jul_utc, &solar_vector, &zero_vector, &obs_geodetic, &solar_set);

	if (Sat_Eclipsed(&pos, &solar_vector, &eclipse_depth))
		SetFlag(SAT_ECLIPSED_FLAG);
	else
		ClearFlag(SAT_ECLIPSED_FLAG);

	if (isFlagSet(SAT_ECLIPSED_FLAG))
		sat_sun_status=0;  /* Eclipse */
	else
		sat_sun_status=1; /* In sunlight */

	/* Convert satellite and solar data */
	sat_azi=Degrees(obs_set.x);
	sat_ele=Degrees(obs_set.y);
	sat_range=obs_set.z;
	sat_range_rate=obs_set.w;
	sat_lat=Degrees(sat_geodetic.lat);
	sat_lon=Degrees(sat_geodetic.lon);
	sat_alt=sat_geodetic.alt;

	fk=12756.33*acos(xkmper/(xkmper+sat_alt));
	fm=fk/1.609344;

	rv=(long)floor((tle.xno*xmnpda/twopi+age*tle.bstar*ae)*age+tle.xmo/twopi)+tle.revnum;

	sun_azi=Degrees(solar_set.x); 
	sun_ele=Degrees(solar_set.y);

	irk=(long)rint(sat_range);
	isplat=(int)rint(sat_lat);
	isplong=(int)rint(360.0-sat_lon);
	iaz=(int)rint(sat_azi);
	iel=(int)rint(sat_ele);
	ma256=(int)rint(256.0*(phase/twopi));

	if (sat_sun_status)
	{
		if (sun_ele<=-12.0 && rint(sat_ele)>=0.0)
			findsun='+';
		else
			findsun='*';
	}
	else
		findsun=' ';
}

char AosHappens(x)
int x;
{
	/* This function returns a 1 if the satellite pointed to by
	   "x" can ever rise above the horizon of the ground station. */

	double lin, sma, apogee;

	if (sat[x].meanmo==0.0)
		return 0;
	else
	{
		lin=sat[x].incl;

		if (lin>=90.0)
			lin=180.0-lin;

		sma=331.25*exp(log(1440.0/sat[x].meanmo)*(2.0/3.0));
		apogee=sma*(1.0+sat[x].eccn)-xkmper;

		if ((acos(xkmper/(apogee+xkmper))+(lin*deg2rad)) > fabs(qth.stnlat*deg2rad))
			return 1;
		else
			return 0;
	}
}

char Decayed(x,time)
int x;
double time;
{
	/* This function returns a 1 if it appears that the
	   satellite pointed to by 'x' has decayed at the
	   time of 'time'.  If 'time' is 0.0, then the
	   current date/time is used. */

	double satepoch;

	if (time==0.0)
		time=CurrentDaynum();

	satepoch=DayNum(1,0,sat[x].year)+sat[x].refepoch;

	if (satepoch+((16.666666-sat[x].meanmo)/(10.0*fabs(sat[x].drag))) < time)
		return 1;
	else
		return 0;
}

char Geostationary(x)
int x;
{
	/* This function returns a 1 if the satellite pointed
	   to by "x" appears to be in a geostationary orbit */

	if (fabs(sat[x].meanmo-1.0027)<0.0002) 

		return 1;
	else
		return 0;
}

double FindAOS()
{
	/* This function finds and returns the time of AOS (aostime). */

	aostime=0.0;

	if (AosHappens(indx) && Geostationary(indx)==0 && Decayed(indx,daynum)==0)
	{
		Calc();

		/* Get the satellite in range */

		while (sat_ele<-1.0)
		{
			daynum-=0.00035*(sat_ele*((sat_alt/8400.0)+0.46)-2.0);
			Calc();
		}

		/* Find AOS */

		while (aostime==0.0)
		{
			if (fabs(sat_ele)<0.03)
				aostime=daynum;
			else
			{
				daynum-=sat_ele*sqrt(sat_alt)/530000.0;
				Calc();
			}
		}
	}

	return aostime;
}

double FindLOS()
{
	lostime=0.0;

	if (Geostationary(indx)==0 && AosHappens(indx)==1 && Decayed(indx,daynum)==0)
	{
		Calc();

		do
		{
			daynum+=sat_ele*sqrt(sat_alt)/502500.0;
			Calc();

			if (fabs(sat_ele) < 0.03)
				lostime=daynum;

		} while (lostime==0.0);
	}

	return lostime;
}

double FindLOS2()
{
	/* This function steps through the pass to find LOS.
	   FindLOS() is called to "fine tune" and return the result. */

	do
	{
		daynum+=cos((sat_ele-1.0)*deg2rad)*sqrt(sat_alt)/25000.0;
		Calc();

	} while (sat_ele>=0.0);

	return(FindLOS());
}

double NextAOS()
{
	/* This function finds and returns the time of the next
	   AOS for a satellite that is currently in range. */

	aostime=0.0;

	if (AosHappens(indx) && Geostationary(indx)==0 && Decayed(indx,daynum)==0)
		daynum=FindLOS2()+0.014;  /* Move to LOS + 20 minutes */

	return (FindAOS());
}

int Print(string,mode)
char *string, mode;
{
	/* This function buffers and displays orbital predictions
	   and allows screens to be saved to a disk file. */

	char type[20], spaces[80], head1[160], head2[70],
	     head3[72], satellite_name[25];
	int key, ans=0, l, x, t;
	static char buffer[1450], lines, quit;
	static FILE *fd;

	/* Pass a NULL string to initialize the buffer, counter, and flags */

	if (string[0]==0)
	{
		lines=0;
		quit=0;
		buffer[0]=0;
		fd=NULL;
	}

	else
	{
		if (mode=='m')
		{
			sprintf(head1,"\n                    %s's Orbit Calendar for the Moon",qth.callsign);
			strncpy(satellite_name,"Moon\0",5);
		}

		if (mode=='o')
		{
			sprintf(head1,"\n                    %s's Orbit Calendar for the Sun",qth.callsign);
			strncpy(satellite_name,"Sun\0",4);
		}
	
		if (mode=='m' || mode=='o')

			sprintf(head2,"\n\t   Date     Time    El   Az   RA     Dec    GHA     Vel   Range\n");


		if (mode=='p')
			strcpy(type,"Orbit");

		if (mode=='v')
			strcpy(type,"Visual");

		if (mode=='s')
			strcpy(type,"Solar Illumination");

		if (mode!='m' && mode!='o')
		{
			strcpy(satellite_name,sat[indx].name);

			l=strlen(qth.callsign)+strlen(satellite_name)+strlen(type);

			spaces[0]=0;

			for (x=l; x<60; x+=2)
				strcat(spaces," ");

			sprintf(head1,"\n%s%s's %s Calendar For %s", spaces, qth.callsign, type, satellite_name);

			if (mode=='s')
				sprintf(head2,"\n\t  Date     Mins/Day    Sun%c          Date      Mins/Day    Sun%c\n",37,37);
			else
			{
				if (calc_squint)

					sprintf(head2,"\n\t   Date     Time    El   Az  Phase  %s   %s    Range  Squint\n",(io_lat=='N'?"LatN":"LatS"),(io_lon=='W'?"LonW":"LonE"));
				else

					sprintf(head2,"\n\t   Date     Time    El   Az  Phase  %s   %s    Range  Orbit\n",(io_lat=='N'?"LatN":"LatS"),(io_lon=='W'?"LonW":"LonE"));
			}
		}

		sprintf(head3,"      -----------------------------------------------------------------");

		strcat(buffer,string);
		lines++;

		if (lines==18)
		{
			bkgdset(COLOR_PAIR(2)|A_BOLD);
			clear();
			addstr(head1);
			attrset(COLOR_PAIR(4)|A_BOLD);
			addstr(head2);
			addstr(head3);
			attrset(COLOR_PAIR(3)|A_BOLD);

			if (buffer[0]!='\n')
				printw("\n");

			addstr(buffer);
			attrset(COLOR_PAIR(4)|A_BOLD);

			if (buffer[0]=='\n')
				printw("\n");

			if (fd==NULL)
				mvprintw(23,63,"        ");
			else
				mvprintw(23,63,"Log = ON");

			mvprintw(23,6,"More? [y/n] >> ");
			curs_set(1);
			refresh();

			while (ans==0)
			{
				key=toupper(getch());

				if (key=='Y' || key=='\n' || key==' ')
				{
					key='Y';
					ans=1;
					quit=0;
				}
			
				if (key=='N' || key=='Q' || key==27)
				{
					key='N';
					ans=1;
					quit=1;
				}

				/* 'L' logs output to "satname.txt" */

				if (key=='L' && fd==NULL && buffer[0])
				{
					sprintf(temp,"%s.txt",satellite_name);

					l=strlen(temp)-4;

					for (x=0; x<l; x++)
					{
						t=temp[x];

						if (t==32 || t==17 || t==92 || t==42 || t==46 || t==47)
							t='_';

						temp[x]=t;
					}

					fd=fopen(temp,"a");
					fprintf(fd,"%s%s%s\n",head1,head2,head3);
					fprintf(fd,"%s",buffer);
					mvprintw(23,63,"Log = ON");
					move(23,21);
					refresh();
				}

				else if (fd!=NULL)
				{
					if (key=='L' || key=='N')
					{
						fprintf(fd,"%s\n\n",buffer);
						fclose(fd);
						fd=NULL;
						mvprintw(23,63,"        ");
						move(23,21);
						refresh();
					}

					else
						fprintf(fd,"%s",buffer);
				}
				buffer[0]=0;
			}

			lines=0;
			curs_set(0);
		}
	}
	return (quit);
}

int PrintVisible(string)
char *string;
{
	/* This function acts as a filter to display passes that could
	   possibly be optically visible to the ground station.  It works
	   by first buffering prediction data generated by the Predict()
	   function and then checking it to see if at least a part of the
	   pass is visible.  If it is, then the buffered prediction data
	   is sent to the Print() function so it can be displayed
	   to the user and optionally logged to a file. */

	static char buffer[10000];
	char line[80], plus, asterisk, visible;
	int x, y, quit=0;

	if (string[0]==0)
		buffer[0]=0;
	else
	{
		strcat(buffer,string);

		if (string[0]=='\n')
		{
			plus=0;
			visible=0;
			asterisk=0;

			for (x=0; buffer[x]!=0 && visible==0; x++)
			{
				if (buffer[x]=='+')
					plus++;

				if (buffer[x]=='*')
					asterisk++;

				/* At least 3 +'s or at least 2 +'s
				   combined with at least 2 *'s is
				   worth displaying as a visible pass. */

				if ((plus>3) || (plus>2 && asterisk>2))
					visible=1;
			}

			if (visible)
			{
				/* Dump buffer to Print() line by line */

				for (x=0, y=0; buffer[x]!=0 && quit==0; x++)
				{	
					line[y]=buffer[x];

					if (line[y]=='\n')
					{
						line[y+1]=0;
						quit=Print(line,'v');
						line[0]=0;
						y=0;
					}

					else
						y++;
				}
			}

			buffer[0]=0;
		}
	}

	return quit;
}

void Predict(mode)
char mode;
{
	/* This function predicts satellite passes.  It displays
	   output through the Print() function if mode=='p' (show
	   all passes), or through the PrintVisible() function if
	   mode=='v' (show optically visible passes only). */

	int quit=0, lastel=0, breakout=0;
	char string[80], type[10];

	PreCalc(indx);
	daynum=GetStartTime(0);
	clear();

	/* Trap geostationary orbits and passes that cannot occur. */

	if (AosHappens(indx) && Geostationary(indx)==0 && Decayed(indx,daynum)==0)
	{
		if (xterm)
		{
			strcpy(type,"Orbit");  /* Default */

			if (mode=='v')
				strcpy(type,"Visual");

			fprintf(stderr,"\033]0;PREDICT: %s's %s Calendar For %s\007",qth.callsign, type, sat[indx].name);
		}

		do
		{
			daynum=FindAOS();
		
			/* Display the pass */

			while (iel>=0 && quit==0)
			{
				if (calc_squint)

					sprintf(string,"      %s%4d %4d  %4d  %4d   %4d   %6ld  %4.0f %c\n",Daynum2String(daynum),iel,iaz,ma256,(io_lat=='N'?+1:-1)*isplat,(io_lon=='W'?isplong:360-isplong),irk,squint,findsun);

				else
					sprintf(string,"      %s%4d %4d  %4d  %4d   %4d   %6ld  %6ld %c\n",Daynum2String(daynum),iel,iaz,ma256,(io_lat=='N'?+1:-1)*isplat,(io_lon=='W'?isplong:360-isplong),irk,rv,findsun);

				lastel=iel;

				if (mode=='p')
					quit=Print(string,'p');

				if (mode=='v')
				{
					nodelay(stdscr,TRUE);
					attrset(COLOR_PAIR(4));
					mvprintw(23,6,"                 Calculating... Press [ESC] To Quit");

					/* Allow a way out if this
					   should continue forever... */

					if (getch()==27)
						breakout=1;

					nodelay(stdscr,FALSE);

					quit=PrintVisible(string);
				}

				daynum+=cos((sat_ele-1.0)*deg2rad)*sqrt(sat_alt)/25000.0;
				Calc();
			}

			if (lastel!=0)
			{
				daynum=FindLOS();
				Calc();

				if (calc_squint)
					sprintf(string,"      %s%4d %4d  %4d  %4d   %4d   %6ld  %4.0f %c\n",Daynum2String(daynum),iel,iaz,ma256,(io_lat=='N'?+1:-1)*isplat,(io_lon=='W'?isplong:360-isplong),irk,squint,findsun);

				else
					sprintf(string,"      %s%4d %4d  %4d  %4d   %4d   %6ld  %6ld %c\n",Daynum2String(daynum),iel,iaz,ma256,(io_lat=='N'?+1:-1)*isplat,(io_lon=='W'?isplong:360-isplong),irk,rv,findsun);

				if (mode=='p')
					quit=Print(string,'p');

				if (mode=='v')
					quit=PrintVisible(string);
			}

			if (mode=='p')
				quit=Print("\n",'p');

			if (mode=='v')
				quit=PrintVisible("\n");

			/* Move to next orbit */
			daynum=NextAOS();

		}  while (quit==0 && breakout==0 && AosHappens(indx) && Decayed(indx,daynum)==0);
	}

	else
	{
		bkgdset(COLOR_PAIR(5)|A_BOLD);
		clear();

		if (AosHappens(indx)==0 || Decayed(indx,daynum)==1)
			mvprintw(12,5,"*** Passes for %s cannot occur for your ground station! ***\n",sat[indx].name);

		if (Geostationary(indx)==1)
			mvprintw(12,3,"*** Orbital predictions cannot be made for a geostationary satellite! ***\n");

		beep();
		bkgdset(COLOR_PAIR(7)|A_BOLD);
		AnyKey();
		refresh();
	}
}

void PredictMoon()
{
	/* This function predicts "passes" of the Moon */

	int iaz, iel, lastel=0;
	char string[80], quit=0;
	double daynum, lastdaynum, moonrise=0.0;

	daynum=GetStartTime('m');
	clear();

	if (xterm)
		fprintf(stderr,"\033]0;PREDICT: %s's Orbit Calendar for the Moon\007",qth.callsign);

	do
	{
		/* Determine moonrise */

		FindMoon(daynum);

		while (moonrise==0.0)
		{
			if (fabs(moon_el)<0.03)
				moonrise=daynum;
			else
			{
				daynum-=(0.004*moon_el);
				FindMoon(daynum);
			}
		}

		FindMoon(moonrise);
		daynum=moonrise;
		iaz=(int)rint(moon_az);
		iel=(int)rint(moon_el);

		do
		{
			/* Display pass of the moon */

			sprintf(string,"      %s%4d %4d  %5.1f  %5.1f  %5.1f  %6.1f%7.3f\n",Daynum2String(daynum), iel, iaz, moon_ra, moon_dec, moon_gha, moon_dv, moon_dx);
			quit=Print(string,'m');
			lastel=iel;
			lastdaynum=daynum;
			daynum+=0.04*(cos(deg2rad*(moon_el+0.5)));
			FindMoon(daynum);
			iaz=(int)rint(moon_az);
			iel=(int)rint(moon_el);

		} while (iel>3 && quit==0);

		while (lastel!=0 && quit==0)
		{
			daynum=lastdaynum;

			do
			{
				/* Determine setting time */

				daynum+=0.004*(sin(deg2rad*(moon_el+0.5)));
				FindMoon(daynum);
				iaz=(int)rint(moon_az);
				iel=(int)rint(moon_el);

			} while (iel>0);

			/* Print moonset */

			sprintf(string,"      %s%4d %4d  %5.1f  %5.1f  %5.1f  %6.1f%7.3f\n",Daynum2String(daynum), iel, iaz, moon_ra, moon_dec, moon_gha, moon_dv, moon_dx);
			quit=Print(string,'m');
			lastel=iel;
		}

		quit=Print("\n",'m');
		daynum+=0.4;
		moonrise=0.0;

	} while (quit==0);
}

void PredictSun()
{
	/* This function predicts "passes" of the Sun. */

	int iaz, iel, lastel=0;
	char string[80], quit=0;
	double daynum, lastdaynum, sunrise=0.0;

	daynum=GetStartTime('o');
	clear();

	if (xterm)
		fprintf(stderr,"\033]0;PREDICT: %s's Orbit Calendar for the Sun\007",qth.callsign);

	do
	{
		/* Determine sunrise */

		FindSun(daynum);

		while (sunrise==0.0)
		{
			if (fabs(sun_ele)<0.03)
				sunrise=daynum;
			else
			{
				daynum-=(0.004*sun_ele);
				FindSun(daynum);
			}
		}

		FindSun(sunrise);
		daynum=sunrise;
		iaz=(int)rint(sun_azi);
		iel=(int)rint(sun_ele);

		/* Print time of sunrise */

		do
		{
			/* Display pass of the sun */

			sprintf(string,"      %s%4d %4d  %5.1f  %5.1f  %5.1f  %6.1f%7.3f\n",Daynum2String(daynum), iel, iaz, sun_ra, sun_dec, sun_lon, sun_range_rate, sun_range);
			quit=Print(string,'o');
			lastel=iel;
			lastdaynum=daynum;
			daynum+=0.04*(cos(deg2rad*(sun_ele+0.5)));
			FindSun(daynum);
			iaz=(int)rint(sun_azi);
			iel=(int)rint(sun_ele);

		} while (iel>3 && quit==0);

		while (lastel!=0 && quit==0)
		{
			daynum=lastdaynum;

			do
			{
				/* Find sun set */

				daynum+=0.004*(sin(deg2rad*(sun_ele+0.5)));
				FindSun(daynum);
				iaz=(int)rint(sun_azi);
				iel=(int)rint(sun_ele);

			} while (iel>0);

			/* Print time of sunset */

			sprintf(string,"      %s%4d %4d  %5.1f  %5.1f  %5.1f  %6.1f%7.3f\n",Daynum2String(daynum), iel, iaz, sun_ra, sun_dec, sun_lon, sun_range_rate, sun_range);
			quit=Print(string,'o');
			lastel=iel;
		}

		quit=Print("\n",'o');
		daynum+=0.4;
		sunrise=0.0;

	} while (quit==0);
}

char KbEdit(x,y)
int x,y;
{
	/* This function is used when editing QTH
	   and orbital data via the keyboard. */

	char need2save=0, input[25];

	echo();
	move(y-1,x-1);
	wgetnstr(stdscr,input,24);

	if (strlen(input)!=0)
	{
		need2save=1;  /* Save new data to variables */
		resave=1;     /* Save new data to disk files */
		strncpy(temp,input,24);
	}

	mvprintw(y-1,x-1,"%-25s",temp);

	refresh();
	noecho();

	return need2save;
}

void ShowOrbitData()
{
	/* This function permits displays a satellite's orbital
	   data.  The age of the satellite data is also provided. */

	int c, x, namelength, age;	
	double an_period, no_period, sma, c1, e2, satepoch;
	char days[5];

	x=Select();

	while (x!=-1)
	{
		if (sat[x].meanmo!=0.0)
		{
			bkgdset(COLOR_PAIR(2)|A_BOLD);
			clear();
			sma=331.25*exp(log(1440.0/sat[x].meanmo)*(2.0/3.0));
			an_period=1440.0/sat[x].meanmo;
			c1=cos(sat[x].incl*deg2rad);
			e2=1.0-(sat[x].eccn*sat[x].eccn); 
			no_period=(an_period*360.0)/(360.0+(4.97*pow((xkmper/sma),3.5)*((5.0*c1*c1)-1.0)/(e2*e2))/sat[x].meanmo);
			satepoch=DayNum(1,0,sat[x].year)+sat[x].refepoch;
			age=(int)rint(CurrentDaynum()-satepoch);

			if (age==1)
				strcpy(days,"day");
			else
				strcpy(days,"days");

			namelength=strlen(sat[x].name);

			printw("\n");

			for (c=41; c>namelength; c-=2)
				printw(" ");
	
			printw("Orbital Data For %s / Catalog Number %ld\n",sat[x].name,sat[x].catnum);
			attrset(COLOR_PAIR(3)|A_BOLD);
			printw("\n\t\t  Issued %d %s ago on %s UTC\n\n",age,days,Daynum2String(satepoch));

			attrset(COLOR_PAIR(4)|A_BOLD);
			mvprintw(5,21,"Reference Epoch");
			mvprintw(6,21,"Inclination");
			mvprintw(7,21,"RAAN");
			mvprintw(8,21,"Eccentricity");
			mvprintw(9,21,"Arg of Perigee");
			mvprintw(10,21,"Mean Anomaly");
			mvprintw(11,21,"Mean Motion");
			mvprintw(12,21,"Decay Rate");
			mvprintw(13,21,"Nddot/6 Drag");
			mvprintw(14,21,"Bstar Drag Factor");
			mvprintw(15,21,"Semi-Major Axis");
			mvprintw(16,21,"Apogee Altitude");
			mvprintw(17,21,"Perigee Altitude");
			mvprintw(18,21,"Anomalistic Period");
			mvprintw(19,21,"Nodal Period");
			mvprintw(20,21,"Orbit Number");
			mvprintw(21,21,"Element Set Number");

			attrset(COLOR_PAIR(2)|A_BOLD);
			mvprintw(5,40,": %02d %.8f",sat[x].year,sat[x].refepoch);
			mvprintw(6,40,": %.4f deg",sat[x].incl);
			mvprintw(7,40,": %.4f deg",sat[x].raan);
			mvprintw(8,40,": %g",sat[x].eccn);
			mvprintw(9,40,": %.4f deg",sat[x].argper);
			mvprintw(10,40,": %.4f deg",sat[x].meanan);
			mvprintw(11,40,": %.8f rev/day",sat[x].meanmo);
			mvprintw(12,40,": %g rev/day/day",sat[x].drag);
			mvprintw(13,40,": %g rev/day/day/day",sat[x].nddot6);
			mvprintw(14,40,": %g 1/earth radii",sat[x].bstar);
			mvprintw(15,40,": %.4f km",sma);
			mvprintw(16,40,": %.4f km",sma*(1.0+sat[x].eccn)-xkmper);
			mvprintw(17,40,": %.4f km",sma*(1.0-sat[x].eccn)-xkmper);
			mvprintw(18,40,": %.4f mins",an_period);
			mvprintw(19,40,": %.4f mins",no_period);
			mvprintw(20,40,": %ld",sat[x].orbitnum);
			mvprintw(21,40,": %ld",sat[x].setnum);

			attrset(COLOR_PAIR(3)|A_BOLD);
			refresh();
			AnyKey();
		}

		x=Select();
	 };
}	

void KepEdit()
{
	/* This function permits keyboard editing of the orbital database. */

	int x;

	do
	{
		x=Select();

		if (x!=-1)
		{
			bkgdset(COLOR_PAIR(3)|A_BOLD);
			clear();
			mvprintw(3,1,"\t\t   *  Orbital Database Editing Utility  *\n\n\n");
			attrset(COLOR_PAIR(4)|A_BOLD);

			printw("\n\t\t\tSpacecraft Name :");
			printw("\n\t\t\tCatalog Number  :");
			printw("\n\t\t\tDesignator      :");
			printw("\n\t\t\tReference Epoch :");
			printw("\n\t\t\tInclination     :");
			printw("\n\t\t\tRAAN            :");
			printw("\n\t\t\tEccentricity    :");
			printw("\n\t\t\tArg of Perigee  :");
			printw("\n\t\t\tMean Anomaly    :");
			printw("\n\t\t\tMean Motion     :");
			printw("\n\t\t\tDecay Rate      :");
			printw("\n\t\t\tNddot/6         :");
			printw("\n\t\t\tBstar Drag Term :");        
			printw("\n\t\t\tOrbit Number    :");
			printw("\n\t\t\tElement Set No. :");

			attrset(COLOR_PAIR(2)|A_BOLD);

			mvprintw(7,42,"%s",sat[x].name);
			mvprintw(8,42,"%ld",sat[x].catnum);
			mvprintw(9,42,"%s",sat[x].designator);
			mvprintw(10,42,"%02d %.8f",sat[x].year,sat[x].refepoch);
			mvprintw(11,42,"%.4f",sat[x].incl);
			mvprintw(12,42,"%.4f",sat[x].raan);
			mvprintw(13,42,"%g",sat[x].eccn);
			mvprintw(14,42,"%.4f",sat[x].argper);
			mvprintw(15,42,"%.4f",sat[x].meanan);
			mvprintw(16,42,"%.8f",sat[x].meanmo);
			mvprintw(17,42,"%g",sat[x].drag);
			mvprintw(18,42,"%g",sat[x].nddot6);
			mvprintw(19,42,"%g",sat[x].bstar);
			mvprintw(20,42,"%ld",sat[x].orbitnum);
			mvprintw(21,42,"%ld",sat[x].setnum);

			curs_set(1);
			refresh();

			sprintf(temp,"%s",sat[x].name);

			if (KbEdit(43,8))
				strncpy(sat[x].name,temp,24);

			sprintf(temp,"%ld",sat[x].catnum);

			if (KbEdit(43,9))
				sscanf(temp,"%ld",&sat[x].catnum);

			sprintf(temp,"%s",sat[x].designator);

			if (KbEdit(43,10))
				sscanf(temp,"%s",sat[x].designator);

			sprintf(temp,"%02d %4.8f",sat[x].year,sat[x].refepoch);

			if (KbEdit(43,11))
				sscanf(temp,"%d %lf",&sat[x].year,&sat[x].refepoch);

			sprintf(temp,"%4.4f",sat[x].incl);

			if (KbEdit(43,12))
				sscanf(temp,"%lf",&sat[x].incl);
			    
			sprintf(temp,"%4.4f",sat[x].raan);

			if (KbEdit(43,13))
				sscanf(temp,"%lf",&sat[x].raan);

			sprintf(temp,"%g",sat[x].eccn);

			if (KbEdit(43,14))
				sscanf(temp,"%lf",&sat[x].eccn);
			    
			sprintf(temp,"%4.4f",sat[x].argper);

			if (KbEdit(43,15))
				sscanf(temp,"%lf",&sat[x].argper);
			    
			sprintf(temp,"%4.4f",sat[x].meanan);

			if (KbEdit(43,16))
				sscanf(temp,"%lf",&sat[x].meanan);

			sprintf(temp,"%4.8f",sat[x].meanmo);

			if (KbEdit(43,17))
				sscanf(temp,"%lf",&sat[x].meanmo);
			    
			sprintf(temp,"%g",sat[x].drag);

			if (KbEdit(43,18))
				sscanf(temp,"%lf",&sat[x].drag);
			    
			sprintf(temp,"%g",sat[x].nddot6);

			if (KbEdit(43,19))
				sscanf(temp,"%lf",&sat[x].nddot6);

			sprintf(temp,"%g",sat[x].bstar);

			if (KbEdit(43,20))
				sscanf(temp,"%lf",&sat[x].bstar);

			sprintf(temp,"%ld",sat[x].orbitnum);

			if (KbEdit(43,21))
				sscanf(temp,"%ld",&sat[x].orbitnum);

			sprintf(temp,"%ld",sat[x].setnum);

			if (KbEdit(43,22))
				sscanf(temp,"%ld",&sat[x].setnum);
		  	
			curs_set(0);
		}

	} while (x!=-1);

	if (resave)
	{
		SaveTLE();
		resave=0;
	}
}	

void QthEdit()
{
	/* This function permits keyboard editing of
	   the ground station's location information. */

	bkgdset(COLOR_PAIR(3)|A_BOLD);
	clear();
	curs_set(1);
	mvprintw(7,0,"\t\t *  Ground Station Location Editing Utility  *\n\n\n");

	attrset(COLOR_PAIR(4)|A_BOLD);
	printw("\n\t\t\tStation Callsign  :");
	printw("\n\t\t\tStation Latitude  :");
	printw("\n\t\t\tStation Longitude :");
	printw("\n\t\t\tStation Altitude  :");

	attrset(COLOR_PAIR(2)|A_BOLD);
	mvprintw(11,44,"%s",qth.callsign);

	if (io_lat=='N')
		mvprintw(12,44,"%g [DegN]",+qth.stnlat);
	else
		mvprintw(12,44,"%g [DegS]",-qth.stnlat);

	if (io_lon=='W')
		mvprintw(13,44,"%g [DegW]",+qth.stnlong);
	else
		mvprintw(13,44,"%g [DegE]",-qth.stnlong);

	mvprintw(14,44,"%d [m]",qth.stnalt);

	refresh();

	sprintf(temp,"%s",qth.callsign);

	mvprintw(18,12,"Enter the callsign or identifier of your ground station");

	if (KbEdit(45,12))
		strncpy(qth.callsign,temp,16);

	if (io_lat=='N')
		sprintf(temp,"%g [DegN]",+qth.stnlat);
	else
		sprintf(temp,"%g [DegS]",-qth.stnlat);

	if (io_lat=='N')
		mvprintw(18,12,"Enter your latitude in degrees NORTH  (south=negative) ");
	else
		mvprintw(18,12,"Enter your latitude in degrees SOUTH  (north=negative) ");
 
	mvprintw(19,12,"  Decimal (74.2467) or DMS (74 14 48) format allowed");

	if (KbEdit(45,13))
	{
		if (io_lat=='N')
			qth.stnlat=+ReadBearing(temp);
		else
			qth.stnlat=-ReadBearing(temp);
	}
 
	if (io_lon=='W')
		sprintf(temp,"%g [DegW]",+qth.stnlong);
	else
		sprintf(temp,"%g [DegE]",-qth.stnlong);
 
	if (io_lon=='W')
		mvprintw(18,12,"Enter your longitude in degrees WEST   (east=negative) ");
	else
		mvprintw(18,12,"Enter your longitude in degrees EAST   (west=negative) ");
 
	if (KbEdit(45,14))
	{
		if (io_lon=='W')
			qth.stnlong=+ReadBearing(temp);
		else
			qth.stnlong=-ReadBearing(temp);
	}
 
	move(19,12);
	clrtoeol();
	mvprintw(18,12,"    Enter your altitude above sea level (in meters)   ");

	sprintf(temp,"%d",qth.stnalt);

	if (KbEdit(45,15))
		sscanf(temp,"%d",&qth.stnalt);

	if (resave)
	{
		SaveQTH();
		resave=0;
	}
}

void SingleTrack(x,speak)
int x;
char speak;
{
	/* This function tracks a single satellite in real-time
	   until 'Q' or ESC is pressed.  x represents the index
	   of the satellite being tracked.  If speak=='T', then
	   the speech routines are enabled. */

	int	ans, oldaz=0, oldel=0, length, xponder=0,
		polarity=0, tshift, bshift;
	char	approaching=0, command[80], comsat, aos_alarm=0,
		geostationary=0, aoshappens=0, decayed=0,
		eclipse_alarm=0, visibility=0, old_visibility=0;
	double	oldtime=0.0, nextaos=0.0, lostime=0.0, aoslos=0.0,
		downlink=0.0, uplink=0.0, downlink_start=0.0,
		downlink_end=0.0, uplink_start=0.0, uplink_end=0.0,
		dopp, doppler100=0.0, delay, loss, shift;
	long	newtime, lasttime=0;

	PreCalc(x);
	indx=x;

	if (sat_db[x].transponders>0)
	{
		comsat=1;
		tshift=0;
		bshift=0;
	}

	else
	{
		comsat=0;
		tshift=2;
		bshift=-2;
	}

	if (comsat)
	{
		downlink_start=sat_db[x].downlink_start[xponder];
		downlink_end=sat_db[x].downlink_end[xponder];
		uplink_start=sat_db[x].uplink_start[xponder];
		uplink_end=sat_db[x].uplink_end[xponder];

		if (downlink_start>downlink_end)
			polarity=-1;

		if (downlink_start<downlink_end)
			polarity=1;

		if (downlink_start==downlink_end)
			polarity=0;

		downlink=0.5*(downlink_start+downlink_end);
		uplink=0.5*(uplink_start+uplink_end);
	}

	daynum=CurrentDaynum();
	aoshappens=AosHappens(indx);
	geostationary=Geostationary(indx);
	decayed=Decayed(indx,0.0);

	if (xterm)
		fprintf(stderr,"\033]0;PREDICT: Tracking %-10s\007",sat[x].name); 
	halfdelay(2);
	curs_set(0);
	bkgdset(COLOR_PAIR(3));
	clear();

	attrset(COLOR_PAIR(6)|A_REVERSE|A_BOLD);

	printw("                                                                                ");
	printw("                     PREDICT Real-Time Satellite Tracking                        ");
	printw("                 Tracking: %-10sOn                                       ",Abbreviate(sat[x].name,9));
	printw("                                                                                 ");

	attrset(COLOR_PAIR(4)|A_BOLD);

	mvprintw(5+tshift,1,"Satellite     Direction     Velocity     Footprint    Altitude     Slant Range");
	mvprintw(6+tshift,1,"---------     ---------     --------     ---------    --------     -----------");
	mvprintw(7+tshift,1,"        .            Az           mi            mi          mi              mi");
	mvprintw(8+tshift,1,"        .            El           km            km          km              km");
	mvprintw(16+bshift,1,"Eclipse Depth   Orbital Phase   Orbital Model   Squint Angle      AutoTracking");
	mvprintw(17+bshift,1,"-------------   -------------   -------------   ------------      ------------");

	if (comsat)
	{
		mvprintw(12,1,"Uplink   :");
		mvprintw(13,1,"Downlink :");
		mvprintw(14,1,"Delay    :");
		mvprintw(14,55,"Echo      :");
		mvprintw(13,29,"RX:");
		mvprintw(13,55,"Path loss :");
		mvprintw(12,29,"TX:");
		mvprintw(12,55,"Path loss :");
	}

	do
	{
		attrset(COLOR_PAIR(6)|A_REVERSE|A_BOLD);
		daynum=CurrentDaynum();
		mvprintw(2,41,"%s",Daynum2String(daynum));
		attrset(COLOR_PAIR(2)|A_BOLD);
		Calc();

		mvprintw(7+tshift,1,"%-6.2f",(io_lat=='N'?+1:-1)*sat_lat);
		attrset(COLOR_PAIR(4)|A_BOLD);
		mvprintw(7+tshift,8,(io_lat=='N'?"N":"S"));
		mvprintw(8+tshift,8,(io_lon=='W'?"W":"E"));

		fk=12756.33*acos(xkmper/(xkmper+sat_alt));
		fm=fk*km2mi;

		attrset(COLOR_PAIR(2)|A_BOLD);

		mvprintw(7+tshift,55,"%0.f ",sat_alt*km2mi);
		mvprintw(8+tshift,55,"%0.f ",sat_alt);
		mvprintw(7+tshift,68,"%-5.0f",sat_range*km2mi);
		mvprintw(8+tshift,68,"%-5.0f",sat_range);

		mvprintw(8+tshift,1,"%-7.2f",(io_lon=='W'?360.0-sat_lon:sat_lon));
		mvprintw(7+tshift,15,"%-7.2f",sat_azi);
		mvprintw(8+tshift,14,"%+-6.2f",sat_ele);
		mvprintw(7+tshift,29,"%0.f ",(3600.0*sat_vel)*km2mi);
		mvprintw(8+tshift,29,"%0.f ",3600.0*sat_vel);

		mvprintw(18+bshift,3,"%+6.2f%c  ",eclipse_depth/deg2rad,176);
		mvprintw(18+bshift,20,"%5.1f",256.0*(phase/twopi));
		mvprintw(18+bshift,37,"%s",ephem);

		if (sat_sun_status)
		{
			if (sun_ele<=-12.0 && sat_ele>=0.0)
				visibility_array[indx]='V';
			else
				visibility_array[indx]='D';
		}

		else
			visibility_array[indx]='N';

		visibility=visibility_array[indx];

		if (comsat)
		{
			if (downlink!=0.0)
				mvprintw(13,11,"%11.5f MHz",downlink);

			else
				mvprintw(13,11,"               ");

			if (uplink!=0.0)
				mvprintw(12,11,"%11.5f MHz",uplink);

			else
				mvprintw(12,11,"               ");
		}

		if (antfd!=-1)
		{
			if (sat_ele>=0.0)
				mvprintw(18+bshift,67,"   Active   ");
			else
				mvprintw(18+bshift,67,"Standing  By");
		}
		else
			mvprintw(18+bshift,67,"Not  Enabled");

		if (calc_squint)
			mvprintw(18+bshift,52,"%+6.2f",squint);
		else
			mvprintw(18+bshift,54,"N/A");

		doppler100=-100.0e06*((sat_range_rate*1000.0)/299792458.0);
		delay=1000.0*((1000.0*sat_range)/299792458.0);

		if (sat_ele>=0.0)
		{
			if (aos_alarm==0)
			{
				beep();
				aos_alarm=1;
			}

			if (comsat)
			{
				attrset(COLOR_PAIR(4)|A_BOLD);

				if (fabs(sat_range_rate)<0.1)
					mvprintw(14,34,"    TCA    ");

				else
				{
					if (sat_range_rate<0.0)
						mvprintw(14,34,"Approaching");

					if (sat_range_rate>0.0)
						mvprintw(14,34,"  Receding ");
				}

				attrset(COLOR_PAIR(2)|A_BOLD);

				if (downlink!=0.0)
				{
					dopp=1.0e-08*(doppler100*downlink);
					mvprintw(13,32,"%11.5f MHz",downlink+dopp);
					loss=32.4+(20.0*log10(downlink))+(20.0*log10(sat_range));
					mvprintw(13,67,"%7.3f dB",loss);
					mvprintw(14,13,"%7.3f   ms",delay);
				}

				else
				{
					mvprintw(13,32,"                ");
					mvprintw(13,67,"          ");
					mvprintw(14,13,"            ");
				}

				if (uplink!=0.0)
				{
					dopp=1.0e-08*(doppler100*uplink);
					mvprintw(12,32,"%11.5f MHz",uplink-dopp);
					loss=32.4+(20.0*log10(uplink))+(20.0*log10(sat_range));
					mvprintw(12,67,"%7.3f dB",loss);
				}

				else
				{
					mvprintw(12,32,"                ");
					mvprintw(12,67,"          ");
				}

				if (uplink!=0.0 && downlink!=0.0)
					mvprintw(14,67,"%7.3f ms",2.0*delay);

				else
					mvprintw(14,67,"              ");
			}

			if (speak=='T' && soundcard)
			{
				if (eclipse_alarm==0 && fabs(eclipse_depth)<0.015) /* ~1 deg */
				{
					/* Hold off regular announcements if
					   satellite is within about 2 degrees
					   of entering into or out of an
					   eclipse. */

					oldtime=CurrentDaynum();

					if ((old_visibility=='V' || old_visibility=='D') && visibility=='N')
					{
						sprintf(command,"%svocalizer/vocalizer eclipse &",predictpath);
						system(command);
						eclipse_alarm=1;
						oldtime-=0.000015*sqrt(sat_alt);
					}

					if (old_visibility=='N' && (visibility=='V' || visibility=='D'))
					{
						sprintf(command,"%svocalizer/vocalizer sunlight &",predictpath);
						system(command);
						eclipse_alarm=1;
						oldtime-=0.000015*sqrt(sat_alt);
					}
				}

				if ((CurrentDaynum()-oldtime)>(0.00003*sqrt(sat_alt)))
				{
					if (sat_range_rate<0.0)
						approaching='+';

					if (sat_range_rate>0.0)
						approaching='-';

					sprintf(command,"%svocalizer/vocalizer %.0f %.0f %c %c &",predictpath,sat_azi,sat_ele,approaching,visibility);
					system(command);
  					oldtime=CurrentDaynum();
					old_visibility=visibility;
				}

				if (sat_ele<=1.0 && approaching=='-')
				{
					/* Suspend regular announcements
					   as we approach LOS. */

					oldtime=CurrentDaynum();
				}
			}
		}

		else
		{
			lostime=0.0;
			aos_alarm=0;
			eclipse_alarm=0;

			if (comsat)
			{
				mvprintw(12,32,"                ");
				mvprintw(12,67,"          ");
				mvprintw(13,32,"                ");
				mvprintw(13,67,"          ");
				mvprintw(14,13,"            ");
				mvprintw(14,34,"           ");
				mvprintw(14,67,"          ");
			}
		}

		mvprintw(7+tshift,42,"%0.f ",fm);
		mvprintw(8+tshift,42,"%0.f ",fk);

		attrset(COLOR_PAIR(3)|A_BOLD);

		mvprintw(21,22,"Orbit Number: %ld",rv);

		/* Send data to serial port antenna tracker
		   either as needed (when it changes), or
		   once per second. */

		if (sat_ele>=0.0 && antfd!=-1)
		{
			newtime=(long)time(NULL);

			if ((oldel!=iel || oldaz!=iaz) || (once_per_second && newtime>lasttime))
			{
				TrackDataOut(antfd,(float)iel,(float)iaz);
				oldel=iel;
				oldaz=iaz;
				lasttime=newtime;
			}
		}

		mvprintw(23,22,"Spacecraft is currently ");

		if (visibility=='V')
			mvprintw(23,46,"visible    ");

		if (visibility=='D')
			mvprintw(23,46,"in sunlight");

		if (visibility=='N')
			mvprintw(23,46,"in eclipse ");

		attrset(COLOR_PAIR(4)|A_BOLD);
		mvprintw(20,5,"   Sun   ");
		mvprintw(21,5,"---------");
		attrset(COLOR_PAIR(3)|A_BOLD);
		mvprintw(22,5,"%-7.2fAz",sun_azi);
		mvprintw(23,4,"%+-6.2f  El",sun_ele);

		FindMoon(daynum);

		attrset(COLOR_PAIR(4)|A_BOLD);
		mvprintw(20,65,"  Moon  ");
		mvprintw(21,65,"---------");
		attrset(COLOR_PAIR(3)|A_BOLD);
		mvprintw(22,65,"%-7.2fAz",moon_az);
		mvprintw(23,64,"%+-6.2f  El",moon_el);

		if (geostationary==1 && sat_ele>=0.0)
		{
			mvprintw(22,22,"Satellite orbit is geostationary");
			aoslos=-3651.0;
		}

		if (geostationary==1 && sat_ele<0.0)
		{
			mvprintw(22,22,"This satellite never reaches AOS");
			aoslos=-3651.0;
		}

		if (aoshappens==0 || decayed==1)
		{
			mvprintw(22,22,"This satellite never reaches AOS");
			aoslos=-3651.0;
		}

		if (sat_ele>=0.0 && geostationary==0 && decayed==0 && daynum>lostime)
		{
			lostime=FindLOS2();
			mvprintw(22,22,"LOS at: %s UTC  ",Daynum2String(lostime));
			aoslos=lostime;
		}

		else if (sat_ele<0.0 && geostationary==0 && decayed==0 && aoshappens==1 && daynum>aoslos)
		{
			daynum+=0.003;  /* Move ahead slightly... */
			nextaos=FindAOS();
			mvprintw(22,22,"Next AOS: %s UTC",Daynum2String(nextaos));
			aoslos=nextaos;

			if (oldtime!=0.0 && speak=='T' && soundcard)
			{
				/* Announce LOS */

				sprintf(command,"%svocalizer/vocalizer los &",predictpath);
				system(command);
			}
		}

		/* This is where the variables for the socket server are updated. */

		if (socket_flag)
		{
			az_array[indx]=sat_azi;
			el_array[indx]=sat_ele;
			lat_array[indx]=sat_lat;
			long_array[indx]=360.0-sat_lon;
			footprint_array[indx]=fk;
			range_array[indx]=sat_range;
			altitude_array[indx]=sat_alt;
			velocity_array[indx]=sat_vel;
			orbitnum_array[indx]=rv;
			doppler[indx]=doppler100;
			nextevent[indx]=aoslos;
			eclipse_depth_array[indx]=eclipse_depth/deg2rad;
			phase_array[indx]=360.0*(phase/twopi);

			if (calc_squint)
				squint_array[indx]=squint;
			else
				squint_array[indx]=360.0;

			FindSun(daynum);

			sprintf(tracking_mode, "%s\n%c",sat[indx].name,0);
		}

		/* Get input from keyboard */

		ans=tolower(getch());

		/* We can force PREDICT to speak by pressing 'T' */

		if (ans=='t')
			oldtime=0.0;

		/* If we receive a RELOAD_TLE command through the
		   socket connection or an 'r' through the keyboard,
		   reload the TLE file.  */

		if (reload_tle || ans=='r')
		{
			ReadDataFiles();
			reload_tle=0;
		}

		if (comsat)
		{
			if (ans==' ' && sat_db[x].transponders>1)
			{
				xponder++;

				if (xponder>=sat_db[x].transponders)
					xponder=0;

				move(10,1);
				clrtoeol();

				downlink_start=sat_db[x].downlink_start[xponder];
				downlink_end=sat_db[x].downlink_end[xponder];
				uplink_start=sat_db[x].uplink_start[xponder];
				uplink_end=sat_db[x].uplink_end[xponder];

				if (downlink_start>downlink_end)
					polarity=-1;

				if (downlink_start<downlink_end)
					polarity=1;

				if (downlink_start==downlink_end)
					polarity=0;

				downlink=0.5*(downlink_start+downlink_end);
				uplink=0.5*(uplink_start+uplink_end);
			}

			if (ans=='>' || ans=='.')
			{
				if (ans=='>')
					shift=0.001;  /* 1 kHz */
				else
					shift=0.0001; /* 100 Hz */

				/* Raise uplink frequency */

				uplink+=shift*(double)abs(polarity);
				downlink=downlink+(shift*(double)polarity);

				if (uplink>uplink_end)
				{
					uplink=uplink_start;
					downlink=downlink_start;
				}
			}

			if (ans=='<' || ans== ',')
			{
				if (ans=='<')
					shift=0.001;  /* 1 kHz */
				else
					shift=0.0001; /* 100 Hz */

				/* Lower uplink frequency */

				uplink-=shift*(double)abs(polarity);
				downlink=downlink-(shift*(double)polarity);

				if (uplink<uplink_start)
				{
					uplink=uplink_end;
					downlink=downlink_end;
				}
			}

			length=strlen(sat_db[x].transponder_name[xponder])/2;
			mvprintw(10,40-length,"%s",sat_db[x].transponder_name[xponder]);
		}

		refresh();

		halfdelay(2);

	} while (ans!='q' && ans!=27);

	cbreak();
	sprintf(tracking_mode, "NONE\n%c",0);
}

void MultiTrack()
{
	/* This function tracks all satellites in the program's
	   database simultaneously until 'Q' or ESC is pressed.
	   Satellites in range are HIGHLIGHTED.  Coordinates
	   for the Sun and Moon are also displayed. */

	int		x, y, z, ans;

	unsigned char	satindex[24], inrange[24], sunstat=0, ok2predict[24];

	double		aos[24], aos2[24], temptime,
			nextcalctime=0.0, los[24], aoslos[24];

	if (xterm)
		fprintf(stderr,"\033]0;PREDICT: Multi-Satellite Tracking Mode\007");

	curs_set(0);
	attrset(COLOR_PAIR(6)|A_REVERSE|A_BOLD);
	clear();

	printw("                                                                                ");
	printw("                     PREDICT Real-Time Multi-Tracking Mode                      ");
	printw("                    Current Date/Time:                                          ");
	printw("                                                                                ");

	attrset(COLOR_PAIR(2)|A_REVERSE);

	printw(" Satellite  Az   El %s  %s  Range  | Satellite  Az   El %s  %s  Range   ",(io_lat=='N'?"LatN":"LatS"),(io_lon=='W'?"LonW":"LonE"),(io_lat=='N'?"LatN":"LatS"),(io_lon=='W'?"LonW":"LonE"));

	for (x=0; x<24; x++)
	{
		if (Geostationary(x)==0 && AosHappens(x)==1 && Decayed(x,0.0)!=1)
			ok2predict[x]=1;
		else
			ok2predict[x]=0;

		aoslos[x]=0.0;
		los[x]=0.0;
		aos[x]=0.0;
		aos2[x]=0.0;
	}

	do
	{
		for (z=0; z<24; z++)
		{
			y=z/2;

			if (z%2)
			{
				indx=y+12;
				x=41;
			}

			else
			{
				indx=y;
				x=1;
			}

			if (sat[indx].meanmo!=0.0 && Decayed(indx,0.0)!=1)
			{
				daynum=CurrentDaynum();
				PreCalc(indx);
				Calc();

				if (sat_ele>=0.0)
				{
					attrset(COLOR_PAIR(2)|A_BOLD);
					inrange[indx]=1;
				}

				else
				{
					attrset(COLOR_PAIR(2));
					inrange[indx]=0;
				}

				if (sat_sun_status)
				{
					if (sun_ele<=-12.0 && sat_ele>=0.0)
						sunstat='V';
					else
						sunstat='D';
				}

				else
					sunstat='N';

				mvprintw(y+6,x,"%-10s%3.0f  %+3.0f  %3.0f   %3.0f %6.0f %c", Abbreviate(sat[indx].name,9),sat_azi,sat_ele,(io_lat=='N'?+1:-1)*sat_lat,(io_lon=='W'?360.0-sat_lon:sat_lon),sat_range,sunstat);

				if (socket_flag)
				{
					az_array[indx]=sat_azi;
					el_array[indx]=sat_ele;
					lat_array[indx]=sat_lat;
					long_array[indx]=360.0-sat_lon;
					footprint_array[indx]=fk;
					range_array[indx]=sat_range;
					altitude_array[indx]=sat_alt;
					velocity_array[indx]=sat_vel;
					orbitnum_array[indx]=rv;
					visibility_array[indx]=sunstat;
					eclipse_depth_array[indx]=eclipse_depth/deg2rad;
					phase_array[indx]=360.0*(phase/twopi);

					doppler[indx]=-100e06*((sat_range_rate*1000.0)/299792458.0);

					if (calc_squint)
						squint_array[indx]=squint;
					else
						squint_array[indx]=360.0;

					FindSun(daynum);
					sprintf(tracking_mode,"MULTI\n%c",0);
				}

				attrset(COLOR_PAIR(4)|A_BOLD);
				mvprintw(20,5,"   Sun   ");
				mvprintw(21,5,"---------");
				attrset(COLOR_PAIR(3)|A_BOLD);
				mvprintw(22,5,"%-7.2fAz",sun_azi);
				mvprintw(23,4,"%+-6.2f  El",sun_ele);

				FindMoon(daynum);

				attrset(COLOR_PAIR(4)|A_BOLD);
				mvprintw(20,65,"  Moon  ");
				mvprintw(21,65,"---------");
				attrset(COLOR_PAIR(3)|A_BOLD);
				mvprintw(22,65,"%-7.2fAz",moon_az);
				mvprintw(23,64,"%+-6.2f  El",moon_el);

				/* Calculate Next Event (AOS/LOS) Times */

				if (ok2predict[indx] && daynum>los[indx] && inrange[indx])
					los[indx]=FindLOS2();

				if (ok2predict[indx] && daynum>aos[indx])
				{
					if (inrange[indx])
						aos[indx]=NextAOS();
					else
						aos[indx]=FindAOS();
				}

				if (inrange[indx])
					aoslos[indx]=los[indx];
				else
					aoslos[indx]=aos[indx];

				if (socket_flag)
				{
					if (ok2predict[indx])
						nextevent[indx]=aoslos[indx];

					else
						nextevent[indx]=-3651.0;
				}

				aos2[indx]=aos[indx];
				satindex[indx]=indx;
			}

			if (Decayed(indx,0.0))
			{
				attrset(COLOR_PAIR(2));
				mvprintw(y+6,x,"%-10s---------- Decayed ---------", Abbreviate(sat[indx].name,9));

				if (socket_flag)
				{
					az_array[indx]=0.0;
					el_array[indx]=0.0;
					lat_array[indx]=0.0;
					long_array[indx]=0.0;
					footprint_array[indx]=0.0;
					range_array[indx]=0.0;
					altitude_array[indx]=0.0;
					velocity_array[indx]=0.0;
					orbitnum_array[indx]=0L;
					visibility_array[indx]='N';
					eclipse_depth_array[indx]=0.0;
					phase_array[indx]=0.0;
					doppler[indx]=0.0;
					squint_array[indx]=0.0;
					nextevent[indx]=-3651.0;
				}
			}
 		}

		attrset(COLOR_PAIR(6)|A_REVERSE|A_BOLD);

		daynum=CurrentDaynum();
		mvprintw(2,39,"%s",Daynum2String(daynum));

		if (daynum>nextcalctime)
		{
			/* Bubble sort the AOS times */

			for (z=22; z>=0; z--)
				for (y=0; y<=z; y++)
					if (aos2[y]>=aos2[y+1])
					{
						temptime=aos2[y];
						aos2[y]=aos2[y+1];
						aos2[y+1]=temptime;

						x=satindex[y];
						satindex[y]=satindex[y+1];
						satindex[y+1]=x;
					}

			/* Display list of upcoming passes */

			attrset(COLOR_PAIR(4)|A_BOLD);
			mvprintw(19,31,"Upcoming Passes");
			mvprintw(20,31,"---------------");
			attrset(COLOR_PAIR(3)|A_BOLD);

			for (x=0, y=0, z=-1; x<21 && y!=3; x++)
			{
				if (ok2predict[satindex[x]] && aos2[x]!=0.0)
				{
					mvprintw(y+21,19,"%10s on %s UTC",Abbreviate(sat[(int)satindex[x]].name,9),Daynum2String(aos2[x]));

					if (z==-1)
						z=x;
					y++;
				}
			}

			if (z!=-1)
				nextcalctime=aos2[z];
		}

		refresh();
		halfdelay(2);  /* Increase if CPU load is too high */
		ans=tolower(getch());

		/* If we receive a RELOAD_TLE command through the
		   socket connection, or an 'r' through the keyboard,
		   reload the TLE file.  */

		if (reload_tle || ans=='r')
		{
			ReadDataFiles();
			reload_tle=0;
			nextcalctime=0.0;
		}

	} while (ans!='q' && ans!=27);

	cbreak();
	sprintf(tracking_mode, "NONE\n%c",0);
}

void Illumination()
{
	double startday, oneminute, sunpercent;
	int eclipses, minutes, quit, breakout=0;
	char string1[365], string[725], datestring[25], count;

	oneminute=1.0/(24.0*60.0);

	PreCalc(indx);
	daynum=floor(GetStartTime(0));
	startday=daynum;
	count=0;

	curs_set(0);
	clear();

	if (xterm)
		fprintf(stderr,"\033]0;PREDICT: %s's Solar Illumination Calendar For %s\007",qth.callsign, sat[indx].name);

	do
	{
		attrset(COLOR_PAIR(4));
		mvprintw(23,6,"                 Calculating... Press [ESC] To Quit");
		refresh();

		count++;
		daynum=startday;

		for (minutes=0, eclipses=0; minutes<1440; minutes++)
		{
			Calc();

			if (sat_sun_status==0)
				eclipses++;

			daynum=startday+(oneminute*(double)minutes);
		}

		sunpercent=((double)eclipses)/((double)minutes);
		sunpercent=100.0-(sunpercent*100.0);

		strcpy(datestring,Daynum2String(startday));
		datestring[11]=0;
		sprintf(string1,"      %s    %4d    %6.2f%c",datestring,1440-eclipses,sunpercent,37);

		/* Allow a quick way out */

		nodelay(stdscr,TRUE);

		if (getch()==27)
			breakout=1;

		nodelay(stdscr,FALSE);

		startday+=18.0;

		daynum=startday;

		for (minutes=0, eclipses=0; minutes<1440; minutes++)
		{
			Calc();

			if (sat_sun_status==0)
				eclipses++;

			daynum=startday+(oneminute*(double)minutes);
		}

		sunpercent=((double)eclipses)/((double)minutes);
		sunpercent=100.0-(sunpercent*100.0);

		strcpy(datestring,Daynum2String(startday));
		datestring[11]=0;
		sprintf(string,"%s\t %s    %4d    %6.2f%c\n",string1,datestring,1440-eclipses,sunpercent,37);
		quit=Print(string,'s');

		/* Allow a quick way out */

		nodelay(stdscr,TRUE);

		if (getch()==27)
			breakout=1;

		nodelay(stdscr,FALSE);

		if (count<18)
			startday-=17.0;
		else
		{
			count=0;
			startday+=1.0;
		}
	}
	while (quit!=1 && breakout!=1 && Decayed(indx,daynum)==0);
}

void MainMenu()
{
	/* Start-up menu.  Your wish is my command. :-) */

	Banner();
	attrset(COLOR_PAIR(4)|A_BOLD);
	mvprintw(10,28,"--==[ Main Menu ]==--");

	attrset(COLOR_PAIR(3)|A_BOLD);
	mvprintw(13,1,"[P]: Predict Satellite Passes");
	mvprintw(14,1,"[V]: Predict Visible Passes");
	mvprintw(15,1,"[S]: Solar Illumination Predictions");
	mvprintw(16,1,"[L]: Lunar Predictions");
	mvprintw(17,1,"[O]: Solar Predictions");
	mvprintw(18,1,"[T]: Single Satellite Tracking Mode");
	mvprintw(19,1,"[M]: Multi-Satellite Tracking Mode");

	mvprintw(13,40,"[I]: Program Information");
	mvprintw(14,40,"[G]: Edit Ground Station Information");
	mvprintw(15,40,"[D]: Display Satellite Orbital Data");
	mvprintw(16,40,"[U]: Update Sat Elements From File");
	mvprintw(17,40,"[E]: Manually Edit Orbital Elements");
	mvprintw(18,40,"[B]: Edit Transponder Database");
	mvprintw(19,40,"[Q]: Exit PREDICT");

	if (socket_flag)
	{
		attrset(COLOR_PAIR(4)|A_BOLD);
		mvprintw(22,33,"Server Mode");
	}

	refresh();

	if (xterm)
		fprintf(stderr,"\033]0;PREDICT: Version %s\007",version); 
}

void ProgramInfo()
{
	Banner();
	attrset(COLOR_PAIR(3)|A_BOLD);

	printw("\n\n\n\n\n\t\tPREDICT version : %s\n",version);
	printw("\t\tQTH file loaded : %s\n",qthfile);
	printw("\t\tTLE file loaded : %s\n",tlefile);
	printw("\t\tDatabase file   : ");

	if (database)
		printw("Loaded\n");
	else
		printw("Not loaded\n");

	if (antfd!=-1)
	{
		printw("\t\tAutoTracking    : Sending data to %s",serial_port);

		if (once_per_second)
			printw(" every second");

		printw("\n");
	}

	else
		printw("\t\tAutoTracking    : Not enabled\n");

	printw("\t\tRunning Mode    : ");

	if (socket_flag)
		printw("Network server on port \"%s\"\n",netport);
	else
		printw("Standalone\n");

	printw("\t\tVocalizer       : ");

	if (soundcard)
		printw("Soundcard present");
	else
		printw("No soundcard available");

	refresh();
	attrset(COLOR_PAIR(4)|A_BOLD);
	AnyKey();
}

void NewUser()
{
	int *mkdir();

	Banner();
	attrset(COLOR_PAIR(3)|A_BOLD);

	mvprintw(12,2,"WELCOME to PREDICT!  Since you are a new user to the program, default\n");
	printw("  orbital data and ground station location information was copied into\n");
	printw("  your home directory to get you going.  Please select option [G] from\n");
	printw("  PREDICT's main menu to edit your ground station information, and update\n");
	printw("  your orbital database using option [U] or [E].  Enjoy the program!  :-)");
	refresh();

	/* Make "~/.predict" subdirectory */

	sprintf(temp,"%s/.predict",getenv("HOME"));
	mkdir(temp,0777);

	/* Copy default files into ~/.predict directory */

	sprintf(temp,"%sdefault/predict.tle",predictpath);

	CopyFile(temp,tlefile);

	sprintf(temp,"%sdefault/predict.db",predictpath);

	CopyFile(temp,dbfile);

	sprintf(temp,"%sdefault/predict.qth",predictpath);

	CopyFile(temp,qthfile);

	attrset(COLOR_PAIR(4)|A_BOLD);
	AnyKey();
}

void db_edit()
{
	clear();
	attrset(COLOR_PAIR(3)|A_BOLD);
	mvprintw(2,15,"* PREDICT Transponder Database Editing Utility *");
	attrset(COLOR_PAIR(2)|A_BOLD);
	mvprintw(13,33,"Coming Soon!");
	attrset(COLOR_PAIR(4)|A_BOLD);
	refresh();
	AnyKey();
}

int QuickFind(string, outputfile)
char *string, *outputfile;
{
	int x, y, z, step=1;
	long start, now, end, count;
	char satname[50], startstr[20], endstr[20];
	time_t t;
	FILE *fd;

	if (outputfile[0])
		fd=fopen(outputfile,"w");
	else
		fd=stdout;

	startstr[0]=0;
	endstr[0]=0;

	ReadDataFiles();

	for (x=0; x<48 && string[x]!=0 && string[x]!='\n'; x++)
		satname[x]=string[x];

	satname[x]=0;
	x++;

	for (y=0; string[x+y]!=0 && string[x+y]!='\n'; y++)
		startstr[y]=string[x+y];

	startstr[y]=0;
	y++;

	for (z=0; string[x+y+z]!=0 && string[x+y+z]!='\n'; z++)
		endstr[z]=string[x+y+z];

	endstr[z]=0;
 
	/* Do a simple search for the matching satellite name */

	for (z=0; z<24; z++)
	{
		if ((strcmp(sat[z].name,satname)==0) || (atol(satname)==sat[z].catnum))
		{
			start=atol(startstr);

			if (endstr[strlen(endstr)-1]=='m')
			{
				step=60;
				endstr[strlen(endstr)-1]=0;
			}
			
			if (endstr[0]=='+')
				end=start+((long)step)*atol(endstr);
			else
				end=atol(endstr);

			indx=z;

			t=time(NULL);
			now=(long)t;

			if (start==0)
				start=now;

			if (startstr[0]=='+')
			{
				start=now;

				if (startstr[strlen(startstr)-1]=='m')
				{
					step=60;
					startstr[strlen(startstr)-1]=0;
				}

				end=start+((long)step)*atol(startstr);

				/* Prevent a list greater than
				   24 hours from being produced */

				if ((end-start)>86400)
				{
					start=now;
					end=now-1;
				}
			}

			if ((start>=now-31557600) && (start<=now+31557600) && end==0)
			{
				/* Start must be one year from now */
				/* Display a single position */
				daynum=((start/86400.0)-3651.0);
				PreCalc(indx);
				Calc();

				if (Decayed(indx,daynum)==0)
					fprintf(fd,"%ld %s %4d %4d %4d %4d %4d %6ld %6ld %c\n",start,Daynum2String(daynum),iel,iaz,ma256,isplat,isplong,irk,rv,findsun);
				break;
			}

			else
			{
				/* Display a whole list */
				for (count=start; count<=end; count+=step)
				{
					daynum=((count/86400.0)-3651.0);
					PreCalc(indx);
					Calc();

					if (Decayed(indx,daynum)==0)
						fprintf(fd,"%ld %s %4d %4d %4d %4d %4d %6ld %6ld %c\n",count,Daynum2String(daynum),iel,iaz,ma256,isplat,isplong,irk,rv,findsun);
				}
				break;
			}
		}
	}

	if (outputfile[0])
		fclose(fd);

	return 0;
}

int QuickPredict(string, outputfile)
char *string, *outputfile;
{
	int x, y, z, lastel=0;
	long start, now;
	double doppler100=0.0;
	char satname[50], startstr[20];
	time_t t;
	FILE *fd;

	if (outputfile[0])
		fd=fopen(outputfile,"w");
	else
		fd=stdout;

	startstr[0]=0;

	ReadDataFiles();

	for (x=0; x<48 && string[x]!=0 && string[x]!='\n'; x++)
		satname[x]=string[x];

	satname[x]=0;
	x++;

	for (y=0; string[x+y]!=0 && string[x+y]!='\n'; y++)
		startstr[y]=string[x+y];

	startstr[y]=0;
	y++;

	/* Do a simple search for the matching satellite name */

	for (z=0; z<24; z++)
	{
		if ((strcmp(sat[z].name,satname)==0) || (atol(satname)==sat[z].catnum))
		{
			start=atol(startstr);
			indx=z;

			t=time(NULL);
			now=(long)t;

			if (start==0)
				start=now;

			if ((start>=now-31557600) && (start<=now+31557600))
			{
				/* Start must within one year of now */
				daynum=((start/86400.0)-3651.0);
				PreCalc(indx);
				Calc();

				if (AosHappens(indx) && Geostationary(indx)==0 && Decayed(indx,daynum)==0)
				{
					/* Make Predictions */
					daynum=FindAOS();

					/* Display the pass */

					while (iel>=0)
					{
						fprintf(fd,"%.0f %s %4d %4d %4d %4d %4d %6ld %6ld %c %f\n",floor(86400.0*(3651.0+daynum)),Daynum2String(daynum),iel,iaz,ma256,isplat,isplong,irk,rv,findsun,doppler100);
						lastel=iel;
						daynum+=cos((sat_ele-1.0)*deg2rad)*sqrt(sat_alt)/25000.0;
						Calc();
					}

					if (lastel!=0)
					{
						daynum=FindLOS();
						Calc();
						fprintf(fd,"%.0f %s %4d %4d %4d %4d %4d %6ld %6ld %c %f\n",floor(86400.0*(3651.0+daynum)),Daynum2String(daynum),iel,iaz,ma256,isplat,isplong,irk,rv,findsun,doppler100);
					}
				}
				break;
			}
		}
	}

	if (outputfile[0])
		fclose(fd);

	return 0;
}

int QuickDoppler100(string, outputfile)
char *string, *outputfile;
{

	/* Do a quick predict of the doppler for non-geo sattelites, returns UTC epoch seconds, 
	   UTC time and doppler normalized to 100MHz for every 5 seconds of satellite-pass as a CSV*/

	int x, y, z, lastel=0;
	long start, now;
	double doppler100;
	char satname[50], startstr[20];
	time_t t;
	FILE *fd;

	if (outputfile[0])
		fd=fopen(outputfile,"w");
	else
		fd=stdout;

	startstr[0]=0;

	ReadDataFiles();

	for (x=0; x<48 && string[x]!=0 && string[x]!='\n'; x++)
		satname[x]=string[x];

	satname[x]=0;
	x++;

	for (y=0; string[x+y]!=0 && string[x+y]!='\n'; y++)
		startstr[y]=string[x+y];

	startstr[y]=0;
	y++;

	/* Do a simple search for the matching satellite name */

	for (z=0; z<24; z++)
	{
		if ((strcmp(sat[z].name,satname)==0) || (atol(satname)==sat[z].catnum))
		{
			start=atol(startstr);
			indx=z;

			t=time(NULL);
			now=(long)t;

			if (start==0)
				start=now;

			if ((start>=now-31557600) && (start<=now+31557600))
			{
				/* Start must within one year of now */
				daynum=((start/86400.0)-3651.0);
				PreCalc(indx);
				Calc();

				if (AosHappens(indx) && Geostationary(indx)==0 && Decayed(indx,daynum)==0)
				{
					/* Make Predictions */
					daynum=FindAOS();

					/* Display the pass */

					while (iel>=0)
					{
						doppler100=-100.0e06*((sat_range_rate*1000.0)/299792458.0);
						fprintf(fd,"%.0f,%s,%f\n",floor(86400.0*(3651.0+daynum)),Daynum2String(daynum),doppler100);
						lastel=iel;
						daynum+=cos((sat_ele-1.0)*deg2rad)*sqrt(sat_alt)/500000.0;
						Calc();
					}

					if (lastel!=0)
					{
						doppler100=-100.0e06*((sat_range_rate*1000.0)/299792458.0);
						daynum=FindLOS();
						Calc();
						fprintf(fd,"%.0f,%s,%f\n",floor(86400.0*(3651.0+daynum)),Daynum2String(daynum),doppler100);
					}
				}
				break;
			}
		}
	}

	if (outputfile[0])
		fclose(fd);

	return 0;
}


int main(argc,argv)
char argc, *argv[];
{
	int x, y, z, key=0;
	char updatefile[80], quickfind=0, quickpredict=0,
	     quickstring[40], outputfile[42], quickdoppler100=0,
	     tle_cli[50], qth_cli[50], interactive=0;
	struct termios oldtty, newtty;
	pthread_t thread;
	char *env=NULL;
	FILE *db;

	/* Set up translation table for computing TLE checksums */

	for (x=0; x<=255; val[x]=0, x++);
	for (x='0'; x<='9'; val[x]=x-'0', x++);

	val['-']=1;

	updatefile[0]=0;
	outputfile[0]=0;
	temp[0]=0;
	tle_cli[0]=0;
	qth_cli[0]=0;
	dbfile[0]=0;
	netport[0]=0;
	serial_port[0]=0;
	once_per_second=0;
		
	y=argc-1;
	antfd=-1;

	/* Make sure entire "quickstring" array is initialized before use */

	for (x=0; x<40; quickstring[x]=0, x++);

	/* Scan command-line arguments */

	for (x=1; x<=y; x++)
	{
		if (strcmp(argv[x],"-f")==0)
		{
			quickfind=1;
			z=x+1;
			while (z<=y && argv[z][0] && argv[z][0]!='-')
			{
				if ((strlen(quickstring)+strlen(argv[z]))<37)
				{
					strncat(quickstring,argv[z],15);
					strcat(quickstring,"\n");
					z++;
				}
			}
			z--;
		}

		if (strcmp(argv[x],"-p")==0)
		{
			quickpredict=1;
			z=x+1;

			while (z<=y && argv[z][0] && argv[z][0]!='-')
			{
				if ((strlen(quickstring)+strlen(argv[z]))<37)
				{
					strncat(quickstring,argv[z],15);
					strcat(quickstring,"\n");
					z++;
				}
			}
			z--;
		}

		if (strcmp(argv[x],"-dp")==0)
		{
			quickdoppler100=1;
			z=x+1;

			while (z<=y && argv[z][0] && argv[z][0]!='-')
			{
				if ((strlen(quickstring)+strlen(argv[z]))<37)
				{
					strncat(quickstring,argv[z],15);
					strcat(quickstring,"\n");
					z++;
				}
			}
			z--;
		}

		if (strcmp(argv[x],"-u")==0)
		{
			z=x+1;
			while (z<=y && argv[z][0] && argv[z][0]!='-')
			{
				if ((strlen(updatefile)+strlen(argv[z]))<75)
				{
					strncat(updatefile,argv[z],75);
					strcat(updatefile,"\n");
					z++;
				}
			}
			z--;	
		}


		if (strcmp(argv[x],"-t")==0)
		{
			z=x+1;
			if (z<=y && argv[z][0] && argv[z][0]!='-')
				strncpy(tle_cli,argv[z],48);
		}

		if (strcmp(argv[x],"-q")==0)
		{
			z=x+1;
			if (z<=y && argv[z][0] && argv[z][0]!='-')
				strncpy(qth_cli,argv[z],48);
		}

		if (strcmp(argv[x],"-a")==0)
		{
			z=x+1;
			if (z<=y && argv[z][0] && argv[z][0]!='-')
				strncpy(serial_port,argv[z],13);
		}

		if (strcmp(argv[x],"-a1")==0)
		{
			z=x+1;
			if (z<=y && argv[z][0] && argv[z][0]!='-')
				strncpy(serial_port,argv[z],13);
			once_per_second=1;
		}

		if (strcmp(argv[x],"-o")==0)
		{
			z=x+1;
			if (z<=y && argv[z][0] && argv[z][0]!='-')
				strncpy(outputfile,argv[z],40);
		}

		if (strcmp(argv[x],"-n")==0)
		{
			z=x+1;
			if (z<=y && argv[z][0] && argv[z][0]!='-')
				strncpy(netport,argv[z],5);
		}

		if (strcmp(argv[x],"-s")==0)
			socket_flag=1;

		if (strcmp(argv[x],"-north")==0) /* Default */
			io_lat='N';

		if (strcmp(argv[x],"-south")==0)
			io_lat='S';

		if (strcmp(argv[x],"-west")==0)  /* Default */
			io_lon='W';

		if (strcmp(argv[x],"-east")==0)
			io_lon='E';
	}

	/* We're done scanning command-line arguments */

	/* If no command-line (-t or -q) arguments have been passed
	   to PREDICT, create qth and tle filenames based on the
	   default ($HOME) directory. */

	env=getenv("HOME");

	if (qth_cli[0]==0)
		sprintf(qthfile,"%s/.predict/predict.qth",env);
	else
		/* sprintf(qthfile,"%s%c",qth_cli,0); */
		sprintf(qthfile,"%s",qth_cli);

	if (tle_cli[0]==0)
		sprintf(tlefile,"%s/.predict/predict.tle",env);
	else
		/* sprintf(tlefile,"%s%c",tle_cli,0); */
		sprintf(tlefile,"%s",tle_cli);

	/* Test for interactive/non-interactive mode of operation
	   based on command-line arguments given to PREDICT. */

	if (updatefile[0] || quickfind || quickpredict || quickdoppler100)
		interactive=0;
	else
		interactive=1;

	if (interactive)
	{
		sprintf(dbfile,"%s/.predict/predict.db",env);

		/* If the transponder database file doesn't already
		   exist under $HOME/.predict, and a working environment
		   is available, place a default copy from the PREDICT
		   distribution under $HOME/.predict. */

		db=fopen(dbfile,"r");

		if (db==NULL)
		{
			sprintf(temp,"%sdefault/predict.db",predictpath);
			CopyFile(temp,dbfile);
		}

		else
			fclose(db);
	}

	x=ReadDataFiles();

	if (x>1)  /* TLE file was loaded successfully */
	{
		if (updatefile[0])  /* -u was passed to PREDICT */
		{
			y=0;
			z=0;
			temp[0]=0;

			while (updatefile[y]!=0)
			{
				while (updatefile[y]!='\n' && updatefile[y]!=0 && y<79)
				{
					temp[z]=updatefile[y];
					z++;
					y++;
				}

				temp[z]=0;

				if (temp[0])
				{
					AutoUpdate(temp);
					temp[0]=0;
					z=0;
					y++;
				}
			}

			exit(0);
		}
	}

	if (x==3)  /* Both TLE and QTH files were loaded successfully */
	{
		if (quickfind)  /* -f was passed to PREDICT */
			exit(QuickFind(quickstring,outputfile));

		if (quickpredict)  /* -p was passed to PREDICT */
			exit(QuickPredict(quickstring,outputfile));

		if (quickdoppler100)  /* -dp was passed to PREDICT */
			exit(QuickDoppler100(quickstring,outputfile));
	}

	else
	{
		if (tle_cli[0] || qth_cli[0])
		{
			/* "Houston, we have a problem..." */

			printf("\n%c",7);

			if (x^1)
				printf("*** ERROR!  Your QTH file \"%s\" could not be loaded!\n",qthfile);

			if (x^2)
				printf("*** ERROR!  Your TLE file \"%s\" could not be loaded!\n",tlefile);

			printf("\n");

			exit(-1);
		}
	}

	if (interactive)
	{
		/* We're in interactive mode.  Prepare the screen */

		/* Are we running under an xterm or equivalent? */

		env=getenv("TERM");

		if (env!=NULL && strncmp(env,"xterm",5)==0)
			xterm=1;
		else
			xterm=0; 

		/* Start ncurses */

		initscr();
		start_color();
		cbreak();
		noecho();
		scrollok(stdscr,TRUE);
		curs_set(0);

		init_pair(1,COLOR_WHITE,COLOR_BLACK);
		init_pair(2,COLOR_WHITE,COLOR_BLUE);
		init_pair(3,COLOR_YELLOW,COLOR_BLUE);
		init_pair(4,COLOR_CYAN,COLOR_BLUE);
		init_pair(5,COLOR_WHITE,COLOR_RED);
		init_pair(6,COLOR_RED,COLOR_WHITE);
		init_pair(7,COLOR_CYAN,COLOR_RED);

		if (x<3)
		{
			/* A problem occurred reading the
			   default QTH and TLE files, and
			   no -t or -q options were
			   provided on the command-line. */

			NewUser();
			x=ReadDataFiles();
			QthEdit();
		}
	}

	if (x==3)
	{
		/* Open serial port to send data to
		   the antenna tracker if present. */

		if (serial_port[0]!=0)
		{
			/* Make sure there's no trailing '/' */

			x=strlen(serial_port);

			if (serial_port[x-1]=='/')
				serial_port[x-1]=0;

			antfd=open(serial_port, O_WRONLY|O_NOCTTY);

			if (antfd!=-1)
			{
				/* Set up serial port */

				tcgetattr(antfd, &oldtty);
				memset(&newtty, 0, sizeof(newtty));

				/* 9600 baud, 8-bits, no parity,
				   1-stop bit, no handshaking */

				newtty.c_cflag=B9600|CS8|CLOCAL;
				newtty.c_iflag=IGNPAR;
				newtty.c_oflag=0;
				newtty.c_lflag=0;

				tcflush(antfd, TCIFLUSH);
				tcsetattr(antfd, TCSANOW, &newtty);
			}

			else
			{
				bailout("Unable To Open Antenna Port");
				exit(-1);
			}
		}
	
		/* Socket activated here.  Remember that
		   the socket data is updated only when
		   running in the real-time tracking modes. */

		if (socket_flag)
		{
			pthread_create(&thread,NULL,(void *)socket_server,(void *)argv[0]);
			bkgdset(COLOR_PAIR(3));
			MultiTrack();
		}

		MainMenu();

		do
		{	
			key=getch();

			if (key!='T')
				key=tolower(key);

			switch (key)
			{
				case 'p':
				case 'v':
					Print("",0);
					PrintVisible("");
					indx=Select();

					if (indx!=-1 && sat[indx].meanmo!=0.0 && Decayed(indx,0.0)==0)
						Predict(key);

					MainMenu();
					break;

				case 'l':
					Print("",0);
					PredictMoon();
					MainMenu();
					break;

				case 'o':
					Print("",0);
					PredictSun();
					MainMenu();
					break;

				case 'u':
					AutoUpdate("");
					MainMenu();
					break;

				case 'e':
					KepEdit();
					MainMenu();
					break;

				case 'd':
					ShowOrbitData();
					MainMenu();
					break;

				case 'g':
					QthEdit();
					MainMenu();
					break;

				case 't':
				case 'T':
					indx=Select();

					if (indx!=-1 && sat[indx].meanmo!=0.0 && Decayed(indx,0.0)==0)
						SingleTrack(indx,key);

					MainMenu();
					break;

				case 'm':
					MultiTrack();
					MainMenu();
					break;

				case 'i':
					ProgramInfo();
					MainMenu();
					break;

				case 'b':
					db_edit();
					MainMenu();
					break;

				case 's':
					indx=Select();
					if (indx!=-1 && sat[indx].meanmo!=0.0 && Decayed(indx,0.0)==0)
					{
						Print("",0);
						Illumination();
					}
					MainMenu();
					break;
			}

		} while (key!='q' && key!=27);

		if (antfd!=-1)
		{
			tcsetattr(antfd,TCSANOW,&oldtty);
			close(antfd);
		}

		curs_set(1);	
		bkgdset(COLOR_PAIR(1));
		clear();
		refresh();
		endwin();
	}

	exit(0);
}

