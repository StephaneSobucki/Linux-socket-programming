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
#include <signal.h>

#define ANSI_COLOR_RED "\x1b[31m"
#define ANSI_COLOR_GREEN "\x1b[92m"
#define ANSI_COLOR_RESET "\x1b[0m"

int g_server_socket;//global pour pouvoir y acceder avec le signal d'extinction du serveur
const int MAX_THREADS=8;//Capacite de notre serveur (ici 16 clients max)
int thread_counter = 0;//nombre de threads en cours (correspond egalement au nombre de clients connectes)
pthread_mutex_t g_mutex = PTHREAD_MUTEX_INITIALIZER;//Utilisation des mutex pour gerer les variables partagees par les threads
//Liste doublement chainees
struct Clients{
	int data;//correspond au socket du client
	int nameset;//flag si le pseudo a ete mis en place (1 si mis en place 0 sinon)
	pthread_mutex_t mutex;
	char name[16];//le pseudonyme du client
	struct Clients *next;//pointe vers le prochain client de la liste
	struct Clients *prev;//pointe vers le precedent client de la liste
};
struct Clients *head;//1ere position de la liste
struct Clients *current_pos;//donne la position du nieme client dans la liste
//Fonction pour concatener s2 a la fin de s1
char *concat(const char *s1, const char *s2)
{
        char *result = malloc(strlen(s1)+strlen(s2) + 1);
	if(result == NULL){
		fprintf(stderr,"Erreur d'allocation\n");
		exit(EXIT_FAILURE);
	}
	strcpy(result,s1);
	strcat(result,s2);
	return result;
}

int cree_socket_tcp_ip()
{
	 int yes = 1;
         int sock;
         struct sockaddr_in adresse;
         if ((sock = socket(PF_INET, SOCK_STREAM, 0)) < 0) { // IP et TCP
                 fprintf(stderr, "Erreur socket\n");
                 return -1;
         }
         if(setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1) {//Permet de reutiliser immediatement la socket apres l'avoir fermer
                        fprintf(stderr,"Erreur socket options\n");
                        return -1;
         }

	 memset(&adresse, 0, sizeof(struct sockaddr_in)); // initialisation à 0
	 adresse.sin_family = PF_INET; // Protocol IP
	 adresse.sin_port = htons(33016); // port 33016
	 inet_aton("127.0.0.1", &adresse.sin_addr); // IP localhost
	 if (bind(sock, (struct sockaddr*) &adresse, sizeof(struct sockaddr_in)) < 0)//Test de bind
	 {
		 if(close(sock) < 0){
			 fprintf(stderr,"Erreur close\n");
		 }
		 fprintf(stderr, "Erreur bind\n");
		 return -1;
	 }
	 return sock;
}
int affiche_adresse_socket(int sock)
{
	 struct sockaddr_in adresse;
	 socklen_t longueur;

	 longueur = sizeof(struct sockaddr_in);

	 if (getsockname(sock, (struct sockaddr*)&adresse, &longueur) < 0)
	 {
		fprintf(stderr, "Erreur getsockname\n");
	 	return -1;
	 }
	 printf("IP = %s, Port = %u\n", inet_ntoa(adresse.sin_addr),
	 ntohs(adresse.sin_port));

	 return 0;
}

void *traite_connection(void *point)
{
	 int BUFFER_SIZE = 256;
	 struct sockaddr_in adresse;
	 socklen_t lg;
	 char bufferR[BUFFER_SIZE-1];
	 bzero(bufferR, BUFFER_SIZE-1);
	 char name[16];
	 int intBuffer[1];//Utilise pour dire au client si le pseudo est valide (1 si ok 0 sinon)
	 int n;
	 int flag;
	 struct Clients *client = (struct Clients*) point;
	 int socket = client->data;	 
	 //On verifie si le nom que l'utilisateur a choisi n'est pas deja utilise
	 while(1){
		struct Clients *p = head;
	 	if(read(socket,name,16) < 0){
			fprintf(stderr,"Erreur de lecture socket communication\n");
			exit(EXIT_FAILURE);
		}
		if(pthread_mutex_lock(&g_mutex) != 0){
			fprintf(stderr,"Erreur lock mutex\n");
			exit(EXIT_FAILURE);
		}
		//Parcours de la liste
		while(p != NULL){
			if(strcmp(p->name,name) == 0){//Si le pseudo est deja utilise on sort de la boucle
				break;
			}
			p = p->next;
		}
		if(pthread_mutex_unlock(&g_mutex) != 0){
			fprintf(stderr,"Erreur unlock mutex\n");
			exit(EXIT_FAILURE);
		}
		//Si on a parcouru toute la liste le pseudo est non utilise
		if(p == NULL){
         		strcpy(client->name,name);//On copie le pseudo dans le champ name du client
			intBuffer[0] = 1;//1 pseudo non utilise
			client->nameset = 1;//1 pseudo mis en place
			if(pthread_mutex_lock(&client->mutex) != 0){
				fprintf(stderr,"Erreur lock mutex\n");
				exit(EXIT_FAILURE);
			}
			if(write(socket,intBuffer,sizeof(int)) < 0){//pseudo non utilise on envoie 1 au client
				fprintf(stderr,"Erreur d'ecriture socket communication\n");
				exit(EXIT_FAILURE);
			}
			if(pthread_mutex_unlock(&client->mutex) != 0){
				fprintf(stderr,"Erreur unlock mutex\n");
				exit(EXIT_FAILURE);
			}
			break;
		}
		else{
			intBuffer[0] = 0;//pseudo deja utilise on envoie 0 au client
			if(pthread_mutex_lock(&client->mutex) != 0){
				fprintf(stderr,"Erreur mutex lock\n");
				exit(EXIT_FAILURE);
			}
			if(write(socket,intBuffer,sizeof(int)) < 0){//pseudo deja utilise on envoie 0 au client
				fprintf(stderr,"Erreur ecriture socket de communication\n");
				exit(EXIT_FAILURE);
			}
			if(pthread_mutex_unlock(&client->mutex) != 0){
				fprintf(stderr,"Erreur mutex unlock\n");
				exit(EXIT_FAILURE);
			}
		}
	 }

	 
	 lg = sizeof(struct sockaddr_in);
	 if (getpeername(socket, (struct sockaddr*) &adresse, &lg) < 0) {
		 fprintf(stderr, "Erreur getpeername\n");
		 exit(EXIT_FAILURE);
	 }

	 printf("\x1b[92mConnexion :\x1b[0m locale IP = \x1b[94m%s\x1b[0m, Port = \x1b[94m%u\x1b[0m, "\
			 "Name = \x1b[94m%s\x1b[0m\n", inet_ntoa(adresse.sin_addr),ntohs(adresse.sin_port),client->name);

	//Tant que le client est connecte (tant qu'il n'envoie pas le mot cle Exit) 
        while(1){	
                bzero(bufferR,BUFFER_SIZE);
		if((n= read(socket, bufferR, BUFFER_SIZE)) < 0){
			fprintf(stderr,"Erreur de lecture socket communication\n");
			exit(EXIT_FAILURE);
		}
		if((flag = strncmp("Exit",bufferR,4))== 0 || (n == 0)){	
                        printf("\x1b[31mDeconnexion :\x1b[0m locale IP = \x1b[94m%s\x1b[0m, Port = \x1b[94m%u\x1b[0m, "\
					"Name = \x1b[94m%s\x1b[0m\n", inet_ntoa(adresse.sin_addr),ntohs(adresse.sin_port),client->name);
			break; 
		 }
		 //Si c'est le premier client la liste des clients est constituee de lui-meme
		if((client == head) && (client->next == NULL)){
			if (strncmp(bufferR,"!list",5) == 0){
				char *s = "liste des clients: [";
				s = concat(s, client->name);
				s = concat(s,"]");
				if(pthread_mutex_lock(&client->mutex) != 0){
					fprintf(stderr,"Erreur lock mutex\n");
					exit(EXIT_FAILURE);
				}
				if(write(socket,s,BUFFER_SIZE) < 0){
					fprintf(stderr,"Erreur ecriture socket communication\n");
					exit(EXIT_FAILURE);
				}
				if(pthread_mutex_unlock(&client->mutex) != 0){
					fprintf(stderr,"Erreur unlock mutex\n");
					exit(EXIT_FAILURE);
				}
				free(s);
			}


		}
		//Gestion des messages prives
		else if(bufferR[0] == '@'){
			struct Clients *p = head;
			char *point;
        		point = strtok(bufferR, "@");//Exemple (transforme "@John Salut" en John Salut) 	
			//On cherche si le destinataire existe
                        while(p != NULL){
                                char *s = concat("(de)",client->name);
				s = concat(s,": ");
				s = concat(s,bufferR);
				//Pour tous les clients de la liste a part l'expediteur,
				// on verifie si l'username du destinataire specifie par l'utilisateur(@username)) existe
				// la 3e condition sert a verifier si on a bien @john salut et pas @johnsalut
                                if((p->data != client->data) && (strncmp(point,p->name,strlen(p->name))== 0) && (point[strlen(p->name)] == ' '))
                                {
					if(pthread_mutex_lock(&p->mutex) != 0){
						fprintf(stderr,"Erreur lock mutex\n");
						exit(EXIT_FAILURE);
					}
                                        if(write(p->data,s, BUFFER_SIZE) < 0){//Si, on le trouve on sort de la boucle et on envoie le message au destinataire
						fprintf(stderr,"Erreur ecriture socket communication\n");
						exit(EXIT_FAILURE);
					}
					if(pthread_mutex_unlock(&p->mutex) != 0){
						fprintf(stderr,"Erreur unlock mutex\n");
						exit(EXIT_FAILURE);
					}
					break;
                                }
                                p = p->next;
				//Si aucun des clients ne correspond a l'username specifie par l'utilisateur
				//On envoie a l'expéditeur un message d'erreur
				if (p == NULL){
					char *c = concat("Message non envoye(utilisateur introuvable): ",bufferR);
					if(pthread_mutex_lock(&client->mutex) != 0){
						fprintf(stderr,"Erreur lock mutex\n");
						exit(EXIT_FAILURE);
					}
					if(write(client->data,c,BUFFER_SIZE) < 0){
						fprintf(stderr,"Erreur ecriture socket communication\n");
						exit(EXIT_FAILURE);
					}
					if(pthread_mutex_unlock(&client->mutex) != 0){
						fprintf(stderr,"Erreur unlock mutex\n");
						exit(EXIT_FAILURE);
					}
					free(c);
				}
                                free(s);
                 	}
		 }
		//Envoie de la liste quand il y a plus d'un utilisateur
		else if(strncmp(bufferR,"!list",5) == 0){
			struct Clients *p = head;
			char *s = "liste des clients: [";
			while(p != NULL){
				if(p->nameset == 1){
					s=concat(s,p->name);
				}
				p = p->next;
				if((p != NULL)&&(p->nameset ==1)){
					s = concat(s,", ");
				}
			}
			
			s = concat(s,"]");
			if(pthread_mutex_lock(&client->mutex) != 0){
				fprintf(stderr,"Erreur lock mutex\n");
				exit(EXIT_FAILURE);
			}
		       	if(write(socket,s,BUFFER_SIZE) < 0){
				fprintf(stderr,"Erreur ecriture socket communication\n");
				exit(EXIT_FAILURE);
			}
			if(pthread_mutex_unlock(&client->mutex) != 0){
				fprintf(stderr,"Erreur unlock mutex\n");
				exit(EXIT_FAILURE);
			}
			free(s);
		}/*else if*/
		 		       
		//Gestion des messages globaux
		else{
		struct Clients *p = head;
			 //On parcourt toute la liste (jusqu'a ce que p pointe vers un element nul)
			 //et on envoie le message à chacun des clients à part l'expediteur
			while(p != NULL){
				char *s = concat(client->name,": ");
				s = concat(s,bufferR);
				if((p->data != client->data) && (p->nameset == 1))//On verifie egalement que l'utilisateur a defini son nom
				{
					if(pthread_mutex_lock(&p->mutex) != 0){
						fprintf(stderr,"Erreur lock mutex\n");
						exit(EXIT_FAILURE);
					}
					if(write(p->data,s, BUFFER_SIZE) < 0){
						fprintf(stderr,"Erreur ecriture socket communication\n");
						exit(EXIT_FAILURE);
					}
					pthread_mutex_unlock(&p->mutex);
				}
				p = p->next;
				free(s);
			}/*while*/
		}/*else*/
	}/*while*/
	//Gere la deconnexion du client
	//Si plusieurs clients se deconnectent en meme temps cela peut creer des problemes lorsqu'ils essayent de modifier thread_counter en meme temps
	//Pour remedier à ca, on utilise mutex (seul 1 thread peut acceder et modifier la variable à la fois)
	if(pthread_mutex_lock(&g_mutex) != 0){
		fprintf(stderr,"Erreur lock mutex\n");
		exit(EXIT_FAILURE);
	}
	thread_counter--;
	if(close(socket) < 0){
		fprintf(stderr,"Erreur close socket communication\n");//On ferme la socket de communication	
		exit(EXIT_FAILURE);
	}
	bzero(client->name,sizeof(client->name));//Remmplace le pseudo par des zeros pour rendre le pseudo utilise disponible
	//On ne peut pas à proprement parler supprimer un client de notre liste, il faut redefinir les liens (next, prev)
	//pour les clients adjacents au client qui s'est deconnecte
	if(client == head){
		if(client->next != NULL){
			client->next->prev = NULL;
			head = client->next;
		}
		else{
			head=NULL;
		}
	}
	else if (client == current_pos){
		current_pos->prev->next = NULL;
		current_pos = current_pos->prev;
		}
	 else{
		 client->next->prev = client->prev;
		 client->prev->next = client->next;
	 }
	 if(pthread_mutex_unlock(&g_mutex) != 0){
		 fprintf(stderr,"Erreur unlock mutex\n");
		 exit(EXIT_FAILURE);
	 }
	 free(client);//On libere l'espace memoire qui etait occupe par client
	 pthread_exit(0);//Enfin on termine le thread


}
void liste(int signal){
	//On parcourt la liste et on incrémente le compteur pour trouver le nombre d'utilisateurs présent sur le serveur
        int counter = 0;
        struct Clients *p = head;
        while(p != NULL){
        	counter++;
                p = p->next;
        }
	if(counter > 1){
        	printf("Il y a actuellement \x1b[94m%d/%d\x1b[0m utilisateurs\n",counter,MAX_THREADS);
	}
	else{
		printf("Il y a actuellement \x1b[94m%d/%d\x1b[0m utilisateur\n",counter,MAX_THREADS);
	}
        fflush(stdout);
}
void turnoff(int signal){
	char server_bye[256] = "12mrné)jlflknémqln&)à1KN2)çj&fnin&oil";//message que le serveur envoie à tous les clients lorsqu'il s'éteint
        struct Clients *p = head;
	//Etape 1 : On envoie le message à tous les clients
        while(p != NULL){
        	if(write(p->data,server_bye,43) < 0){
			fprintf(stderr,"Erreur ecriture socket communication\n");
			exit(EXIT_FAILURE);
		}
                p = p->next;
        }
	//Etape 2 : On ferme les sockets et on libere l'espace memoire qui etait occupe par chaque client dans notre liste
        while(head != NULL){
                if(close(head->data) < 0){
			fprintf(stderr,"Erreur fermeture socket de communication\n");
			exit(EXIT_FAILURE);
		}
                p = head;
                head = head->next;
                free(p);
        }
	//Etape 3 : On ferme la socket du serveur et on sort du programme
        printf("\rShutting Down...\n");
        fflush(stdout);
        if(close(g_server_socket) < 0){
		fprintf(stderr,"Erreur fermeture socket serveur\n");
		exit(EXIT_FAILURE);
	}
        exit(0);

}

int main(){
	signal(SIGUSR1,liste);//Affiche le nombre d'utilisateurs presents sur le serveur
	signal(SIGUSR2,turnoff);//Eteint le serveur 
	signal(SIGINT,turnoff);//Eteint le serveur avec ctrl+c
	if((g_server_socket = cree_socket_tcp_ip()) == -1){//Creation de la socket du serveur
		return -1;
	}
	if(listen(g_server_socket,5) < 0){
		fprintf(stderr,"Erreur listening\n");
		return -1;
	}
	int client_socket;
	int intBuffer[1];
	pthread_t traite_connection_t[MAX_THREADS];
	printf("ID du processus serveur : \x1b[94m%d\x1b[0m\n", getpid());//affiche le pid du serveur pour envoyer des signaux depuis le terminal
	printf("Eteindre le serveur et arreter les connexions : Envoyer au serveur un SIGUSR2 ou CTRL+C\n");
	printf("Afficher le nombre d'utilisateurs présents sur le serveur : Envoyer au serveur un SIGUSR1\n");	
	printf("En attente de connexion...\n");
	while(1){
		if((client_socket = accept(g_server_socket,NULL, NULL)) < 0){
			fprintf(stderr,"Erreur connexion du client\n");
			return -1;
		}
		//On verifie si on peut accueilir le client
		//Si on ne peut pas, on envoie lui envoie un message et on ferme la socket de connexion
		if(pthread_mutex_lock(&g_mutex) != 0){
			fprintf(stderr,"Erreur lock mutex\n");
			return -1;
		}
		if(thread_counter == MAX_THREADS){
			intBuffer[0] = 0;//correspond au serveur complet
		        if(write(client_socket,intBuffer,sizeof(int)) < 0){
				fprintf(stderr,"Erreur d'ecriture sur la socket communication\n");
				return -1;
			}	
			if(close(client_socket) < 0){
				fprintf(stderr,"Erreur fermeture socket communication\n");
				return -1;
			}
		}
		else{
			intBuffer[0] = 1;//correspond au serveur avec de la place
			if(write(client_socket,intBuffer,sizeof(int)) < 0){
				fprintf(stderr,"Erreur d'ecriture\n");
				return -1;
			}
			struct Clients *client = (struct Clients*)malloc(sizeof(struct Clients));
			if (client == NULL){
				fprintf(stderr,"Erreur d'allocation de memoire\n");
				return -1;
			}
			client->data = client_socket;
			client->next = NULL;
			//Si il n'y a pas de client head pointe vers client
			if(head == NULL){
				head = client;
			}
			//Sinon le nouveau client pointe vers le dernier client qui s'est connecte
			else{
				client->prev = current_pos;
				current_pos->next = client;
			}
			//Et on reajuste la position du nieme client
			current_pos = client;

			if(pthread_create(&traite_connection_t[thread_counter],NULL,traite_connection,current_pos)!=0){
				fprintf(stderr,"Erreur de creation de thread\n");
				return -1;
			}
			//Acces 
			else{
				thread_counter++;
			}

		}/*else*/
		if(pthread_mutex_unlock(&g_mutex) != 0){
			fprintf(stderr,"Erreur unlock mutex\n");
			return -1;
		}

	}/*while*/


	return 0;
}
