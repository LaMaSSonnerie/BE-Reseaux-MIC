#include <mictcp.h>
#include <api/mictcp_core.h>

char* buffer[128];
mic_tcp_sock sockt; // je suis supposé créer un tableau de socket parce que par exemple, les fonctions identifie le socket par leurs FD. c'est donc pas logique de ne créer que un socket global

/*
 * Permet de créer un socket entre l’application et MIC-TCP
 * Retourne le descripteur du socket ou bien -1 en cas d'erreur
 */
int mic_tcp_socket(start_mode sm)
{
    int result = -1;
    printf("[MIC-TCP] Appel de la fonction: ");  printf(__FUNCTION__); printf("\n");
    result = initialize_components(sm); /* Appel obligatoire */
    set_loss_rate(0);

    if(result != -1){ // ça ne sert à rien de créer le socket si on a pas reussi a initialiser les outils pour la programmation asynchrone

        sockt.fd = 1;
        sockt.state = IDLE;    // etat de depart
        return sockt.fd;
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

    if(sockt.fd == socket){  // pareil je suis censé faire cette vérification sur un tableau 

        sockt.local_addr = addr;
        return 0;
    }

    else
        return -1;
}

/*
 * Met le socket en état d'acceptation de connexions
 * Retourne 0 si succès, -1 si erreur
 */
int mic_tcp_accept(int socket, mic_tcp_sock_addr* addr)
{
    printf("[MIC-TCP] Appel de la fonction: ");  printf(__FUNCTION__); printf("\n");


    return -1;
}

/*
 * Permet de réclamer l’établissement d’une connexion
 * Retourne 0 si la connexion est établie, et -1 en cas d’échec
 */
int mic_tcp_connect(int socket, mic_tcp_sock_addr addr)
{
    printf("[MIC-TCP] Appel de la fonction: ");  printf(__FUNCTION__); printf("\n");

    return -1;
}

/*
 * Permet de réclamer l’envoi d’une donnée applicative
 * Retourne la taille des données envoyées, et -1 en cas d'erreur
 */
int mic_tcp_send(int mic_sock, char* mesg, int mesg_size)
{
    printf("[MIC-TCP] Appel de la fonction: "); printf(__FUNCTION__); printf("\n");

    if(sockt.fd == socket){

        mic_tcp_pdu PDU;
        PDU.payload.data = mesg;
        PDU.payload.size = mesg_size;

        PDU.header.dest_port = sockt.remote_addr.port;
        PDU.header.source_port = sockt.local_addr.port;

        int bytes_sent = IP_send(PDU,sockt.remote_addr.ip_addr);

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
    
    if(sockt.fd == socket)

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
int mic_tcp_close (int socket)
{
    printf("[MIC-TCP] Appel de la fonction :  "); printf(__FUNCTION__); printf("\n");
    if(sockt.fd == socket)

    {
        sockt.fd = -1;
        sockt.state;
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

    
}
