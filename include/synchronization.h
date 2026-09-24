#ifndef SYNCHRONIZATION_H
#define SYNCHRONIZATION_H

#include "common.h"

int create_d_sem(int dialog_id);   //συνάρτηση που δημιουργεί semaphore για ένα διάλογο
int get_d_sem(int dialog_id);  //συνάρτηση που κρατάει τα id των semaphores ενός διαλόγου
void destroy_d_sem(int dialog_id); //συνάρτηση που καταστρέφει τα semaphores ενός διαλόγου

void d_lock(int semid);    //συνάρτηση που κλειδώνει τη πρόσβαση σε ένα διάλογο
void d_unlock(int semid);  //συνάρτηση που ξελειδώνει τη πρόσβαση σε ένα διάλογο

void wait_for_space(int semid); //συνάρτηση που κάνει αναμονή για ένα κενό slot για τον αποστολέα
void signal_new_space(int semid);   //συνάρτηση που στέλνει ειδοποιηση για ένα νέο κενό slot για το παραλήπτη 
void msg_wait(int semid);   //συνάρτηση που κάνει αναμονή για νέο διαθέσιμο μήνυμα για το παραλήπτη 
void new_msg_signal(int semid); //συνάρτηση που στέλνει ειδοποίηση για ένε νέο μήνυμα από τον αποστολέα

#endif