#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "msg_queue.h"
#include "synchronization.h"

int send_msg(Dialog* dialog, int semid, const char* cont, int sender_pid, int terminated){ //συνάρτηση που κάνει αποστολή ενός μηνύματος
    wait_for_space(semid);  //περιμένει για κενό slot στην ουρά μηνυμάτων
    d_lock(semid);  //κλειδώνει τη πρόσβαση στο διάλογο
    
    if(dialog->msg_queue.count >= max_msgs){    //αν η ουρά είναι γεμάτη, στέλνουμε σήμα για αύξηση των κενών slot
        d_unlock(semid);
        signal_new_space(semid);
        return 0;   //αποτυχία αποστολής μηνύματος
    }
    
    MSG* msg = &dialog->msg_queue.msgs[dialog->msg_queue.head]; //δείκτης στο επόμενο διαθέσιμο μήνυμα σην ουρά 
    
    strncpy(msg->cont, cont, max_msg_length - 1);   //αντιγραφή του περιεχόμενου του μηνύματος
    msg->cont[max_msg_length - 1] = '\0';   //ένδειξη ότι το μήνυμα τελείωσε 
    msg->sender_pid = sender_pid;   //κρατάμε το pid του αποστολέα 
    msg->msg_id = dialog->msg_queue.next_msg_id;    //κρατάμε τη θέση στην ουρά του νέου μηνύματος
    msg->terminated = terminated;   //τροποποιούμε το flag αν το μήνυμα είναι terminate
    msg->readers_c = 0; //μηδενίζουμε τους αναγνώστες (κανένας δεν έχει διαβάσει το μήνυμα) 
    msg->participants = dialog->participants_c; //κρατάμε τον αριθμό των συμμετεχόντων στο διάλογο
    
    for(int i = 0; i < max_participants; i++){  //αρχικοποιούμε το πίνακα με τους αναγνώστες (συμμετέχοντες στο διάλογο)
        msg->readers[i] = 0;
    }
    
    read_msg(msg, sender_pid);  //ο αποστολέας θεωρούμε ότι έχει διαβάσει το μήνυμα που έστειλε
    
    dialog->msg_queue.head = (dialog->msg_queue.head + 1) % max_msgs;   //ενημερ΄νουμε το δείκτη που δείχνει στο πιο πρόσφατο μήνυμα 
    dialog->msg_queue.count++;  //αυξάνουμε το πλήθος των μηνυμάτων στην ουρα
    dialog->msg_queue.next_msg_id++;    //προχωράμε στο επόμενο διαθέσιμο id για νέο μήνυμα
    
    d_unlock(semid);    //ξεκλειδώνουμε τη πρόσβαση στο διάλογο
    new_msg_signal(semid);  //στέλνουμε ειδοποίηση για νέο μήνυμα 
    
    return 1;   //επιτυχία αποστολής μηνύματος
}

int receive_msg(Dialog* dialog, int semid, MSG* output, int receiver_pid){  //συνάρτηση που κάνει λήψη ενός μηνύματος
    d_lock(semid);
    int new_msg = 0;    //flag για να ελέγχουμε αν έγινε επιτυχής λήψη του μηνύματος
    
    if(dialog->msg_queue.count > 0){    //αν υπάρχουν μηνύματα στην ουρά
        for(int i = 0; i < dialog->msg_queue.count; i++){   //διασχίζουμε όλη την ουρά μηνυμάτων
            int idx = (dialog->msg_queue.tail + i) % max_msgs;  //κρατάμε το δείκτη στο πιο παλιό μήνυμα
            MSG* msg = &dialog->msg_queue.msgs[idx];    //παίρνουμε το μήνυμα που δείχνει στη θέση idx
            
            int read = 0;
            for(int j = 0; j < msg->readers_c; j++){    //ελέγχουμε αν οι παραλήπτες έχουν διαβάσει το μήνυμα
                if(msg->readers[j] == receiver_pid){
                    read = 1;   
                    break;
                }
            }
            
            if(!read){  //αν το έχουν διαβάσει 
                memcpy(output, msg, sizeof(MSG));   //γίνεται αντιγραφή του μηνύματος 
                read_msg(msg, receiver_pid);    //το σημειώνουμε σαν διαβασμένο
                new_msg = 1;    //ενημερώνουμε το flag
                
                if(msg_read_by_all(msg, dialog)){   //αν το μήνυμα έχει διαβαστεί από όλους τους παραλήπτες 
                    dialog->msg_queue.tail = (dialog->msg_queue.tail + 1) % max_msgs;   //ενημερώνουμε το δείκτη στο πιο παλιο μήνυμα
                    dialog->msg_queue.count--;  //μει΄ωνουμε το πλήθος μηνυμάτων στην ουρά
                    signal_new_space(semid);    //στέλνουμε σήμα ότι υπάρχει νέο κενό slot στην ουρά μηνυμάτων
                }
                
                break;  //για να σταματήσει η αναζήτηση, αφού βρέθηκε μήνυμα
            }
        }
    }
    
    d_unlock(semid);    //ξεκλειδώνει τη πρόσβαση στο διάλογο
    return new_msg; //επιστρέφει 1 αν έγινε επιτυχής λήψη του μηνυματος
}

int unread_msg(Dialog* dialog, int receiver_pid){   //συνάρτηση που κοιτάζει αν υπάρχουν μη διαβασμένα μηνύματα
    int semid = dialog->sem_id; //κραταμε το id του semaphore για τη λίστα
    d_lock(semid);
    int unread = 0; //flag για τα μηνλυματα που δεν έχουν διαβαστει
    
    for(int i = 0; i < dialog->msg_queue.count; i++){   //διασχίζουμε όλη την ουρα μηνυματων
        int idx = (dialog->msg_queue.tail + i) % max_msgs;  //κρατάμε το δείκτη στο πιο παλιό μήνυμα 
        MSG* msg = &dialog->msg_queue.msgs[idx];    //παίρνουμε το μήνυμα που δείχνει στη θέση idx
        
        int read = 0;
        for(int j = 0; j < msg->readers_c; j++){    //ελέγχουμε αν οι παραλήπτες έχουν διαβάσει το μήνυμα
            if(msg->readers[j] == receiver_pid){
                read = 1;
                break;
            }
        }
        
        if(!read){  //αν το έχουν διαβάσει 
            unread = 1; //ενημερώνουμε το flag
            break;
        }
    }
    
    d_unlock(semid);
    return unread;  //επιστρέφουμε 0 είναι μη διαβασμένο και 1 αν είναι διαβασμένο το μηνυμα
}

void read_msg(MSG* msg, int reader_pid){    //συνάρτηση που σηατοδοτεί τα διαβασμένα μηνύματα και προσθέτει το pid του αναγνώστη στο πίνακα
    if(msg->readers_c < max_participants){  //αν μπορούμε να προσθέσουμε συμμετέχοντα στο διάλογο
        msg->readers[msg->readers_c] = reader_pid;  //προσθέτουμε το pid στο πινακα
        msg->readers_c++;   //αυξάνουμε το πλήθος των αναγνωστών
    }
}

int msg_read_by_all(MSG* msg, Dialog* dialog){ //συνάρτηση που ελέγχει αν κάποιο μήνυμα το έχουν διαβάσει όλοι όσοι είναι να το διαβάσουν
    int curr_participants = 0;  //τρέχοντες συμμετέχοντες 
    int curr_readers = 0;  //τρέχοντες αναγνώστες
    
    for(int i = 0; i < max_participants; i++){  //μετράμε τους συμμετέχοντες στο διάλογο
        if(dialog->participant_pids[i] != 0){
            curr_participants++;
            for(int j = 0; j < msg->readers_c; j++){    //αν ο συμμετέχον έχει διαβάσει το μηνυμα
                if(msg->readers[j] == dialog->participant_pids[i]){
                    curr_readers++; //αυξάνουμε το πλήθος των αναγνωστών
                    break;
                }
            }
        }
    }
    
    if(curr_participants > 0 && curr_readers == curr_participants){ //αν υπάρχουν συμμετέχοντες και όλοι οι συμετέχοντες έχουν διαβάσει το μηνυμα
        return 1;   //επιστρέφει 1 ως επιτυχια
    }
    else{
        return 0;
    }
}

int remove_old_msgs(Dialog* dialog, int semid){    //συνάρτηση που αφαιρεί τα παλιά (διαβασμένα από όλους) μηνύματα 
    d_lock(semid);
    int removed = 0;    //μετράμε το πλήθος των διαγραμμένων μηνυμάτων
    
    while(dialog->msg_queue.count > 0){ //διασχίζουμε όλη την ουρά μηνυματων
        MSG* msg = &dialog->msg_queue.msgs[dialog->msg_queue.tail];
        if(msg_read_by_all(msg, dialog)){
            dialog->msg_queue.tail = (dialog->msg_queue.tail + 1) % max_msgs;   //παίρνουμε το μήνυμα που είναι πιο παλιό
            dialog->msg_queue.count--;  //μειώνουμε το μέγεθος της ουράς μηνυμάτων
            removed++;  //αυξάνουμε τα διαγραμμένα μηνυματα
        } 
        else{   //αν βρει μη διαβασμένο μήνυμα σταματα τη διαδικασια
            break;
        }
    }
    
    d_unlock(semid);
    for(int i = 0; i < removed; i++){   //στέλουμε σήμα για νέα κενά slot στην ουρα μηνυμάτων   
        signal_new_space(semid);
    }
    
    return removed; //επιστρέφουμε το πλήθος των διαγραμμένων μηνυματων
}