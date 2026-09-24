#ifndef SHARED_MEMORY_H
#define SHARED_MEMORY_H

#include "common.h"

Dialog_list* attach_d_list(int creator);    //συνάρτηση που κάνει σύνδεση στη λίστα διαλόγων
void detach_d_list();  //συνάρτηση που κάνει αποσύνδεση από τη λίστα διαλόγων
int destroy_d_list();  //συνάρτηση που καταστρέφει τη λίστα διαλόγων 

Dialog* attach_dialog(int dialog_id, int creator);   //συνάρτηση που κάνει σύνδεση σε ένα διάλογο
void detach_dialog();   //συνάρτηση που κάνει αποσύνδεση από ένα διάλογο
int destroy_dialog(int dialog_id);  //συνάρτηση που καταστρέφει ένα διάλογο

key_t generate_key(int base_value, int id); //συνάρτηση που δημιουργεί ένα μοναδικό κλειδί
void clean_all();   //συνάρτηση που κάνει γενικό καθαρισμό 

#endif