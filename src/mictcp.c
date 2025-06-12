#include <mictcp.h>
#include <api/mictcp_core.h>
#include <string.h>

#define MAX_SOCKETS_NUMBER 16 
#define MAX_RETRY 10
#define PAQUET_WINDOW 100


// au lieu de se contenter de un seul socket, on crée un tableau de socket pour pouvoir en gérer plusieurs
int sock_nb = 0;
mic_tcp_sock sockets[MAX_SOCKETS_NUMBER];

// variable pour savoir si le tableau a été initialiser sinon on l'initialise lors de la création du premier socket
int sock_init = 0;

//Pour la reception de pdu, PA et PE
int expected_seq = 0;
int n_seq = 0;


//pour une fiabilité partielle 
int windowPaquet = PAQUET_WINDOW;
int maxLose = 0;
int sentPaquet = 0;
int lossPaquet = 0;

// debug de cette mécanique
int total_sent_paquet = 0;
int total_lose_paquet = 0;

/*
 * Permet de créer un socket entre l’application et MIC-TCP
 * Retourne le descripteur du socket ou bien -1 en cas d'erreur
 */

int mic_tcp_socket(start_mode sm)
{
    int result = -1;
    printf("[MIC-TCP] Appel de la fonction: ");  printf(__FUNCTION__); printf("\n");
    result = initialize_components(sm); /* Appel obligatoire */
    set_loss_rate(50);


    // initialisation du tableau de socket
    if(!sock_init){
        for(int i = 0; i < MAX_SOCKETS_NUMBER; i++)
            sockets[i].fd = -1; 
        sock_init = 1;
    } 

    if(result != -1){ // ça ne sert à rien de créer le socket si on a pas reussi a initialiser les outils pour la programmation asynchrone

        if(sock_nb < MAX_SOCKETS_NUMBER){

            sockets[sock_nb].fd = sock_nb;
            sockets[sock_nb].state = IDLE;    // etat de depart
            sock_nb++;
            return sockets[sock_nb-1].fd;

        }
        else
            return -1;
    }

    else
        return result;
}

/*
 * Permet d’attribuer une adresse à un socket.
 * Retourne 0 si succès, et -1 en cas d’échec
 */
int mic_tcp_bind(int socket, mic_tcp_sock_addr addr)
{
    printf("[MIC-TCP] Appel de la fonction: ");  printf(__FUNCTION__); printf("\n");


    // on vériifie que le descripteur existe sinon on quitte immédiatement le sous programme et on le fait dans quasiment toutes les fonctions
    if(0 <= socket && socket < MAX_SOCKETS_NUMBER){ 

        sockets[socket].local_addr = addr; // association de l'adresse local et du socket
        return 0;
    }

    else
        return -1;
}

/*
 * Met le socket en état d'acceptation de connexions
 * Retourne 0 si succès, -1 si erreur
 */

int mic_tcp_accept(int socket, mic_tcp_sock_addr* addr) // appelé par le programme puits
{
    printf("[MIC-TCP] Appel de la fonction: ");  printf(__FUNCTION__); printf("\n");
    if(0 <= socket && socket < MAX_SOCKETS_NUMBER){

        sockets[socket].remote_addr = *addr; // faut quand même que l'on sache à qui on envoie la demande de connexion. On ne connaît pas l'adresse distante lors de la creation du socket

        while(sockets[socket].state != SYN_RECEIVED);
        while(sockets[socket].state != ESTABLISHED);


        return 0;
    }
    else
        return -1;

    
}

/*
 * Permet de réclamer l’établissement d’une connexion
 * Retourne 0 si la connexion est établie, et -1 en cas d’échec
 */
int mic_tcp_connect(int socket, mic_tcp_sock_addr addr) // appelé  par le programme source
{
    printf("[MIC-TCP] Appel de la fonction: ");  printf(__FUNCTION__); printf("\n");

    if(0 <= socket && socket < MAX_SOCKETS_NUMBER){

        int loss_rate_req = 20;

        mic_tcp_pdu pdu_syn;
        pdu_syn.header.syn = 1;
        pdu_syn.header.ack = 0;
        pdu_syn.payload.size = 0;
        pdu_syn.header.dest_port = addr.port;

        pdu_syn.payload.data =  malloc(sizeof(char)*3);
        pdu_syn.payload.size = 3*sizeof(char);

        sprintf(pdu_syn.payload.data,"%d",loss_rate_req);

        IP_send(pdu_syn,addr.ip_addr);


        //on se met en état wait 
        sockets[socket].state = SYN_SENT;
        mic_tcp_pdu pdu_syn_ack;
        pdu_syn_ack.payload.data = malloc(sizeof(char)*3);
        pdu_syn_ack.payload.size = sizeof(char)*3;
        
        if(IP_recv(&pdu_syn_ack,&sockets[socket].local_addr.ip_addr,&addr.ip_addr,100.0) == -1){
            return -1;
        }

        //a ce stade on a recu le syn_ack
        if(pdu_syn_ack.header.syn == 1 && pdu_syn_ack.header.ack == 1)
        {

            if(atoi(pdu_syn_ack.payload.data) != atoi(pdu_syn.payload.data))
                return -1;
            
            maxLose = loss_rate_req;
                
            mic_tcp_pdu pdu_ack;
            pdu_ack.header.syn = 0;
            pdu_ack.header.ack = 1;
            pdu_ack.payload.size = 0;
            pdu_ack.header.dest_port = addr.port;
            IP_send(pdu_ack,addr.ip_addr);
            sockets[socket].state = ESTABLISHED;

            sockets[socket].remote_addr = addr; // faut quand même que l'on sache à qui on envoie la demande de connexion. On ne connaît pas l'adresse distante lors de la creation du socket
            
            return 0;

        }

       else{
            return -1;
       }

    } 

    
    
    
    else
        return -1;
}

/*
 * Permet de réclamer l’envoi d’une donnée applicative
 * Retourne la taille des données envoyées, et -1 en cas d'erreur
 */
 
int mic_tcp_send(int mic_sock, char* mesg, int mesg_size)
{
    printf("[MIC-TCP] Appel de la fonction: "); printf(__FUNCTION__); printf("\n");

    if(0 <= mic_sock && mic_sock < MAX_SOCKETS_NUMBER){

            
        if(total_sent_paquet != 0)
            printf("Perdu:%d | envoye:%d\nloss rate :%f\n", total_lose_paquet, total_sent_paquet, (float)total_lose_paquet/(float)total_sent_paquet);

        //set_loss_rate(0); // pour simuler les paquets perdu

        mic_tcp_pdu PDU;
        mic_tcp_pdu Recv_PDU;

        int bytes_sent = 0;
        int nb_retransmit = 0;
        int ack_recv = 0;
        unsigned long ms_timer = 100.0;
        int result;

        Recv_PDU.payload.data = malloc(sizeof(char)*4);

        Recv_PDU.payload.size = 4*sizeof(char);

        // MESSAGE UTILE

        PDU.payload.data = mesg;        // buffer dans lequel est stockée les données utiles
        PDU.payload.size = mesg_size;   // taille du message utile

        // HEADER

        PDU.header.dest_port = sockets[mic_sock].remote_addr.port; // port de destination
        PDU.header.source_port = sockets[mic_sock].local_addr.port; // port source
        PDU.header.seq_num = n_seq;


        while(!ack_recv && nb_retransmit < MAX_RETRY){

            // SEND OR RETRY
            result = -1;
            bytes_sent = IP_send(PDU,sockets[mic_sock].remote_addr.ip_addr);
            
            // WAIT FOR ACK
            
            result = IP_recv(&Recv_PDU, &sockets[mic_sock].local_addr.ip_addr, &sockets[mic_sock].remote_addr.ip_addr, ms_timer);
            printf("res:%d\n",result);
            sentPaquet = (sentPaquet+1) % windowPaquet;

            if(sentPaquet == 0){

            lossPaquet = 0;
            }

            total_sent_paquet++;

            // on vérifie que l'on reçois le bon num d'acquittement et on met ack_recv à true

            printf("PE = %d| ACK_N = %d | ack = %d\n", n_seq, Recv_PDU.header.ack_num, Recv_PDU.header.ack);
            if(result != -1 && Recv_PDU.header.ack == 1 && Recv_PDU.header.ack_num == n_seq){
                ack_recv = 1;
                //puts("ACK\n");
            }

            // s'il agissait pas d'un ack ou que le num est le mauvais ou que le temps est dépassé, on se prépare à la retransmission
            else{
                if(lossPaquet < maxLose){
                    lossPaquet++;
                    total_lose_paquet++;
                    free(Recv_PDU.payload.data);
                    
                    return -1;
                    
                }
                nb_retransmit++;
                //puts("PB\n");
            }
        }
        //puts("END\n");

        if(ack_recv){

            // MAJ n_seq

            n_seq = (n_seq+1)%2;
            free(Recv_PDU.payload.data);
            return bytes_sent;

        }
        else{
            free(Recv_PDU.payload.data);
            return -1;
        } 
        
    }
    else
        return -1;

}

/*
 * Permet à l’application réceptrice de réclamer la récupération d’une donnée
 * stockée dans les buffers de réception du socket
 * Retourne le nombre d’octets lu ou bien -1 en cas d’erreur
 * NB : cette fonction fait appel à la fonction app_buffer_get()
 */
int mic_tcp_recv (int socket, char* mesg, int max_mesg_size)
{
    printf("[MIC-TCP] Appel de la fonction: "); printf(__FUNCTION__);printf("\n");
    
    if(0 <= socket && socket < MAX_SOCKETS_NUMBER)

    {

        mic_tcp_payload payload;
        payload.data = mesg;
        payload.size = max_mesg_size;

        int effective_data_size = app_buffer_get(payload);


        return effective_data_size;

    }
    
    else
        return -1;
}

/*
 * Permet de réclamer la destruction d’un socket.
 * Engendre la fermeture de la connexion suivant le modèle de TCP.
 * Retourne 0 si tout se passe bien et -1 en cas d'erreur
 */
int mic_tcp_close(int socket)
{
    printf("[MIC-TCP] Appel de la fonction :  "); printf(__FUNCTION__); printf("\n");
    if(0 <= socket && socket < MAX_SOCKETS_NUMBER)

    {
        sockets[socket].fd = -1;
        sockets[socket].state = CLOSED;
        sock_nb--;
        return 0;
    }
    return -1;
}

/*
 * Traitement d’un PDU MIC-TCP reçu (mise à jour des numéros de séquence
 * et d'acquittement, etc.) puis insère les données utiles du PDU dans
 * le buffer de réception du socket. Cette fonction utilise la fonction
 * app_buffer_put().
 */

void process_received_PDU(mic_tcp_pdu pdu, mic_tcp_ip_addr local_addr, mic_tcp_ip_addr remote_addr)
{
    
    printf("[MIC-TCP] Appel de la fonction: "); printf(__FUNCTION__); printf("\n");

    //set_loss_rate(0);
    mic_tcp_pdu pdu_ack;
    pdu_ack.payload.size = 0;
    pdu_ack.header.ack = 1;
    pdu_ack.header.ack_num = pdu.header.seq_num; // faut le faire avant de vérifier les seq num car soit le msg était un ancien ou l'attendu, il faut ack dans tout les cas

    /*Si on recu bien le pdu attendu on le met dans le buffer et on envoie un ack*/

    for(int i = 0; i < MAX_SOCKETS_NUMBER; i++){


        if(sockets[i].local_addr.port == pdu.header.dest_port){

            if(sockets[i].state == ESTABLISHED){

                
                printf("SEQ = %d| PA = %d, ACK_N = %d\n ", pdu.header.seq_num, expected_seq, pdu_ack.header.ack_num);
                if(pdu.header.seq_num == expected_seq)
                {

                    app_buffer_put(pdu.payload);
                    expected_seq = (expected_seq + 1)%2;
                }

            }

            else if(sockets[i].state == SYN_RECEIVED){


                if(pdu.header.ack){
                    sockets[i].state = ESTABLISHED;
                    return;
                }
            }

            else if(sockets[i].state == IDLE){

                int negociated_loss_rate = atoi(pdu.payload.data);
                int loss_rate_request = 20;

                if(pdu.header.syn && !pdu.header.ack){

                    pdu_ack.payload.data = malloc(sizeof(char)*3);
                        pdu_ack.payload.size = sizeof(char)*3;

                    if(negociated_loss_rate <= loss_rate_request){
                        sprintf(pdu_ack.payload.data,"%d",negociated_loss_rate);
                    }
                    
                    else{
                        sprintf(pdu_ack.payload.data,"%d",loss_rate_request);
                    }
                

                    pdu_ack.header.syn = 1;
                    sockets[i].state = SYN_RECEIVED;
                }
            }

            /*Sinon on a recu un doublon et dans ce cas on envoie juste un ack sans le mettre 
            dans le buffer donc pas besoin de faire  un else car dans tous les cas on
            fait un send*/
                        

            //SEND ACK or SYN 
            printf("Evoie ack N:%d\n", pdu_ack.header.ack_num);
            IP_send(pdu_ack,remote_addr); 

        }
    
       
    }
    
}

