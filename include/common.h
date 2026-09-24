#ifndef COMMON_H
#define COMMON_H

#include <sys/sem.h>    //βιβλιοθήκη για τη διαχείριση σημαφόρων

//ορισμός σταθερών μεταβλητών
#define max_dialogs 100 //max αριθμός διαλόγων ανά system
#define max_msgs 50 //max αριθμός μηνυμάτων ανά queue
#define max_msg_length 256  //max χαρακτήρες ανά μήνυμα
#define max_participants 50 //max αριθμός συμμετεχόντων ανά διάλογο

//keys για shared memory και semaphores
#define key_list 1234   //key για το shared memory της λίστας διαλόγων 
#define key_sem 5678    //key για semaphores
#define key_dialog 9000 //key για το shared memory των διαλόγων  

//δείκτες semaphore για κάθε διάλογο
#define sem_access_dialog 0 //για πρόσβαση στη δομή Dialog
#define sem_empty_slots 1   //για κενά slots στο msg queue
#define sem_full_slots  2   //για γεμάτα slots στο msg queue

//struct για κάθε μήνυμα
typedef struct{
    int msg_id; //id κάθε μηνύματος
    char cont[max_msg_length];  //πίνακας που κρατάει το περιεχόμενο του κάθε μηνύματος
    int sender_pid; //το pid του αποστολέα
    int readers[max_participants];  //πίνακας με τα pid των αναγνωστών 
    int readers_c;  //πλήθος των αναγνωστών που έχουν διαβάσει το μήνυμα 
    int participants;   //πλήθος συμμετεχόντων όταν στάλθηκε το μήνυμα
    int terminated; //flag για TERMINATE, αν γίνει 1 τότε κάνει ΤΕΡΜΙΝΑΤΕ
}MSG;

//struct ουράς μηνυμάτων
typedef struct{
    MSG msgs[max_msgs]; //πίνακας μηνυμάτων 
    int head;   //δείκτης που δείχνει το πιο νέο μήνυμα
    int tail;   //δείκτης που δείχβει το πιο παλιό μήνυμα
    int count;  //τρέχον αριθμός των μηνυμάτων κάθε φορά στην ουρά
    int next_msg_id;    //επόμενο διαθέσιμο id για νέο μήνυμα
}MSG_queue;

//struct για κάθε διάλογο
typedef struct{
    int dialog_id;  //το id του διαλόγου
    int participants_c; //πλήθος των συμμετεχόντων κάθε φορά 
    int participant_pids[max_participants]; //πίνακας με τα pid των συμμετεχόντων στο διάλογο
    MSG_queue msg_queue;    //ουρά μηνυμάτων του διαλόγου
    int actived;    //flag που δείχνει αν ο διάλογος είναι ενεργός, αν είναι 1 τότε σημαίνει ότι είναι ενεργός
    int sem_id; //το id του semaphore γι αυτό το διάλογο
}Dialog;

//struct λίστας διαλόγων
typedef struct{
    int dialog_ids[max_dialogs];    //πίνακας με τα id των ενεργών διαλόγων 
    int active_c;   //πλήθος ενεργών διαλόγων 
    int sem_id; //το id του semaphore για τη λίστα
}Dialog_list;

//μακροεντολές για την πιο εύκολη διαχείριση των semaphores
//με το SEM_UNDO γίνεται αυτόματη επαναφορά σε περίπτωση τερματισμού κάποιας διεργασίας
#define LOCK(semid) do{ /*αποκλειστική πρόσβαση στο dialog*/ \
    struct sembuf op = {sem_access_dialog, -1, SEM_UNDO}; /*μειώνει τον semaphore κατά 1*/ \
    semop(semid, &op, 1); /*αν ο semaphore είναι 0 τότε η διεργασία μπλοκάρει μέχρι να αυξηθεί*/ \
}while(0)

#define UNLOCK(semid) do{ /*απελευθέρωση της αποκλειστικής πρόσβασης στο dialog */ \
    struct sembuf op = {sem_access_dialog, 1, SEM_UNDO}; /*αυξάνει τον semaphore κατά 1*/ \
    semop(semid, &op, 1); /*αν ο semaphore είναι 0 τότε η διεργασία μπλοκάρει μέχρι να αυξηθεί*/ \
}while(0)

#define WAIT_EMPTY(semid) do{ /*περιμένει για κενό slot στην ουρά των μηνυμάτων*/ \
    struct sembuf op = {sem_empty_slots, -1, SEM_UNDO}; /*μειώνει τον semaphore κατά 1*/ \
    semop(semid, &op, 1); /*αν δεν υπάρχουν κενά slot τότε η διεργασία μπλοκάρει*/ \
}while(0)

#define SIGNAL_EMPTY(semid) do{ /*σηματοδοτεί ότι δημιουργήθηκε ένα κενό slot*/ \
    struct sembuf op = {sem_empty_slots, 1, SEM_UNDO}; /*αυξάνει τον semaphore κατά 1*/ \
    semop(semid, &op, 1); /*αν δεν υπάρχουν κενά slot τότε η διεργασία μπλοκάρει*/ \
}while(0)

#define WAIT_FULL(semid) do{ /*περιμένει για διαθέσιμο μήνυμα στην ουρά μηνυμάτων*/ \
    struct sembuf op = {sem_full_slots, -1, SEM_UNDO}; /*μειώνει τον semaphore κατά 1*/ \
    semop(semid, &op, 1); /*αν δεν υπάρχουν μηνύματα τότε η διεργασία μπλοκάρει*/ \
}while(0)

#define SIGNAL_FULL(semid) do{ /*σηματοδοτεί ότι προστέθηκε ένα νέο μήνυμα στην ουρά μηνυμάτων*/ \
    struct sembuf op = {sem_full_slots, 1, SEM_UNDO}; /*αυξάνει τον semaphore κατά 1*/ \
    semop(semid, &op, 1); /*αν δεν υπάρχουν μηνύματα τότε η διεργασία μπλοκάρει*/ \
}while(0)

#endif