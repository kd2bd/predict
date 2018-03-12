/** Header file generated with fdesign on Mon Jan 31 23:02:21 2000.**/

#ifndef FD_map_h_
#define FD_map_h_

/** Callbacks, globals and object handlers **/
extern void j(FL_OBJECT *, long);
extern void l(FL_OBJECT *, long);


/**** Forms and Objects ****/
typedef struct {
	FL_FORM *map;
	void *vdata;
	char *cdata;
	long  ldata;
	FL_OBJECT *t;
	FL_OBJECT *g;
	FL_OBJECT *a;
	FL_OBJECT *b;
	FL_OBJECT *c;
	FL_OBJECT *f;
	FL_OBJECT *h;
	FL_OBJECT *k;
	FL_OBJECT *s;
} FD_map;

extern FD_map * create_form_map(void);
FD_map *fdmap;
char servername[32];
char satnum[32];
int oldx,oldy;    
long time_offset;
FL_POINT xa[360];
#endif /* FD_map_h_ */
