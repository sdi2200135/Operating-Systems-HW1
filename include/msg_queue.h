#ifndef MSG_QUEUE_H
#define MSG_QUEUE_H

#include "common.h"

int send_msg(Dialog* dialog, int semid, const char* cont, int sender_pid, int terminated); //συνάρτηση που κάνει αποστολή ενός μηνύματος 
int receive_msg(Dialog* dialog, int semid, MSG* output, int receiver_pid);  //συνάρτηση που κάνει λήψη ενός μηνύματος
int unread_msg(Dialog* dialog, int receiver_pid);  //συνάρτηση που κοιτάζει αν υπάρχουν μη διαβασμένα μηνύματα
void read_msg(MSG* msg, int reader_pid);   //συνάρτηση που σηατοδοτεί τα διαβασμένα μηνύματα και προσθέτει το pid του αναγνώστη στο πίνακα
int msg_read_by_all(MSG* msg, Dialog* dialog);  //συνάρτηση που ελέγχει αν κάποιο μήνυμα το έχουν διαβάσει όλοι όσοι είναι να το διαβάσουν
int remove_old_msgs(Dialog* dialog, int semid); //συνάρτηση που αφαιρεί τα παλιά (διαβασμένα από όλους) μηνύματα 

#endif