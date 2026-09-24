#include <stdio.h>
#include <stdlib.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

#include "shared_memory.h"
#include "synchronization.h"

//μεταβλητές για τη διαχείριση του shared memory
static Dialog_list* glb_d_list = NULL;  //δείκτης στη λίστα των διαλόγων
static Dialog* glb_dialog = NULL;   //δείκτης στο τρέχοντα διάλογο
static int curr_dialog_id = -1; //id του τρέχοντος διαλόγου
static int sh_mem_id_list = -1; //id του shared memory για τη λίστα διαλόγων
static int sh_mem_id_dialog = -1;   //id του shared memory για το τρέχοντα διάλογο

//flags 
static int is_list = 0; //είναι 1 αν έχουμε δημιουργήσει τη λίστα διαλόγων
static int is_dialog = 0;   //είναι 1 αν έχουμε δημουργήσει το διάλογο

key_t generate_key(int base_value, int id){ //συνάρτηση που δημιουργεί ένα μοναδικό κλειδί
    return IPC_PRIVATE + base_value + id;   //με το IPC_PRIVATE εξασφαλίζεται η μοναδικότητα
}

Dialog_list* attach_d_list(int creator){ //συνάρτηση που κάνει σύνδεση στη λίστα διαλόγων
    is_list = creator;  //αποθηκεύεται η τιμή του creator 
    key_t key = generate_key(key_list, 0);  //δημιουργεί κλειδί για τη λίστα διαλόγων
    int flags = 0666;   //δικαίωμα read και write
    
    if(creator){    //ανάλογα με τη τιμή του creator δημιουργεί ένα νέο αντικείμενο 
        flags |= IPC_CREAT | IPC_EXCL;
    }

    sh_mem_id_list = shmget(key, sizeof(Dialog_list), flags);   //πρόσβαση στο shared memory
    if(sh_mem_id_list == -1){
        if(creator){
            sh_mem_id_list = shmget(key, sizeof(Dialog_list), IPC_CREAT | 0666); //αν αποτύχει η δημιουργία με το IPC_EXCL δοκιμάζει και χωρίς
            if(sh_mem_id_list == -1){
                return NULL;    //αποτυχία δημιουργίας id
            }
        } 
        else{
            return NULL;    //αποτυχία δημιουργίας πρόσβασης 
        }
    }

    glb_d_list = (Dialog_list*)shmat(sh_mem_id_list, NULL, 0);  //σύνδεση στο shared memory
    if(glb_d_list == (void*)-1){    //αποτυχία σύνδεσης 
        sh_mem_id_list = -1;
        return NULL;
    }

    if(creator && sh_mem_id_list != -1){    //αν το creator ειναι 1(δημιουργείται το σύστημα), αρχικοποιούμε τη δομή της λίστας διαλόγων 
        glb_d_list->active_c = 0;   //κανένας ενεργός διάλογος στην αρχή
        
        key_t sem_key = generate_key(key_sem, 0);   
        int semid = semget(sem_key, 1, IPC_CREAT | 0666);   //δημιουργία semaphore για το συγχρονισμό στη λίστα διαλόγων
        if(semid != -1){    //αν δημιουργηθεί με επιτυχία
            glb_d_list->sem_id = semid; //αποθηκεύουμε το id του semaphore
            semctl(semid, 0, SETVAL, 1);    //αρχικοποιούμε με τιμή 1, για να δηλώσουμε ότι είναι ελεύθερο
        } 
        else{   //αποτυχία δημιουργίας semaphore
            glb_d_list->sem_id = -1;    
        }
    }

    return glb_d_list;  //επιστρέφουμε δείκτη στη λίστα διαλόγων
}

void detach_d_list(){  //συνάρτηση που κάνει αποσύνδεση από τη λίστα διαλόγων
    if(glb_d_list && glb_d_list != (void*)-1){  //αν υπάρχει το global list 
        shmdt(glb_d_list);  //αποσύνδεση από το segment
        glb_d_list = NULL;  //μηδενισμός δείκτη
    }
}

int destroy_d_list(){  //συνάρτηση που καταστρέφει τη λίστα διαλόγων 
    if(sh_mem_id_list != -1 && is_list){    //αν έχει δημιουργηθεί η λίστα διαλόγων και υπάρχει id του shared memory για τη λίστα διαλόγων
        detach_d_list();   //κ΄άνουμε αποσύνδεση από το σύστημα
        
        if(shmctl(sh_mem_id_list, IPC_RMID, NULL) == -1){   //διαγράφουμε το τμήμα του shared memory 
            return -1;
        }
        
        sh_mem_id_list = -1;    //ενημερώνουμε τo id του shared memory για τη λίστα διαλόγων 
        
        key_t sem_key = generate_key(key_sem, 0);
        int semid = semget(sem_key, 0, 0666);
        if(semid != -1){    //διαγράφουμε το semaphore της λίστας  
            semctl(semid, 0, IPC_RMID);
        }
        
        return 0;   //ειστρέφουμε 0 για να δηλώσουμε επιτυχία της διαδικασίας
    }
    
    return -1;  //ειστρέφουμε -1 για να δηλώσουμε αποτυχία
}

Dialog* attach_dialog(int dialog_id, int creator){   //συνάρτηση που κάνει σύνδεση σε ένα διάλογο
    is_dialog = creator;    //κρατάμε αν ο διάλογος δηιουργείται τώρα ή όχι αναλογα με τη τιμή του creator
    curr_dialog_id = dialog_id; //κρατάμε το id του διαλόγου
    
    key_t key = generate_key(key_dialog, dialog_id);    
    int flags = 0666;   //δικαίωμα read και write
    
    if(creator){
        flags |= IPC_CREAT | IPC_EXCL;  //δημιουργία νέου διαλόγου ανάλογα με τη τιμή του creator
    }

    sh_mem_id_dialog = shmget(key, sizeof(Dialog), flags);  //δηιουργία και πρόσβαση στο τμήμα του shared memory για το διάλογο 
    if(sh_mem_id_dialog == -1){ //αν δεν δημιουργήθηκε προσπαθούμε να το δημιουργήσουμε χωρίς να χρησιμοποιούμε το IPC_EXCL
        if(creator){
            sh_mem_id_dialog = shmget(key, sizeof(Dialog), IPC_CREAT | 0666);
            if(sh_mem_id_dialog == -1){
                return NULL;    //αποτυχία δημιουργίας τμήματος shared memory για το διάλογο
            }
        } 
        else{
            return NULL;    //αποτυχία πρόσβασης στο διαλογο
        }
    }

    glb_dialog = (Dialog*)shmat(sh_mem_id_dialog, NULL, 0); //σύνδεση στο shared memory του διαλόγου
    if(glb_dialog == (void*)-1){    //αποτυχία σ΄ύνδεσης
        sh_mem_id_dialog = -1;
        return NULL;   
    }

    if(creator){    //αν δημιουργείται τώρα ο διάλογος αρχικοποιουμε τις τιμές των δομών
        glb_dialog->dialog_id = dialog_id;  //αρχικοποίηση δομής Dialog 
        glb_dialog->actived = 1;
        glb_dialog->participants_c = 0;
        glb_dialog->sem_id = -1;
        
        for(int i = 0; i < max_participants; i++){
            glb_dialog->participant_pids[i] = 0;
        }
        
        glb_dialog->msg_queue.head = 0; //αρχικοποιήση δομης MSG_queue μέσω της δομής Dialog
        glb_dialog->msg_queue.tail = 0;
        glb_dialog->msg_queue.count = 0;
        glb_dialog->msg_queue.next_msg_id = 1;
        
        for(int i = 0; i < max_msgs; i++){ //αρχικοποιήση δομης MSG μέσω της δομής MSG_queue
            glb_dialog->msg_queue.msgs[i].msg_id = 0;
            glb_dialog->msg_queue.msgs[i].cont[0] = '\0';
            glb_dialog->msg_queue.msgs[i].sender_pid = 0;
            glb_dialog->msg_queue.msgs[i].readers_c = 0;
            glb_dialog->msg_queue.msgs[i].participants = 0;
            glb_dialog->msg_queue.msgs[i].terminated = 0;
            for(int j = 0; j < max_participants; j++){
                glb_dialog->msg_queue.msgs[i].readers[j] = 0;
            }
        }
        
        // Δημιουργία semaphore για αυτόν τον διάλογο
        key_t sem_key = generate_key(key_sem, dialog_id);
        int sem_id = semget(sem_key, 3, IPC_CREAT | 0666);  //δημιουργία 3 semaphore για το διάλογο
        if(sem_id != -1){   //επιτυχής δημιουργία 
            glb_dialog->sem_id = sem_id;    //αποθηκεύουμε τα id των semaphore
            semctl(sem_id, sem_access_dialog, SETVAL, 1);   //1 για πρόσβαση στο διάλογο
            semctl(sem_id, sem_empty_slots, SETVAL, max_msgs);  //1 για ενημέρωση για τα κενά slot
            semctl(sem_id, sem_full_slots, SETVAL, 0);  //1 για ενημέρωση για τα γεμάτα slot
        } 
        else{   //αποτυχής δημιουργία semaphore
            glb_dialog->sem_id = -1;
        }
    }

    return glb_dialog;  //επιστρέφουμε δείκτη στο τρέχοντα διάλογο
}

void detach_dialog(){   //συνάρτηση που κάνει αποσύνδεση από ένα διάλογο
    if(glb_dialog && glb_dialog != (void*)-1){  //αν υπάρχει ο διάλογος
        shmdt(glb_dialog);  //κάνουμε αποσύνδεση από το segment στο shared memory
        glb_dialog = NULL;  //μηδενίζουμε το δείκτη
        curr_dialog_id = -1;    //κάνουμε επαναφορά του id του τρέχοντα διαλογου
        sh_mem_id_dialog = -1;  //κάνουμε επαναφορά του id του shared memory segment
    }
}

int destroy_dialog(int dialog_id){  //συνάρτηση που καταστρέφει ένα διάλογο
    if(sh_mem_id_dialog != -1 && is_dialog){    //αν έχει δημιουργηθεί o διαλογος και είναι ο τελευταίος συμμετέχον σε αυτό
        if(glb_dialog && glb_dialog->sem_id != -1){ //διαγράφουμε του semaphores του διαλόγου
            semctl(glb_dialog->sem_id, 0, IPC_RMID);
        }
        
        detach_dialog();    //κάνουμε αποσύνδεση από το διάλογο
        
        key_t key = generate_key(key_dialog, dialog_id);
        int shmid = shmget(key, 0, 0666);   
        if(shmid != -1){    //διαγράφουμε το τμημα του shared memory
            shmctl(shmid, IPC_RMID, NULL);
        }
        
        return 0;   //αν επιστραφεί 0 σημαίνει πως διαγράφηκε με επιτυχία 
    }
    
    return -1;  //αν επιστραφεί 0 σημαίνει πως υπήρξε αποτυχία 
}

void clean_all(){   //συνάρτηση που κάνει γενικό καθαρισμό
    detach_dialog();    //αποσύνδεση από το διάλογο
    detach_d_list();    //αποσύνδεση από τη λίστα διαλόγων
}