#ifndef DIALOG_CONTROL_H
#define DIALOG_CONTROL_H

#include "common.h"

int create_new_dialog(Dialog_list* d_list); //συνάρτηση που δημιουργεί νέο διάλογο
int join_dialog(Dialog_list* d_list, int dialog_id);    //συνάρτηση που κάνει σύνδεση σε ήδη υπάρχοντα διάλογο
void leave_dialog(Dialog_list* d_list, int dialog_id, int pid); //συνάρτηση που κ΄άνει αποχώριση από ένα διάλογο
void remove_dialog(Dialog_list* d_list, int dialog_id); //συνάρτηση που αφαιρεί ένα διάλογο από τη λίστα διαλόγων
void list_active_dialogs(Dialog_list* d_list);  //συνάρτηση που εμφανίζει τους ενεργούς διαλόγους

int add_participant(Dialog* dialog, int pid);   //συνάρτηση που προσθέτει ένα συμμετέχοντα σε διάλογο
int remove_participant(Dialog* dialog, int pid);    //συνάρτηση που αφαιρεί ένα συμμετέχοντα από ένα διάλογο
int is_participant(Dialog* dialog, int pid);    //συνάρτηση που ελέγχει αν κάποιο pid συμμετέχει σε ένα διάλογο
void list_participants(Dialog* dialog); //συνάρτηση που εμφανίζει τη λίστα των συμμετεχόντων

#endif