/* Prototypes for the communications functions */

int connectsock(char *host, char *service, char *protocol);
int send_command(int sock, char *command);
int get_response(int sock, char *buf);
gint timeout_callback( gpointer data );
void error_dialog(gchar *message);
void show_status( const gchar * statusmsg );
int connect_server( void );
void clear_map( void );
void draw_grid( void );
void plot_QTH( float qthlat, float qthlong, char *callsign );
void clear_widgets( void );
void disconnect_server( void );
