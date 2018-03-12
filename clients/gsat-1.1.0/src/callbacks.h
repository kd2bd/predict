#include <gtk/gtk.h>


gboolean
mainwindow_delete_event                (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data);

gboolean
maparea_configure_event                (GtkWidget       *widget,
                                        GdkEventConfigure *event,
                                        gpointer         user_data);

gboolean
maparea_expose_event                   (GtkWidget       *widget,
                                        GdkEventExpose  *event,
                                        gpointer         user_data);

void
combo_entry1_changed                   (GtkEditable     *editable,
                                        gpointer         user_data);

void
on_cb_connect_clicked                  (GtkButton       *button,
                                        gpointer         user_data);

void
on_cb_disconnect_clicked               (GtkButton       *button,
                                        gpointer         user_data);

void
on_tb_grid_toggled                     (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_tb_utctime_toggled                  (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_bt_preferences_clicked              (GtkButton       *button,
                                        gpointer         user_data);

void
on_bt_prefs_apply_clicked              (GtkButton       *button,
                                        gpointer         user_data);

void
on_bt_prefs_cancel_clicked             (GtkButton       *button,
                                        gpointer         user_data);

void
on_bt_con_connect_clicked              (GtkButton       *button,
                                        gpointer         user_data);

void
on_bt_con_cancel_clicked               (GtkButton       *button,
                                        gpointer         user_data);

gboolean
on_dialog_preferences_delete_event     (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data);

gboolean
on_dialog_connect_delete_event         (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data);

void
on_tb_footpsat_toggled                 (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_tb_footpqth_toggled                 (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_tb_track_toggled                    (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_cb_updoppler_toggled                (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_cb_dwdoppler_toggled                (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_bt_prefs_save_clicked               (GtkButton       *button,
                                        gpointer         user_data);

gboolean
on_dialog_azel_graph_delete_event      (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data);

gboolean
on_azelgraph_expose_event              (GtkWidget       *widget,
                                        GdkEventExpose  *event,
                                        gpointer         user_data);

void
on_tb_autofreq_toggled                 (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_bt_azelgraph_clicked                (GtkButton       *button,
                                        gpointer         user_data);

void
on_bt_dbedit_clicked                   (GtkButton       *button,
                                        gpointer         user_data);

void
on_cb_beacondoppler_toggled            (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_cb_rotor_toggled                    (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_clist_satdb_select_row              (GtkCList        *clist,
                                        gint             row,
                                        gint             column,
                                        GdkEvent        *event,
                                        gpointer         user_data);

void
on_bt_dbadd_clicked                    (GtkButton       *button,
                                        gpointer         user_data);

void
on_bt_dbupdate_clicked                 (GtkButton       *button,
                                        gpointer         user_data);

void
on_bt_dbdelete_clicked                 (GtkButton       *button,
                                        gpointer         user_data);

gboolean
on_dialog_dbedit_delete_event          (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data);

void
on_bt_about_clicked                    (GtkButton       *button,
                                        gpointer         user_data);

void
combo_tx_satname_changed               (GtkEditable     *editable,
                                        gpointer         user_data);

void
combo_tx_mode_changed                  (GtkEditable     *editable,
                                        gpointer         user_data);

gboolean
on_dialog_about_delete_event           (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data);

void
on_bt_about_ok_clicked                 (GtkButton       *button,
                                        gpointer         user_data);
