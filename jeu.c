#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>


// Paramètres du jeu
#define LARGEUR_MAX 7 		// nb max de fils pour un noeud (= nb max de coups possibles)
#define HAUTEUR_MAX 6

#define TEMPS 5		// temps de calcul pour un coup avec MCTS (en secondes)

// macros
#define AUTRE_JOUEUR(i) (1-(i))
#define min(a, b)       ((a) < (b) ? (a) : (b))
#define max(a, b)       ((a) < (b) ? (b) : (a))

// Critères de fin de partie
typedef enum {NON, MATCHNUL, ORDI_GAGNE, HUMAIN_GAGNE } FinDePartie;

// Definition du type Etat (état/position du jeu)
typedef struct EtatSt {
    int joueur; // à qui de jouer ?
    char plateau[HAUTEUR_MAX][LARGEUR_MAX];
} Etat;

// Définition du type Coup
typedef struct {
    int colonne;
} Coup;
// Definition du type Noeud
typedef struct NoeudSt {

    int joueur; // joueur qui a joué pour arriver ici
    Coup * coup;   // coup joué par ce joueur pour arriver ici

    Etat * etat; // etat du jeu

    struct NoeudSt * parent;
    struct NoeudSt * enfants[LARGEUR_MAX]; // liste d'enfants : chaque enfant correspond à un coup possible
    int nb_enfants;	// nb d'enfants présents dans la liste

    // POUR MCTS:
    int nb_victoires;
    int nb_simus;

} Noeud;

// Copier un état
Etat * copieEtat(Etat *src) {
    Etat *etat = (Etat *)malloc(sizeof(Etat));
    etat->joueur = src->joueur;

    for (int i = 0; i < HAUTEUR_MAX; i++)
        for (int j = 0; j < LARGEUR_MAX; j++)
            etat->plateau[i][j] = src->plateau[i][j];

    return etat;
}

// Etat initial
Etat * etat_initial(void) {
    Etat *etat = (Etat *)malloc(sizeof(Etat));
    etat->joueur = 0;

    for (int i = 0; i < HAUTEUR_MAX; i++)
        for (int j = 0; j < LARGEUR_MAX; j++)
                etat->plateau[i][j] = ' ';

    return etat;
}

void afficheJeu(Etat *etat) {
    printf("\n");
    for (int i = 0; i < HAUTEUR_MAX; i++) {
        for (int j = 0; j < LARGEUR_MAX; j++)
            printf("| %c ", etat->plateau[i][j]);
        printf("|\n");
    }
    printf("-----------------------------\n");
    for (int i = 0; i < LARGEUR_MAX; i++)
        printf("  %d ", i);
    printf("\n");
}

// Nouveau coup
Coup * nouveauCoup(int colonne) {
    Coup *coup = (Coup *)malloc(sizeof(Coup));
    coup->colonne = colonne;
    return coup;
}

// Demander à l'humain quel coup jouer
Coup * demanderCoup() {
    int colonne;
    printf("\nDans quelle colonne jouer ? ");
    scanf("%d", &colonne);
    return nouveauCoup(colonne);
}

// Modifier l'état en jouant un coup
int jouerCoup(Etat *etat, Coup *coup) {
    for (int i = HAUTEUR_MAX - 1; i >= 0; i--) {
        if (etat->plateau[i][coup->colonne] == ' ') {
            etat->plateau[i][coup->colonne] = etat->joueur ? 'O' : 'X';
            etat->joueur = AUTRE_JOUEUR(etat->joueur);
            return 1; // Coup valide
        }
    }
    return 0; // Coup invalide
}
Noeud * nouveauNoeud (Noeud * parent, Coup * coup ) {
    Noeud * noeud = (Noeud *)malloc(sizeof(Noeud));

    if ( parent != NULL && coup != NULL ) {
        noeud->etat = copieEtat ( parent->etat );
        jouerCoup ( noeud->etat, coup );
        noeud->coup = coup;
        noeud->joueur = AUTRE_JOUEUR(parent->joueur);
    }
    else {
        noeud->etat = NULL;
        noeud->coup = NULL;
        noeud->joueur = 0;
    }
    noeud->parent = parent;
    noeud->nb_enfants = 0;

    // POUR MCTS:
    noeud->nb_victoires = 0;
    noeud->nb_simus = 0;


    return noeud;
}
// Retourne une liste de coups possibles à partir d'un état
Coup ** coups_possibles(Etat *etat) {
    Coup **coups = (Coup **)malloc((1 + LARGEUR_MAX) * sizeof(Coup *));
    int k = 0;

    for (int i = 0; i < LARGEUR_MAX; i++) {
        if (etat->plateau[0][i] == ' ')
            coups[k++] = nouveauCoup(i);
    }

    coups[k] = NULL;
    return coups;
}



// Ajouter un enfant à un parent en jouant un coup
// retourne le pointeur sur l'enfant ajouté
Noeud * ajouterEnfant(Noeud * parent, Coup * coup) {
    Noeud * enfant = nouveauNoeud (parent, coup ) ;
    parent->enfants[parent->nb_enfants] = enfant;
    parent->nb_enfants++;
    return enfant;
}

void freeNoeud ( Noeud * noeud) {
    if ( noeud->etat != NULL)
        free (noeud->etat);

    while ( noeud->nb_enfants > 0 ) {
        freeNoeud(noeud->enfants[noeud->nb_enfants-1]);
        noeud->nb_enfants --;
    }
    if ( noeud->coup != NULL)
        free(noeud->coup);

    free(noeud);
}
// Test si l'état est un état terminal
// Test si l'état est un état terminal
FinDePartie testFin(Etat *etat) {
    for (int i = 0; i < HAUTEUR_MAX; i++) {
        for (int j = 0; j < LARGEUR_MAX; j++) {
            char joueur = etat->plateau[i][j];
            if (joueur != ' ') {
                // Vérifier les alignements dans toutes les directions
                if (j + 3 < LARGEUR_MAX &&
                    (etat->plateau[i][j + 1] == joueur) &&
                    (etat->plateau[i][j + 2] == joueur) &&
                    (etat->plateau[i][j + 3] == joueur))
                    return joueur == 'O' ? ORDI_GAGNE : HUMAIN_GAGNE;

                if (i + 3 < HAUTEUR_MAX) {
                    if ((etat->plateau[i + 1][j] == joueur) &&
                        (etat->plateau[i + 2][j] == joueur) &&
                        (etat->plateau[i + 3][j] == joueur))
                        return joueur == 'O' ? ORDI_GAGNE : HUMAIN_GAGNE;

                    if (j + 3 < LARGEUR_MAX &&
                        (etat->plateau[i + 1][j + 1] == joueur) &&
                        (etat->plateau[i + 2][j + 2] == joueur) &&
                        (etat->plateau[i + 3][j + 3] == joueur))
                        return joueur == 'O' ? ORDI_GAGNE : HUMAIN_GAGNE;

                    if (j - 3 >= 0 &&
                        (etat->plateau[i + 1][j - 1] == joueur) &&
                        (etat->plateau[i + 2][j - 2] == joueur) &&
                        (etat->plateau[i + 3][j - 3] == joueur))
                        return joueur == 'O' ? ORDI_GAGNE : HUMAIN_GAGNE;
                }
            }
        }
    }

    for (int j = 0; j < LARGEUR_MAX; j++) {
        if (etat->plateau[0][j] == ' ')
            return NON; // Il reste des cases vides, le jeu n'est pas fini
    }

    return MATCHNUL; // Toutes les cases sont remplies, c'est un match nul
}
double calculerUCT(Noeud *noeud) {
    if (noeud->nb_simus == 0) {
        return INFINITY; // Utiliser une valeur élevée pour les nœuds non explorés
    }

    double exploration = 1.41; // Paramètre d'exploration, à ajuster si nécessaire
    double exploitation = (double)noeud->nb_victoires / (double)noeud->nb_simus;

    return exploitation + exploration * sqrt(log(noeud->parent->nb_simus) / (double)noeud->nb_simus);
}

Noeud *selectionnerNoeud(Noeud *racine) {
    Noeud *noeud = racine;

    while (noeud->nb_enfants > 0) {
        double meilleur_uct = -1.0;
        Noeud *meilleur_enfant = NULL;

        for (int i = 0; i < noeud->nb_enfants; i++) {
            double uct = calculerUCT(noeud->enfants[i]);

            if (uct > meilleur_uct) {
                meilleur_uct = uct;
                meilleur_enfant = noeud->enfants[i];
            }
        }

        noeud = meilleur_enfant;
    }

    return noeud;
}

void expansion(Noeud *noeud) {
    Coup **coups = coups_possibles(noeud->etat);
    int k = 0;

    while (coups[k] != NULL) {
        ajouterEnfant(noeud, coups[k]);
        k++;
    }

    free(coups);
}

int simulation(Noeud *noeud) {
    Etat *etat_simule = copieEtat(noeud->etat);

    while (testFin(etat_simule) == NON) {
        Coup **coups = coups_possibles(etat_simule);
        int k = rand() % (LARGEUR_MAX + 1);

        while (coups[k] == NULL) {
            k = rand() % (LARGEUR_MAX + 1);
        }

        jouerCoup(etat_simule, coups[k]);
        free(coups);
    }

    FinDePartie resultat = testFin(etat_simule);
    free(etat_simule);

    if (resultat == HUMAIN_GAGNE) {
        return 0;
    } else if (resultat == ORDI_GAGNE) {
        return 1;
    } else {
        return 0.5; // Match nul
    }
}

void retropropagation(Noeud *noeud, double score) {
    while (noeud != NULL) {
        noeud->nb_simus++;
        noeud->nb_victoires += score;
        noeud = noeud->parent;
    }
}
// Calcule et joue un coup de l'ordinateur avec MCTS-UCT
void ordijoue_mcts(Etat *etat, int tempsmax) {
    clock_t tic, toc;
    tic = clock();
    int temps;

    Coup **coups;
    Coup *meilleur_coup;

    // Créer l'arbre de recherche
    Noeud *racine = nouveauNoeud(NULL, NULL);
    racine->etat = copieEtat(etat);

    // créer les premiers noeuds
    coups = coups_possibles(racine->etat);
    int k = 0;
    Noeud *enfant;
    while (coups[k] != NULL) {
        enfant = ajouterEnfant(racine, coups[k]);
        k++;
    }

    meilleur_coup = coups[rand() % k]; // choix aléatoire

    /* TODO: Implémenter l'algorithme MCTS-UCT pour déterminer le meilleur coup ici */

    int iter = 0;

    do {
        Noeud *noeud_selectionne = selectionnerNoeud(racine);
        expansion(noeud_selectionne);

        for (int i = 0; i < noeud_selectionne->nb_enfants; i++) {
            double score_simulation = simulation(noeud_selectionne->enfants[i]);
            retropropagation(noeud_selectionne->enfants[i], score_simulation);
        }

        toc = clock();
        temps = (int)(((double)(toc - tic)) / CLOCKS_PER_SEC);
        iter++;
    } while (temps < tempsmax);

    // Sélectionner le meilleur coup en fonction des statistiques
    double meilleur_score = -1.0;
    for (int i = 0; i < racine->nb_enfants; i++) {
        double score = (double)racine->enfants[i]->nb_victoires / (double)racine->enfants[i]->nb_simus;
        if (score > meilleur_score) {
            meilleur_score = score;
            meilleur_coup = racine->enfants[i]->coup;
        }
    }

    // Jouer le meilleur coup
    jouerCoup(etat, meilleur_coup);

    // Penser à libérer la mémoire
    freeNoeud(racine);
    free(coups);
}

int main(void) {
    Coup *coup;
    FinDePartie fin;

    // initialisation
    Etat *etat = etat_initial();

    // Choisir qui commence :
    printf("Qui commence (0 : humain, 1 : ordinateur) ? ");
    scanf("%d", &(etat->joueur));

    // boucle de jeu
    do {
        printf("\n");
        afficheJeu(etat);

        if (etat->joueur == 0) {
            // tour de l'humain
            do {
                coup = demanderCoup();
            } while (!jouerCoup(etat, coup));
        } else {
            // tour de l'Ordinateur
            ordijoue_mcts(etat, TEMPS);
        }

        fin = testFin(etat);
    } while (fin == NON);

    printf("\n");
    afficheJeu(etat);

    if (fin == ORDI_GAGNE)
        printf("** L'ordinateur a gagné **\n");
    else if (fin == MATCHNUL)
        printf("Match nul !  \n");
    else
        printf("** BRAVO, l'ordinateur a perdu  **\n");

    return 0;
}
