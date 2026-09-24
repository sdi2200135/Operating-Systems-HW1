#include <stdio.h>
#include <stdlib.h>
#include <sys/sem.h>
#include <sys/ipc.h>
#include <sys/types.h>
#include <errno.h>

#include "synchronization.h"
#include "common.h"

int create_d_sem(int dialog_id){    //συνάρτηση που δημιουργεί semaphore για ένα διάλογο
    key_t key = IPC_PRIVATE + key_sem + dialog_id;
    int semid = semget(key, 3, IPC_CREAT | 0666);   //δημιουργεί 3 semaphores
    
    if(semid != -1){    //αρχικοποιεί τους 3 semaphores 
        semctl(semid, sem_access_dialog, SETVAL, 1);    //για πρόσβαση, αρχική τιμή 1
        semctl(semid, sem_empty_slots, SETVAL, max_msgs);   //για κενά slots, αρχική τιμή κενά όλα
        semctl(semid, sem_full_slots, SETVAL, 0);   //για γεμάτα slots, αρχική τιμή 0
    }
    
    return semid;   //επιστρέφει το id του semaphore
}

int get_d_sem(int dialog_id){   //συνάρτηση που κρατάει τα id των semaphores ενός διαλόγου
    key_t key = IPC_PRIVATE + key_sem + dialog_id;  //παίρνει το ίδιο key με αυτό που δημιουργείται αρχικά
    int semid = semget(key, 3, 0666);   //κάνει πρόσβαση χωρίς δημιουργία (IPC_CREAT)
    
    return semid;   //επιτρέφει το id του semaphore
}

void destroy_d_sem(int dialog_id){  //συνάρτηση που καταστρέφει τα semaphores ενός διαλόγου
    int semid = get_d_sem(dialog_id);  //παίρνει το id του semaphore του διαλόγου 
    
    if(semid != -1){    //διαγράφει το semaphore set, τα 3 semaphores που έιχε δημιουργήσει στην αρχή
        semctl(semid, 0, IPC_RMID);
    }
}

void d_lock(int semid){ //συνάρτηση που κλειδώνει τη πρόσβαση σε ένα διάλογο
    if(semid != -1){    //αν υπάρχει το id του semaphore τότε κλειδώνει τη πρόσβαση στο διάλογο
        LOCK(semid);
    }
}

void d_unlock(int semid){   //συνάρτηση που ξελειδώνει τη πρόσβαση σε ένα διάλογο
    if(semid != -1){    //αν υπάρχει το id του semaphore τότε ξεκλειδώνει τη πρόσβαση στο διάλογο
        UNLOCK(semid);
    }
}

void wait_for_space(int semid){ //συνάρτηση που κάνει αναμονή για ένα κενό slot για τον αποστολέα
    if(semid != -1){    //αν υπάρχει το id του semaphore τότε μειώνει τα κενά slot
        WAIT_EMPTY(semid);
    }
}

void signal_new_space(int semid){   //συνάρτηση που στέλνει ειδοποιηση για ένα νέο κενό slot για το παραλήπτη
    if(semid != -1){    //αν υπάρχει το id του semaphore τότε αυξάνει τα κενά slot
        SIGNAL_EMPTY(semid);
    }
}

void msg_wait(int semid){   //συνάρτηση που κάνει αναμονή για νέο διαθέσιμο μήνυμα για το παραλήπτη
    if(semid != -1){    //αν υπάρχει το id του semaphore τότε μειώνει τα γεμάτα slot
        WAIT_FULL(semid);
    }
}

void new_msg_signal(int semid){ //συνάρτηση που στέλνει ειδοποίηση για ένε νέο μήνυμα από τον αποστολέα
    if(semid != -1){    //αν υπάρχει το id του semaphore τότε αυτξάνει τα γεμάτα slot
        SIGNAL_FULL(semid);
    }
}