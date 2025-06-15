commande compilation : make
execution puit : tsock_texte -p 9001
execution source : tsock_texte -s localhost 9001

ce qui ne marche ou pas:

à partir de la V2, tsock_video fonctionne mal.
IP_Recv a un comportement bizzare

pout tsock_texte toute les version sont fonctionnels

version complété : 

V1 
V2 
V3 
V4.1 
V4.2

chaque version a été taggé sur github

---------------------------------choix d'implémentation-------------------------- 

stop and wait fiabilité partielle

pour le mécanisme de reprise des pertes à fiabilité partielle, nous avons choisi celui-ci:
on fonctionne par fenêtre glissante. c'est à dire que nous contrôlons le taux de perte moyen des paquets sur un certain nombre de paquet.
une fois que la fenêtre courante est gérée, on change de fenêtre.

exemple:

admettons que l'on fixe à 20% le taux de perte admissible. et que la fenêtre fait 100 paquet.
on va donc à chaque perte de paquet vérifier que le nombre de paquet perdu sur la fenêtre est inférieur à 20.
s'il est égal à 20. on réémet tout les autres paquets pour respecter le taux de perte.
dès que on fini de traîter les 100 paquets, on change de fenêtre et on réitère le processus.

on s'est servi pour la version 4 du PGCD pour réduire au maximum la taille de la fenêtre afin de mieux répartir les pertes.
diviser la taille de la fenêtre par le pgcd entre la taille de la fenêtre et le nombre de paquet perdu admissible permet de mieux maintenir le loss_rate





négociation des pertes

le programme source envoie son taux perte au puits avec le syn

le puits regarde si son propre taux de perte est compatible avec celui de la source.
si oui, le puits envoie le syn-ack avec le taux de pertes proposé par la source.
sinon, le puits envoie le syn-ack avec son propore taux de perte.

à la reception du syn-ack la source vérifie que le taux de perte reçu est bien celui proposé. si c'est
pas le cas, la connexion est annulée.

on considère qu'il y a compatibilé des taux de pertes si le taux de pertes proposé par la source est <= à celui du puits


asynchronisme côté serveur

pour gérer la phase d'établissement de connexion,
chaques changement d'état du socket est effectué en section critique, et est controllé par un mutex et une condition
pour la phase d'acceptation de connexion et la mise à jour de l'état du socket dans process_receive_pdu

---------------------------impact de la V4 par rapport à à la V2------------------------

grâce à la V4 , on devrait pouvoir conserver une certaine fluidité dans la lecture de la vidéo par rapport à la V2, car la gestion de la fiabilité total entraîne 
des coupure à la lecture. une fiabilité partielle est supposé pallier à ce problème.



