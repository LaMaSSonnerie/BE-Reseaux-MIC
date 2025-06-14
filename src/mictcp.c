
/* ----------------------------------------V1--------------------------------------*/

#include <mictcp.h>
#include <api/mictcp_core.h>

#define MAX_SOCKETS_NUMBER 16 


// nombre de socket utilisé
int sock_nb = 0;

// pour éviter que il y ait de vlaur aléatoire dans le tableau des socket, cette variable permet de savoir si le tableau est initialisé ou pas
int sock_init = 0;

// au lieu de se contenter de un seul socket, on crée un tableau de socket pour pouvoir en gérer plusieurs
mic_tcp_sock sockets[MAX_SOCKETS_NUMBER]; 


/*
 * Permet de créer un socket entre l’application et MIC-TCP
 * Retourne le descripteur du socket ou bien -1 en cas d'erreur
 */


int mic_tcp_socket(start_mode sm)
{
    int result = -1;
    printf("[MIC-TCP] Appel de la fonction: ");  printf(__FUNCTION__); printf("\n");
    result = initialize_components(sm); /* Appel obligatoire */
    set_loss_rate(30);

    // initialisation du tableau de socket

    if(!sock_init){

        for(int i = 0; i < MAX_SOCKETS_NUMBER;i++)
            sockets[i].fd = -1;
        sock_init = 1;
    } 

    // on vérifie que les composant sont bien initialisé et que il y a toujours des socket de disponible
    if(result != -1 && sock_nb < MAX_SOCKETS_NUMBER){ 
      
        // on cherche un socket de disponible
        for(int i = 0; i < MAX_SOCKETS_NUMBER; i++){

            // socket disponible trouvé
            if(sockets[i].fd < 0){

                sockets[i].fd = i;
                sockets[i].state = IDLE; 
                sock_nb++;
                result = i;
                break;
                
                
            } 
        } 
        return result;
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

    // on vérifie si le socket existe
    if(0 <= socket && socket < MAX_SOCKETS_NUMBER){

        // association de l'adresse local et du socket
        sockets[socket] .local_addr = addr; 
        return 0;
    }

    else
        return -1;
}

/*
 * Met le socket en état d'acceptation de connexions
 * Retourne 0 si succès, -1 si erreur
 * appellé par le puits
 */

int mic_tcp_accept(int socket, mic_tcp_sock_addr* addr)
{
    printf("[MIC-TCP] Appel de la fonction: ");  printf(__FUNCTION__); printf("\n");
    if(0 <= socket && socket < MAX_SOCKETS_NUMBER){

        // enregistrement de l'adresse de l'hôte distant
        sockets[socket].remote_addr = *addr;
        return 0;
    }
    else
        return -1;

    
}

/*
 * Permet de réclamer l’établissement d’une connexion
 * Retourne 0 si la connexion est établie, et -1 en cas d’échec
 * appellé  par le programme source
 */
int mic_tcp_connect(int socket, mic_tcp_sock_addr addr)
{
    printf("[MIC-TCP] Appel de la fonction: ");  printf(__FUNCTION__); printf("\n");

    if(0 <= socket && socket < MAX_SOCKETS_NUMBER){

        // enregistrement de l'adresse de l'hôte distant
        sockets[socket].remote_addr = addr; 
        return 0;

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

        mic_tcp_pdu PDU;
        PDU.payload.data = mesg;        // buffer dans lequel est stockée les données utiles
        PDU.payload.size = mesg_size;   // taille du message utile

        PDU.header.dest_port = sockets[mic_sock].remote_addr.port; // port de destination
        PDU.header.source_port = sockets[mic_sock].local_addr.port; // port source

        // envoie du pdu depuis la couche IP
        int bytes_sent = IP_send(PDU,sockets[mic_sock].remote_addr.ip_addr);

        return bytes_sent;

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

        // ecriture des données bufferisé dans la payload
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
        // fermeture du socket
        sockets[socket].fd = -1;
        sockets[socket].state = IDLE;
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
void process_received_PDU(mic_tcp_pdu pdu, mic_tcp_ip_addr local_addr, mic_tcp_ip_addr remote_addr) // on vérifie pas que le remote_addr est associé à un port d'écoute, on est en version 1
{
    printf("[MIC-TCP] Appel de la fonction: "); printf(__FUNCTION__); printf("\n");

    // écriture dans le buffer des données reçu
    app_buffer_put(pdu.payload);
}
