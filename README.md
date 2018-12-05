# Leaker
Memory leaks detection for c/ c++
il y a 2 fichiers importants    leaker.c et leaker.h
ainsi qu'un fichier de test avec le makefile  pour l'utiliser

en pratique il faut mettre le leaker.c dans le même répertoire que les fichiers sources que tu veux compiler
le leaker.h dans le même repertoire que tes fichiers .h  (s'il sont séparés des sources)

et important il faut  dans tous les fichiers (.h ou .c) où se trouve  #include <stdlib.h>

remplacer     #include <stdlib.h>    par :

#ifdef USE_LEAKER
# include "leaker.h"
#else
# include <stdlib.h>
#endif

ou bien équivalent (mais je crois que les correcteurs préfèrent cette forme)

#ifndef USE_LEAKER
# include <stdlib.h>
#else
# include "leaker.h"
#endif

cela prépare une compilation conditionnelle que le makefile va utiliser (voir dans makefile en exemple)

après il faut lancer   make  en ajoutant un flag pour valider USE_LEAKER dans les fichiers
pour cela     faire  :
        make "LEAKER=1"                                       pour lancer   all       avec le leaker ajouté
ou   make re "LEAKER=1"                                   pour lancer   re       avec le leaker ajouté

tu verra dans le makefile que le fichier leaker.c    n'est  pas inclu dans la liste des fichiers sources
    il est automatiquement ajouté si le flag   "LEAKER=1"   est placé dans la cde   make

en pratique tu peux utiliser n'importe quoi     comme "LEAKER=toto" car le makefile verifie que le flag n'est pas vide

par ailleurs le makefile est  intelligent, il peut te servir de modele pour tous tes projets
        il sépare les fichiers .o  (ainsi que les fichiers de dependances) dans un dossier obj_d  c'est plus propre  ( variable OBJ_DIR )
        il permet d'avoir les fichiers sources dans un dossier séparé    (variable   SRC_DIR)
        même chose pour les .h   (variable   INCLUDES)
    tu n'a donc pas besoin de mettre le chemin absolu ou relatif dans tes fichiers de code ...

dernier point il crée automatiquement des fichiers de dépendances
     voir les lignes 72 et 74
                $(CC) -c -MMD -MP $(CFLAGS) -o $@ $< -I$(INCLUDES)
                -include $(DEP_F)
    et la fin de la ligne 70  il y a        Makefile
                $(OBJ_F): $(OBJ_DIR)/%.o: $(SRC_DIR)/%.c  Makefile
    avec cela la recompilation ne se fera que quand les fichiers auront été modifiés (ou ton makefile modifié)


bon maintenant : comment cela fonctionne en pratique ,
 on substitue les functions malloc/free/realloc
chaque fois que tu utilise malloc
l'adresse pointer est stockée dans une liste chainée avec la taille et le fichier ainsi que la ligne
quand tu free   , il vérifie que l'adresse était valide et efface les autres infos

s'il voit des erreurs il l'indique dès qu'il peut
et à la fin de ton exécution il donne le résultat ok ou bien les erreurs...

bien sûr il ne faut pas laisser tout ce bazard dans ton rendu ,
quand tu as fini tes tests enleve les fichiers leaker.c et leaker.h et c'est tout
