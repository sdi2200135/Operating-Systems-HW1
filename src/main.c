#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <ctype.h>

#include "common.h"
#include "shared_memory.h"
#include "dialog_control.h"
#include "msg_queue.h"

static int curr_dialog_id = -1; //το id του τρέχοντος διαλόγου
static int curr_pid = 0;    //το pid της τρέχουσας διεργασίας

void sign_handler(int signum){    //συνάρτηση που κάνει διαχείριση των σημάτων για τον user
    printf("\nΛήψη σήματος %d. Τερματισμός...\n", signum);
    
    if(curr_dialog_id != -1){   //αν ο user είναι σε καποιο διάλογο τότε αποχωρεί από αυτον
        Dialog_list* d_list = attach_d_list(0);
        if(d_list){
            leave_dialog(d_list, curr_dialog_id, curr_pid);
            detach_d_list();
        }
    }
    
    clean_all();    //αποσύνδεση από το διάλογο και τη λίστα διαλόγων
    exit(0);    //τερματισμός 
}

void handle_send_command(Dialog* dialog, int dialog_id){    //συνάρτηση που κάνει διαχέιριση για την εντολή αποστολής μηνύματος
    char msg[max_msg_length];
    
    printf("Εισάγετε μήνυμα ή TERMINATE για τερματισμό: ");
    getchar();
    fgets(msg, max_msg_length, stdin);  //διαβάζουμε το μήνυμα από το πληκτρολόγιο
    msg[strcspn(msg, "\n")] = 0;    //αφαιρούμε το newline
    
    if(strlen(msg) == 0){   //αν δεν έχει περιεχόμενο το μήνυμα, σταματαμε
        printf("Κενό μήνυμα. Ακύρωση.\n");
        return;
    }
    
    int terminated = (strcmp(msg, "TERMINATE") == 0);   //ελέγχουμε αν το μήνυμα είναι terminate
    
    if(send_msg(dialog, dialog->sem_id, msg, curr_pid, terminated)){    //αν γίνει αποστολή του μηνύματος
        printf("Μήνυμα απεστάλη επιτυχώς! (Συμμετέχοντες: %d)\n", dialog->participants_c);
        
        if(terminated){ //αν το μήνυμα ειναι terminate
            printf("Απεστάλη TERMINATE. Τερματισμός σε 2 δευτερόλεπτα...\n");
            sleep(2);   //περιμένει για 2 δευτερολεπτα
            
            Dialog_list* d_list = attach_d_list(0); 
            if(d_list){ //συνδεόμαστε στη λίστα μηνυμάτων και 
                leave_dialog(d_list, dialog_id, curr_pid);  //αποχωρούμε από το διάλογο
                detach_d_list();    //και κάνουμε αποσύνδεση από τη λίστα διαλόγων
            }
            
            clean_all();    //αποσύνδεση από το διάλογο και τη λίστα διαλόγων
            exit(0);    //τερματισμός
        }
    } 
    else{   //αλλιώς η ουρά μηνυμάτων είναι γεμάτη και πρέπει να περιμένουμε να αδειάσει κάποιο slot
        printf("Σφάλμα: Η ουρά μηνυμάτων είναι γεμάτη!\n");
    }
}

void handle_receive_command(Dialog* dialog, int dialog_id){ //συνάρτηση που κάνει διαχέιριση για την εντολή λήψης μηνύματος 
    MSG received_msg;
    int msgs_received = 0;  //κρατάμε το πλήθος των μηνυματων που έχουν ληφθει από τους παραλήπτες
    
    printf("Αναζήτηση νέων μηνυμάτων\n");
    while(receive_msg(dialog, dialog->sem_id, &received_msg, curr_pid)){    //γίνεται λήψη όλων των μη διαβασμένων μηνυμάτων
        printf("\n---- Νέο Μήνυμα ----\n"); //εμφανίζουμε τα στοιχεία του κάθε μηνύματος
        printf("Αποστολέας (PID): %d\n", received_msg.sender_pid);
        printf("Περιεχόμενο: %s\n", received_msg.cont);
        printf("Αρχικοί παραλήπτες: %d\n", received_msg.participants);
        printf("Διαβάστηκε από: %d/%d\n", received_msg.readers_c, dialog->participants_c);
        
        msgs_received++;    //αυξάνουμε τα μηνύματα που έχουν ληφθεί από τους παραλήπτες
        
        if(received_msg.terminated || strcmp(received_msg.cont, "TERMINATE") == 0){ //έλεγχος αν το μήνυμα είναι TERMINATE
            printf("\nΛήφθηκε TERMINATE από PID %d!\n", received_msg.sender_pid);
            printf("Τερματισμός σε 2 δευτερόλεπτα...\n");
            sleep(2);
            
            Dialog_list* d_list = attach_d_list(0);
            if(d_list){ //συνδεόμαστε στη λίστα μηνυμάτων και
                leave_dialog(d_list, dialog_id, curr_pid);  //αποχωρούμε από το διάλογο
                detach_d_list();    //και κάνουμε αποσύνδεση από τη λίστα διαλόγων
            }
            
            clean_all();    //κάνουμε αποσύνδεση από το διάλογο και τη λίστα διαλόγων
            exit(0);    //τερματισμός
        }
    }
    
    if(msgs_received == 0){
        printf("Δεν υπάρχουν νέα μηνύματα.\n");
    } 
    else{
        printf("---------------------\n");
        printf("Λήφθηκαν %d μηνύματα.\n", msgs_received);
    }
}

void print_menu(){  //συνάτηση που εμφανίζει το μενού με τις επιλογές μέσα σε ένα διάλογο
    printf("\n---- Μενού ----\n");
    printf("[S] Αποστολή μηνύματος (Send)\n");
    printf("[R] Λήψη μηνυμάτων (Receive)\n");
    printf("[I] Πληροφορίες διαλόγου (Info)\n");
    printf("[L] Κατάσταση συστήματος (List)\n");
    printf("[U] Χρήστες (Users)\n");
    printf("[E] Έξοδος (Exit)\n");
    printf("Επιλογή: ");
}

int main(void){
    //διαχείριση σηματων τερματισμού
    signal(SIGINT, sign_handler);   //τερματισμός με Ctrl+c
    signal(SIGTERM, sign_handler);  //σήμα τερματισμού από το σύστημα 
    
    curr_pid = getpid();    //αποθηκευση του τρεχοντος pid
    
    printf("\n-------------------------------\n");
    printf("  Σύστημα Ανταλλαγής Μηνυμάτων\n");
    printf("  PID διεργασίας: %d\n", curr_pid);
    printf("-------------------------------\n\n");
    
    Dialog_list* d_list = attach_d_list(0); //σύνδεση στη λίστα διαλόγων 
    if(!d_list){
        printf("Δημιουργία νέας λίστας διαλόγων\n");
        d_list = attach_d_list(1);  //δημιουργία λίστας δια΄λόγων αν δεν υπάρχει
        if(!d_list){
            printf("Σφάλμα: Δεν είναι δυνατή η σύνδεση στην λίστα διαλόγων!\n");
            return 1;
        }
    }
    
    char ch;
    int dialog_id = -1;
    int exit_prog = 0;  //flag για εξοδο από το σύστημα
    
    while(!exit_prog){
        printf("\nΕπιλογές:\n");    //εμφανίζουμε τις επιλογές για δημιουργία, συμμετοχή σε διαλογο ή έξοδο από το σύστημα
        printf("[N] Νέος διάλογος (New)\n");
        printf("[J] Σύνδεση σε ήδη υπάρχοντα (Join)\n");
        printf("[L] Κατάσταση συστήματος (List)\n");
        printf("[E] Έξοδος (Exit)\n");
        printf("Δώσε την επιλογή σου: ");
        scanf(" %c", &ch);  
        
        ch = toupper(ch);   //μετατρέπουμε την είσοδο σε κεφαλαία γράμματα σε περιπτωση που δωθει με μικρα
        if(ch == 'N'){  //νέος διάλογος
            dialog_id = create_new_dialog(d_list);  //δημιουργεί νεο διαλογο
            if(dialog_id == -1){    //αποτυχία δημιουργίας 
                continue;
            }
            printf("Δημιουργήθηκε νέος διάλογος με ID: %d\n", dialog_id);
            break;
        } 
        else if(ch == 'J'){ //σύνδεση σε ήδη υπάρχοντα διάλογο
            printf("Εισάγετε ID διαλόγου: ");
            scanf("%d", &dialog_id);    //διαβάζουμε το id του διαλόγου από το πληκτρολογιο
            
            if(join_dialog(d_list, dialog_id) == -1){   //προσπαθούμε να συνδεθούμε στο διάλογο
                printf("Ο διάλογος %d δεν υπάρχει!\n", dialog_id);
                continue;
            }
            printf("Συνδέθηκες στον διάλογο %d\n", dialog_id);
            break;
        } 
        else if(ch == 'L'){ //εμφάνιση των διαθέσιμων διαλόγων
            list_active_dialogs(d_list);
            continue;
        }
        else if(ch == 'E'){ //έξοδος από το σύστημα 
            detach_d_list();    //κάνουμε αποσύνδεση από τη λίστα διαλόγων
            printf("Έξοδος...\n");
            return 0;
        } 
        else{   //περίπτωση που δίνεται μη έγκυρη επιλογή
            printf("Μη έγκυρη επιλογή!\n");
            continue;
        }   
    }

    if(ch != 'N' && ch != 'J'){
        detach_d_list();
        return 0;
    }

    curr_dialog_id = dialog_id; //αποθηκεύουμε το id του τρέχοντα διάλογου
    
    int creator = (ch == 'N');  //κρατάμε αν θέλουμε να δημιουργηθεί νέος διάλογος
    Dialog* dialog = attach_dialog(dialog_id, creator); //κάνουμε σύνδεση στο διάλογο
    if(!dialog){
        printf("Σφάλμα: Δεν είναι δυνατή η σύνδεση στα δεδομένα του διαλόγου!\n");
        detach_d_list();
        return 1;
    }
    
    LOCK(dialog->sem_id);
    add_participant(dialog, curr_pid);  //προσθέτουμε τον τρέχον συμμετέχοντα στο διάλογο
    UNLOCK(dialog->sem_id);
    
    printf("\n-------------------------------------\n");
    printf("Συνδέθηκες στο διάλογο %d\n", dialog_id);
    printf("Συμμετέχοντες: %d\n", dialog->participants_c);
    printf("-------------------------------------\n\n");
    
    while(1){
        print_menu();   //εμφανίζουμε το menu
        scanf(" %c", &ch);  //διαβάζουμε την επιλογή που εισάγεται
        ch = toupper(ch);   //μετατρέπουμε σε κεφαλαια γράμματα
        
        switch(ch){
            case 'S':   //επιλογή αποστολής μηνύματος 
                handle_send_command(dialog, dialog_id);
                break;
            case 'R':   //επιλογή λήψης μηνυματος
                handle_receive_command(dialog, dialog_id);
                break;
            case 'I':   //επιλογή εμφάνισης πληροφοριών διαλόγου
                printf("\n---- Πληροφορίες Διαλόγου ----\n");
                printf("ID Διαλόγου: %d\n", dialog->dialog_id);
                printf("Συμμετέχοντες: %d\n", dialog->participants_c);
                printf("Μηνύματα στην ουρά: %d\n", dialog->msg_queue.count);
                printf("Ενεργός: %s\n", dialog->actived ? "ΝΑΙ" : "ΟΧΙ");
                list_participants(dialog);
                break;
            case 'U':   //επιλογή εμφάνισης των συμμετεχόντων στο διάλογο
                printf("\nΧρήστες στο διάλογο:\n");
                list_participants(dialog);
                break;
            case 'L':   //εμφάνιση των ενεργών διαλόγων στο συστημα
                list_active_dialogs(d_list);
                break;
            case 'E':   //επιλογή εξόδου από το σύστημα 
                printf("Αποχώρηση από το σύστημα...\n");
                leave_dialog(d_list, dialog_id, curr_pid);  //αποχώριση από το διάλογο 
                detach_d_list();    //αποσύνδεση από τη λίστα διαλόγων 
                clean_all();    //αποσύνδεση από το διάλογο και τη λίστα διαλόγων
                printf("Αποσυνδέθηκες επιτυχώς.\n");
                return 0;
            default:    //μη έγκυρη επιλογή
                printf("Μη έγκυρη επιλογή. Δοκιμάστε ξανά!\n");
        }
    }
    return 0;
}