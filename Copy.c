#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <unistd.h>
#include <stdio.h>
//Pour eviter des warning sur des fonctions manipulant des string
#include <string.h> //Pour str***
#include <libgen.h> //pour basename()

//Etapes 1 et 2
int copyfile(const char*input_name, const char*output_name){
	int input_des = open(input_name, O_RDONLY);
	int output_des = open(output_name, O_WRONLY|O_CREAT|O_EXCL, 0666);
	while(1){
		char buffer[4096];
		int nb_octets_lus = read(input_des, buffer, sizeof(buffer));
		//Si on n'a lu aucun caractère
		if(nb_octets_lus == 0){break;}
		if(nb_octets_lus == -1){return 1;} //Aucun octet lu : Impossible de lire le fichier source
		int pointeur = 0;
		int n = 0;
		while(nb_octets_lus>0){
			n = write(output_des, buffer+pointeur, nb_octets_lus);
			if(n==0){return 2;}// plus de place sur le disque, impossible d'écrire
			if(n==-1){return 3;}// le fichier existe déjà, write renvoie -1
			pointeur+=n;
			nb_octets_lus-=n;
		}	
	}
	//Déclaration de la structure des états du fichier
	struct stat fileStat;//Etape2
	//On récupère les états du fichier de départ
	fstat(input_des,&fileStat); //Etape2
	//On applique les permissions du fichier à celui d'arrivée
	fchmod(output_des, fileStat.st_mode);//Etape2
	close(input_des);
	close(output_des);
	return 0;
}


//Etapes 3 et 4
//Definition d'une fonction recursive terminale
int aux(const char*input_name, const char*output_name, const char*master){
	//Ouverture des fichiers
	DIR *input_dir = opendir(input_name);
	DIR *output_dir = opendir(output_name);
	//Verifications sur les fichiers
	if(input_dir== NULL){return 1;} //Fichier de depart inexistant
	if(output_dir== NULL){return 2;} //Fichier de destination inexistant

	//Path dossier source
	char *symlinkpath = input_name;
	char dossier_source [PATH_MAX+1];
	char *ptr;
	ptr = realpath(symlinkpath, dossier_source);
	if(ptr == NULL){return 6;}//On n'arrive pas a acceder au dossier source
	
	//Path dossier destination
	char *symlinkpath2 = output_name;
	char dossier_destination [PATH_MAX+1];
	char *ptr2;
	ptr2 = realpath(symlinkpath2, dossier_destination);
	if(ptr2 == NULL){return 7;}//On n'arrive pas a acceder au dossier maitre

	//On recupere le path du dossier maitre
	char *symlinkpath3 = master;
	char dossier_maitre [PATH_MAX+1];
	char *ptr3;
	ptr3 = realpath(symlinkpath3, dossier_maitre);
	if(ptr3 == NULL){return 8;}//On n'arrive pas a acceder au dossier de maitre

	//Déclaration de la structure dirent qui permet de lire le flux de données du répertoire entrant
	struct dirent *read1;


	//Tant que le repertoire n'a pas ete entierement parcouru, on continue de parcourir le repertoire
	while((read1=readdir(input_dir))!= NULL){
		
		//Verifie que l'on travaille bien sur les fichiers contenus dans le dossier source
		if ( !strcmp(read1->d_name, ".") || !strcmp(read1->d_name, "..") ){
			// On ne fait rien dans ce cas, on ne veut travailler que sur les fichiers

		} else{

			//Etape 4
		
			//On se deplace dans le dossier source
			int depart = chdir(dossier_source);
			if(depart == -1){return 6;}//On n'arrive pas a accerder au dossier source	
			int input_des = open(read1->d_name, O_RDONLY);
			
			//Déclaration de la structure des états du fichier
			struct stat fileStat;//Etape2
			//On récupère les états du fichier de départ
			fstat(input_des,&fileStat); //Etape2
			
			//Determine si le fichier est un dossier. Si c'est le cas, alors effectue cette boucle
			if(read1->d_type == DT_REG) {
			//Effectue cette boucle s'il s'agit d'un fichier standard
			//On lit le fichier a copier en premier lieux			
			char buffer[4096];
			int nb_octets_lus = read(input_des, buffer, sizeof(buffer));
			if(nb_octets_lus == 0){break;}//Si on n'a lu aucun caractère
			if(nb_octets_lus == -1){return 3;} //Aucun octet lu : Impossible de lire le fichier source
			
			int pointeur = 0;
			//On se deplace dans le dossier maitre
			int debut1 = chdir(dossier_maitre);
			if(debut1 == -1){return 7;}//On n'arrive pas a revenir au dossier maitre
			
			//On se deplace dans le fichier de destination
			int dest = chdir(dossier_destination);
			if(dest == -1){return 8;}//Le programme n'arrive pas acceder au dossier destination 

			int output_des = open(read1->d_name, O_WRONLY|O_CREAT|O_EXCL, 0666);
			//Boucle while pour ecrire dans le dossier de destination
			while(nb_octets_lus>0){
				int n = 0;
				n = write(output_des, buffer+pointeur, nb_octets_lus);
				if(n==0){return 4;}// plus de place sur le disque, impossible d'écrire
				if(n==-1){return 5;} // le fichier existe déjà, write renvoie -1	
				pointeur+=n;
				nb_octets_lus-=n;
			}
			
			//On applique les permissions du fichier à celui d'arrivée
			fchmod(output_des, fileStat.st_mode);//Etape2
			
			//On fermes les fichiers
			close(input_des);
			close(output_des);
						
			} else if(read1->d_type == DT_DIR){
				int deplacement = chdir(dossier_source);
				if(deplacement == -1){return 10;}//On n'arrive pas a lire le sous dossier source

				//Path du sous-dossier source
				char cwd2[10000];
				getcwd(cwd2, sizeof(cwd2));
				char nouvelle_source[1000];
				strcat(nouvelle_source, cwd2);
				strcat(nouvelle_source, "/");
				strcat(nouvelle_source, read1->d_name);

				//Effectue cette boucle dans le cas d'un dossier
				//On ouvre le dossier à copier
				DIR *input_dir2 = opendir(read1->d_name);
				if(input_dir2 == NULL){return 10;}//Le programme n'arrive pas a acceder au sous-dossier du dossier source

				//On se deplace dans le fichier de destination
				int deb = chdir(dossier_destination);
				if(deb == -1){return 8;}//Le programme n'arrive pas acceder au dossier destination
				
				//On cree le fichier dans le dossier de destination, avec les informations du dossier source
				int output_des2 = mkdir(read1->d_name, fileStat.st_mode);
				if(output_des2 == -1){return 12;}//Echec de la creation du dossier de copie

				//On applique les permissions du dossier à celui de destination
				fchmod(output_des2, fileStat.st_mode);//Etape2

				//Path du sous-dossier de destination
				char cwd[500];
				getcwd(cwd, sizeof(cwd));
				char destina[1000];
				strcat(destina, cwd);
				strcat(destina, "/");
				strcat(destina, read1->d_name);

				//On ouvre le dossier dans lequel on doit coller les donnees
				DIR *output_dir2 = opendir(destina);
				if(output_dir2 == NULL){return 11;}//Le programme n'arrive pas a acceder au sous-dossier du dossier de destination
				
				//On se deplace dans le dossier maitre
				int depart4 = chdir(dossier_maitre);
				if(depart4 == -1){return 7;}//On n'arrive pas a revenir au dossier maitre	
				
				//On se redéplace dans le fichier source
				int deb3 = chdir(dossier_source);
				if(deb3 == -1){return 6;}//On n'arrive pas à revenir au dossier précédent
				
				//On applique la recursion sur le sous-dossier source et sur le sous-dossier de destination
				aux(nouvelle_source, destina, dossier_maitre);
			
				//On fermes les dossiers source et destination
				closedir(input_dir2);
				closedir(output_dir2);
			} 
		}
	}
	//On referme les dossiers	
	closedir(input_dir);
	closedir(output_dir);
	return 0;
}

int copyDirectory(const char*input_name, const char*output_name){
	const char*master = dirname(input_name);
	int res = aux(input_name, output_name, master);
	return res;
}

int main(){
	//PARTIES 1 ET 2
	/*int errcode = copyfile("titi", "toto");
	if (errcode==1) {
		printf("La copie a échouée: impossible de lire le fichier source\n");
	} else if (errcode==2) {
		printf("La copie a échouée: plus de place pour écrire\n");
	} else if (errcode==3) {
		printf("La copie a échouée: le fichier existe déjà\n");
	}*/
	
	//PARTIE 3 et 4
	int errcode2 = copyDirectory("test1", "test3");
	if(errcode2==0){ printf("La copie de repertoire a Reussi\n");
	} else if(errcode2==1){printf("La copie de repertoire a echouee: Le fichier source est inexistant\n");
	} else if(errcode2==2){printf("La copie de repertoire a echouee: Le fichier de destination est inexistant\n");
	} else if(errcode2==3){printf("La copie de repertoire a echouee: Impossible de lire le ficher source\n");
	} else if(errcode2==4){printf("La copie de repertoire a echouee: Plus de place disponible pour ecrire\n");
	} else if(errcode2==5){printf("La copie de repertoire a echouee: Le fichier de destination existe deja\n");
	} else if(errcode2==6){printf("La copie de repertoire a echouee: Le programme n'arrive pas a acceder au dossier source\n");
	} else if(errcode2==7){printf("La copie de repertoire a echouee: Le programme n'arrive pas a revenir au dossier maitre\n");
	} else if(errcode2==8){printf("La copie de repertoire a echouee: Le programme n'arrive pas a acceder au dossier destination\n");
	} else if(errcode2==9){printf("La copie de repertoire a echouee: Aucun chemin detecte pour le dossier\n");
	} else if(errcode2==10){printf("La copie de repertoire a echouee: Le programme n'arrive pas a acceder au sous-dossier du dossier source\n");
	} else if(errcode2==11){printf("La copie de repertoire a echouee: Le programme n'arrive pas a acceder au sous-dossier du dossier de destination\n");
	} else if(errcode2==12){printf("La copie de repertoire a echouee: Pas de copie recursive (dest)\n");
	} else if(errcode2==13){printf("La copie de repertoire a echouee: Pas de copie recursive(source)\n");
	} else if(errcode2==14){printf("La copie de repertoire a echouee: Les noms ne correspondent pas pour l'appel recurssif\n");
	}
	return 0;
}
