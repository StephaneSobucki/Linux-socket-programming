#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <pthread.h>
#include<signal.h>

#define ANSI_COLOR_RED "\x1b[31m"
#define ANSI_COLOR_GREEN "\x1b[92m"
#define ANSI_COLOR_YELLOW "\x1b[93m"
#define ANSI_COLOR_BLUE "\x1b[94m"
#define ANSI_COLOR_MAGENTA "\x1b[95m"
#define ANSI_COLOR_CYAN "\x1b[36m"
#define ANSI_COLOR_RESET "\x1b[0m"

int BUFFER_SIZE = 256;
char name[256];
int g_done = 1;//flag pour deconnexion du serveur/client

void *splitIt(char str[]){
	strtok(str, "\n");
}
void *send_message(void *p_socket){
	int network_socket = *(int *) p_socket;
	int n;
	char bufferR[BUFFER_SIZE-1];
	//Tant que l'on est connecte au serveur
	while(g_done != 0){
		printf("\x1b[96m%s: \x1b[0m",name);
		fflush(stdout);
		bzero(bufferR,BUFFER_SIZE);
		//Tant que l'entree de l'utilisateur existe
		while(fgets(bufferR,BUFFER_SIZE,stdin) != NULL){
			if(bufferR[0] != '\0') break;//Si on a une entree clavier de l'utilisateur, on sort de la boucle
		}
		g_done = strncmp("Exit",bufferR,4);//On compare l'entree clavier de l'utilisateur et le mot cle de déconnexion Exit
		if((n = write(network_socket,bufferR,BUFFER_SIZE)) < 0){
			fprintf(stderr,"Erreur d'ecriture socket communication\n");
			exit(EXIT_FAILURE);
		}
		if (g_done == 0){//Si le flag est à 0, on arrête la transmission de message
			break;
		}
	}

}
void *receive_message(void *p_socket){
	char bufferW[BUFFER_SIZE-1];
	int network_socket = *(int *) p_socket;
	int n;
	int flag_disc;
	while(1){
		if((n = read(network_socket, bufferW, BUFFER_SIZE)) < 0){
			fprintf(stderr,"Erreur de lecture socket communication\n");
			exit(EXIT_FAILURE);
		}
		//Si on recoit la cle de deconnexion du serveur, on affiche un message de deconnexion et on met le flag a 0 pour preparer
		//la fermeture de la network_socket
		if((flag_disc = strncmp("12mrné)jlflknémqln&)à1KN2)çj&fnin&oil",bufferW,39))==0){//C'est la clé emise par le serveur quand il se deconnecte
			printf(ANSI_COLOR_RED "\rServer Disconnect\n" ANSI_COLOR_RESET);
			fflush(stdout);
			g_done = 0;
			break;
		}
		if (n > 0){
			//Utilise pour les messages prives
			if(strncmp(bufferW,"(de)",4) == 0){
				printf(ANSI_COLOR_MAGENTA"\r%s"ANSI_COLOR_RESET,bufferW);
			}
			else if(strncmp(bufferW,"Message non envoye(utilisateur introuvable)",42) == 0){
				printf(ANSI_COLOR_RED"\r%s"ANSI_COLOR_RESET,bufferW);
			}
			else if(strncmp(bufferW,"liste des clients: [",18) == 0){
				printf(ANSI_COLOR_YELLOW"\r%s\n"ANSI_COLOR_RESET,bufferW);
			}
			//Utilise pour les messages globaux
			else{
				printf("\r%s",bufferW);
			}
			printf("\x1b[96m \r%s: \x1b[0m",name);
			fflush(stdout);
		}
		else if (n == 0){//si le retour de read est 0, on est deconnecte
			break;
		}


        }

}
int affiche_adresse_socket(int socket)
{
	struct sockaddr_in adresse;
        socklen_t longueur;

        longueur = sizeof(struct sockaddr_in);

        if (getsockname(socket, (struct sockaddr*)&adresse, &longueur) < 0)
        {
        	fprintf(stderr, "Erreur getsockname\n");
                return -1;
        }

        printf("IP = \x1b[94m %s \x1b[0m, Port = \x1b[94m %u\n \x1b[0m", inet_ntoa(adresse.sin_addr),
        ntohs(adresse.sin_port));

        return 0;
}
//Utilise pour quitter le serveur avec CTRL+C
void deconnexion(int signal){
	g_done = 0;
}

int main(int argc, char *argv[]){
	signal(SIGINT,deconnexion);
	int network_socket;	      
	if((network_socket = socket(AF_INET, SOCK_STREAM,0)) == -1){
		fprintf(stderr,"Erreur creation socket de communication\n");
		return -1;
	}
	struct sockaddr_in server_address;
	server_address.sin_family = AF_INET;
	server_address.sin_port = htons(atoi(argv[2]));
	server_address.sin_addr.s_addr = inet_addr(argv[1]);
	if((connect(network_socket, (struct sockaddr *) &server_address, sizeof(server_address)))<0){;
		fprintf(stderr,"There was an error making a connection to the remote socket\n");
		return -1;
	}
	int intBuffer[1];
	read(network_socket,intBuffer,sizeof(int));
	if(intBuffer[0] == 0){
		if(close(network_socket) < 0){
			fprintf(stderr,"Erreur fermeture socket de communication\n");
		}
		printf("Serveur complet : Veuillez reessayer plus tard\n");
		return -1;
	}
	printf(ANSI_COLOR_YELLOW"Bonjour, veuillez choisir un pseudo: "ANSI_COLOR_RESET);
	//Entree du pseudonyme de l'utilisateur
	while(1){
		while(1){
			fgets(name,256,stdin);
			//Supression du retour a la ligne
			splitIt(name);
			if((strlen(name) < 3)  || (strlen(name) > 8)){
				printf(ANSI_COLOR_RED"\rLe pseudo doit etre compris entre 3 et 8 caractères: "ANSI_COLOR_RESET);
			}
			else{
				int nospecial = 1;
				//On verifie egalement la presence d'espace ou de caracteres speciaux dans le pseudonyme
				//Si on detecte leur presence, on redemande un pseudo a l'utilisateur
				for(int i = 0; i < strlen(name); i++)
				{
					if((name[i] == ' ')||(name[i] == '@')||(name[i] == '!')){
						nospecial = 0;
						printf(ANSI_COLOR_RED"\rLe pseudo ne doit pas comprendre d'espace ou de caracteres speciaux(!,@): "ANSI_COLOR_RESET);
						break;
					}
				}
				if(nospecial == 1){
					break;
				}
			}
		}
		//On envoie le pseudonyme au serveur, si celui ci n'est pas utilise on recoit un 1 dans le intBuffer
		if(write(network_socket,name,16) < 0){
			fprintf(stderr,"Erreur d'ecriture socket communication\n");
			return -1;
		}
		if(read(network_socket,intBuffer,sizeof(int)) < 0){
			fprintf(stderr,"Erreur de lecture socket communication\n");
			return -1;
		}
		if(intBuffer[0]){
			break;
		}
		//Sinon on recommence le processus
		printf(ANSI_COLOR_RED"\rCe pseudo est deja utilise. Veuillez reessayer: "ANSI_COLOR_RESET);
	}
	affiche_adresse_socket(network_socket);
	printf(ANSI_COLOR_GREEN"Bienvenue sur le serveur, %s. Voici quelques commandes qui vous seront utiles:"\
		       	"\n1. @username pour envoyer un message privé \n2. Exit pour quitter le serveur\n"\
			"3. !list pour afficher la liste des utilisateurs connectés\n"ANSI_COLOR_RESET,name);
	//2 threads : 1 thread pour l'envoie des messages et 1 thread pour la reception des messages
	pthread_t send_t;
	//thread pour l'envoie des messages
	if(pthread_create(&send_t,NULL,send_message,&network_socket)!=0){
        	fprintf(stderr,"Erreur de creation de thread\n");
                return -1;
        }
	//thread pour la reception des messages
	pthread_t receive_t;
	if(pthread_create(&receive_t,NULL,receive_message,&network_socket)!=0){
        	fprintf(stderr,"Erreur de creation de thread\n");
                return -1;
        }
	//Tant que le flag est à 1 on est connecte
	while(1){
		if (g_done == 0){
			printf("\rDeconnexion du serveur...\n");
			break;
		}
		
	}
	//A la fin on ferme la socket de communication
	if(close(network_socket) < 0){
		fprintf(stderr,"Erreur close socket communication\n");
		return -1;
	}
	return 0;
}
