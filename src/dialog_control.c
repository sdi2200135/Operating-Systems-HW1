#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <string.h>

#include "dialog_control.h"
#include "synchronization.h"
#include "shared_memory.h"

int create_new_dialog(Dialog_list* d_list){ //συνάρτηση που δημιουργεί νέο διάλογο
    int new_d_id = -1;
    
    struct sembuf lock_op = {0, -1, 0}; //κλειδώνουμε τη λίστα των διαλόγων
    semop(d_list->sem_id, &lock_op, 1);
    
    if(d_list->active_c >= max_dialogs){    //ελέγχουμε αν έχουμε υπερβεί το όριο διαλόγων που μπορουν να δημιουργηθούν
        struct sembuf unlock_op = {0, 1, 0};    //ξεκλειδώνουμε και επιστρέφουμε αποτυχία (-1)
        semop(d_list->sem_id, &unlock_op, 1);
        return -1;
    }

    srand(time(NULL) ^ getpid());   //παράγουμε τυχαίο id για το νέο διάλογο
    do{ //δημιουργούμε μοναδικό id 
        new_d_id = (rand() % 900) + 100;    //αριθμός μεταξύ του 100 και του 999
        int exists = 0; //flag για να ελέγξουμε αν υπαρχει ηδη το id
        
        for(int i = 0; i < d_list->active_c; i++){  //στο πίνακα με τα id των διαλόγων ελέγχουμε αν υπάρχει αυτό που δημιουργησαμε
            if(d_list->dialog_ids[i] == new_d_id){
                exists = 1;
                break;
            }
        }
        
        if(!exists){    //αν είναι μοναδικό σταματάμε
            break;
        }
    }while (1);

    d_list->dialog_ids[d_list->active_c] = new_d_id;    //προσθέτουμε το νεο id στο πίνακα
    d_list->active_c++; //αυξάνουμε το πλήθος των διαλογων
    
    struct sembuf unlock_op = {0, 1, 0};    //ξεκλειδώνουμε τη πρόσβαση στη λίστα διαλόγων
    semop(d_list->sem_id, &unlock_op, 1);
    
    return new_d_id;    //επιστρέφουμε το id του νεου διαλογου 
}

int join_dialog(Dialog_list* d_list, int dialog_id){    //συνάρτηση που κάνει σύνδεση σε ήδη υπάρχοντα διάλογο
    struct sembuf lock_op = {0, -1, 0}; //κλειδώνουμε τη πρόσβαση στη λίστα διαλογων
    semop(d_list->sem_id, &lock_op, 1);
    
    int exists = 0; //flag για την υπαρξη του διαλογου
    for(int i = 0; i < d_list->active_c; i++){  //αν υπάρχει ο διάλογος σταματαμε
        if(d_list->dialog_ids[i] == dialog_id){
            exists = 1;
            break;
        }
    }
    
    struct sembuf unlock_op = {0, 1, 0};    //ξεκλειδώνουμε τη πρόσβαση στη λίστα διαλογων
    semop(d_list->sem_id, &unlock_op, 1);
    
    if(exists){ //αν υπάρχει ο διάλογος επιστρέφουμε το id του 
        return dialog_id;
    }
    else{   //αλλιως αποτυχία εύρεσης του διαλογου
        return -1;
    }
}

void leave_dialog(Dialog_list* d_list, int dialog_id, int pid){ //συνάρτηση που κ΄άνει αποχώριση από ένα διάλογο
    Dialog* dialog = attach_dialog(dialog_id, 0);   //κάνουμε συνδεση στο διάλογο
    
    if(!dialog){    //αν ο διαλογος έχει ηδη διαγραφεί τότε σταματαμε
        printf("Ο διάλογος %d έχει ήδη διαγραφεί.\n", dialog_id);
        return;
    }
    
    LOCK(dialog->sem_id);   //κλειδωνουμε τη πρόσβαση στο διάλογο
    
    int was_participant = remove_participant(dialog, pid);  //αφαιρούμε το συμμετέχον του διαλόγου
    
    if(was_participant){
        printf("Ο χρήστης %d αποχώρησε από το διάλογο %d. ", pid, dialog_id);
        printf("Υπόλοιποι: %d\n", dialog->participants_c);
    }
    
    if(dialog->participants_c == 0){    //αν ο διάλογος δεν έχει συμμετέχοντες
        UNLOCK(dialog->sem_id); //ξεκλειώνουμε τη πρόσβαση

        struct sembuf lock_op = {0, -1, 0}; 
        semop(d_list->sem_id, &lock_op, 1);
        remove_dialog(d_list, dialog_id);   //αφαιρούμε το διάλογο από τη λίστα διαλόγων
        struct sembuf unlock_op = {0, 1, 0};
        semop(d_list->sem_id, &unlock_op, 1);
        
        destroy_d_sem(dialog_id);   //ο τελευταίος συμμετέχον διαγράφει τους semaphores 
        destroy_dialog(dialog_id);  //και διαγράφει το τμήμα στο shared memory
        
        printf("Ο διάλογος %d διαγράφηκε (καθαρισμός πόρων).\n", dialog_id);
    } 
    else{   //αλλιώς απλά ξεκλειδώνουμε τη πρόσβαση στο διάλογο
        UNLOCK(dialog->sem_id);
    }
    
    detach_dialog();    //κάνουμε αποσύνδεση από το διάλογο
}

void remove_dialog(Dialog_list* d_list, int dialog_id){ //συνάρτηση που αφαιρεί ένα διάλογο από τη λίστα διαλόγων
    for(int i = 0; i < d_list->active_c; i++){  //εντοπίζουμε το διάλογο που θέλουμε να διαγράψουμε
        if(d_list->dialog_ids[i] == dialog_id){
            for(int j = i; j < d_list->active_c - 1; j++){  //μετατοπίζουμε όλα τα στοιχεία του πίνακα κατα μια θέση
                d_list->dialog_ids[j] = d_list->dialog_ids[j + 1];
            }
            d_list->active_c--; //μειώνουμε τους ενεργούς διαλόγους
            d_list->dialog_ids[d_list->active_c] = 0;   //μηδενίζουμε τη τελευταία θέση του πίνακα
            break;
        }
    }
}

void list_active_dialogs(Dialog_list* d_list){  //συνάρτηση που εμφανίζει τους ενεργούς διαλόγους
    struct sembuf lock_op = {0, -1, 0}; //κλείδωμα λίστας διαλόγων
    semop(d_list->sem_id, &lock_op, 1);
    
    printf("\n----- Διαθέσιμοι Διάλογοι -----\n");
    if(d_list->active_c == 0){  //ελέγχουμε αν υπάρχει ενεργός διάλογος
        printf("Κανένας διάλογος.\n");
    } 
    else{
        for(int i = 0; i < d_list->active_c; i++){  //για κάθε ενεργό διάλογο
            Dialog* dialog = attach_dialog(d_list->dialog_ids[i], 0);
            if(dialog){ //αν έχουν συνδεθεί στο διάλογο συμμετέχοντες τους εμφανίζουμε
                printf("ID: %d | Συμμετέχοντες: %d\n", dialog->dialog_id, dialog->participants_c);
                detach_dialog();
            } 
            else{   //αλλιως εμφανίζουμε το αντίστοιχο μηνυμα
                printf("ID: %d | Σφάλμα πρόσβασης\n", d_list->dialog_ids[i]);
            }
        }
    }
    
    struct sembuf unlock_op = {0, 1, 0};    //ξεκλείδωμα λίστας διαλόγων
    semop(d_list->sem_id, &unlock_op, 1);
}

int add_participant(Dialog* dialog, int pid){   //συνάρτηση που προσθέτει ένα συμμετέχοντα σε διάλογο
    for(int i = 0; i < max_participants; i++){
        if(dialog->participant_pids[i] == 0){   //αναζητούμε αν υπάρχει κενή θέση στο πίνακα συμμετεχόντων 
            dialog->participant_pids[i] = pid;  //αν υπάρχει προσθέτουμε το pid
            dialog->participants_c++;   //και αυξάνουμε το πλ΄ηθος των συμμετεχόντων
            return 1;   //επιτυχής ολοκλήρωση προσθήκης συμμετέχοντα 
        }
    }
    return 0;  //αποτυχία, δεν υπάρχει χώρος για να προσθέσουμε συμμετεχοντα
}

int remove_participant(Dialog* dialog, int pid){    //συνάρτηση που αφαιρεί ένα συμμετέχοντα από ένα διάλογο
    for(int i = 0; i < max_participants; i++){
        if(dialog->participant_pids[i] == pid){ //αναζητούμε αν υπάρχει το pid στο πίνακα συμμετεχόντων
            dialog->participant_pids[i] = 0;    //αν υπάρχει μηδενίζουμε τη θέση του
            dialog->participants_c--;   //και μειώνουμε το πλήθος των συμμετεχόντων
            return 1;  //επιτυχής ολοκλήρωση αφαίρεσης συμμετέχοντα
        }
    }
    return 0;  //αποτυχία, δεν βρέθηκε ο συμμετεχοντας
}

int is_participant(Dialog* dialog, int pid){    //συνάρτηση που ελέγχει αν κάποιο pid συμμετέχει σε ένα διάλογο
    for(int i = 0; i < max_participants; i++){
        if(dialog->participant_pids[i] == pid){ //αναζητούμε αν ένα pid συμμετέχει σε ένα διάλογο
            return 1;   //επιτυχής αναζήτηση
        }
    }
    return 0;   //αποτυχία
}

void list_participants(Dialog* dialog){ //συνάρτηση που εμφανίζει τη λίστα των συμμετεχόντων
    printf("Συμμετέχοντες (%d):\n", dialog->participants_c);
    
    int c = 0;  //για τον αυξοντα αριθμό στη λίστα με τους συμμετέχοντες που εμφαν΄΄ιζει η for loop
    for(int i = 0; i < max_participants; i++){
        if(dialog->participant_pids[i] != 0){
            printf("    [%d] PID: %d\n", ++c, dialog->participant_pids[i]);
        }
    }
    
    if(c == 0){ //αν παραμείνει 0 σημαίνει πως δεν υπάρχουν συμμετεχοντες στο διαλογο
        printf("    Κανένας συμμετέχων\n");
    }
}