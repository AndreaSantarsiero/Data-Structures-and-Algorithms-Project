/*  progetto API 2023-24  */
/* voto 30L superato (L'output è corretto | 1,403 s | 11,7 MiB) */

#include <stdio.h>
#include <stdlib.h>

#define TRUE 1
#define FALSE 0
#define MAX 256     /* numero max caratteri dei nomi di ricette e ingredienti */
#define MAXCOMANDO 26000    /* numero max caratteri dei comandi */
#define HASHMAGAZZINO 21269    /* parametro funzione di hash per database magazzino ingredienti */
#define HASHCATALOGO 26959   /* parametro funzione di hash per database catalogo ricette */






/* variabili globali */
unsigned int time=0;
unsigned int period_corriere=0, capienza_corriere=0;
int scanf_return=0;
char comando[MAXCOMANDO];
char nome_ricetta_rr[MAX];
char nome_ricetta_o[MAX];



/* database magazzino ingredienti */
typedef struct magaz{
  int quantita;   /* quantità ingrediente */
  int scadenza;   /* scadenza ingrediente */
  struct magaz *next;   /* puntatore al lotto successivo dello stesso ingrediente (con scadenza più lontana) */
} magazzino;

typedef struct datmag{
  char *nome;   /* puntatore al nome dell'ingrediente (allocato con una seconda malloc tarata sulla lunghezza del nome) */
  int scorte_ingr;   /* somma delle quantità di tutti i lotti presenti nella lista (anche quelli scaduti) */
  struct magaz *lista;   /* puntatore alla lista contenente i lotti di quell'ingrediente */
  struct datmag *next;   /* puntatore al prossimo ingrediente con lo stesso hash primario */
} database_magazzino;



/* database catalogo ricette */
typedef struct catal{
  int quantita;   /* quantità ingrediente */
  struct datmag *ingrediente;   /* puntatore al padre dell'ingrediente */
  struct catal *next;   /* puntatore all'ingrediente successivo della ricetta */
} catalogo;

typedef struct datcat{
  char *nome;   /* puntatore al nome della ricetta (allocata con una seconda malloc tarata sulla lunghezza del nome) */
  int num_ordini_sospesi;   /* contatore che segnala quanti ordini in sospeso esistono sulla ricetta puntata da questo nodo */
  int last_check;   /* istante di tempo in cui ho provato a cucinare questa ricetta per l'ultima volta */
  int max_pezzi;    /* numero max di pezzi che erano realizzabili al momento dell'ultimo check (non significa che posso ancora realizzarli tutti) */
  struct catal *lista;   /* puntatore alla lista contenente gli ingredienti della ricetta */
  struct datcat *next;   /* puntatore alla prossima ricetta con lo stesso hash primario */
} database_catalogo;



/* coda ordini pronti per il corriere o in attesa di rifornimento */
typedef struct coda_r_c{
  int istante_arrivo;   /* istante di arrivo dell'ordine */
  int pezzi;   /* numero di pezzi ordinati */
  int peso;   /* peso totale dell'ordine (somma pesi ingredienti moltiplicata per numero di pezzi ordinati) */
  struct datcat *ricetta;   /* puntatore al padre della ricetta ordinata */
  struct coda_r_c *next;   /* puntatore alla prossima ricetta in attesa di rifornimento o corriere */
} coda_rif_corr;



/* array principali usati per indirizzamento diretto hash table (conflitti risolti con chaining di nodi padre) */
database_magazzino *hash_array_magaz[HASHMAGAZZINO]={NULL};    /* array principale per indirizzamento diretto hash table magazzino ingredienti */
database_catalogo *hash_array_catal[HASHCATALOGO]={NULL};    /* array principale per indirizzamento diretto hash table catalogo ricette */

/* variabili principali usate per la gestione delle code di attesa rifornimento e corriere */
coda_rif_corr *inizio_coda_rifornimento=NULL;   /* puntatore al primo elemento coda degli ordini in attesa di rifornimento */
coda_rif_corr *fine_coda_rifornimento=NULL;   /* puntatore all'ultimo elemento coda degli ordini in attesa di rifornimento */
coda_rif_corr *inizio_coda_corriere=NULL;   /* puntatore al primo elemento coda degli ordini in attesa del corriere */
coda_rif_corr *fine_coda_corriere=NULL;   /* puntatore all'ultimo elemento coda degli ordini in attesa del corriere */



/* dichiarazione sottofunzioni */
coda_rif_corr *mergesort_time(coda_rif_corr *testa);
coda_rif_corr *mergesort_peso(coda_rif_corr *testa);
void corriere();
int checkmagaz(coda_rif_corr *temp_r_c);
void rifwakeup();
void rifornimento();
void inseriscicatal(database_catalogo *padre_catal, int i);
void aggiungi_ricetta();
void rimuovi_ricetta();
void lethimcook(coda_rif_corr *temp_r_c);
void ordine();






/* funzione main */
int main(int argc, char *argv[]){

  /* leggo periodicità e capienza del corriere */
  scanf_return=scanf("%d %d\n", &period_corriere, &capienza_corriere);


  /* gestione dei comandi */
  while(fgets(comando, MAXCOMANDO, stdin)!=NULL){

    /* ordine */
    if(comando[2]=='d'){
      ordine();
    }

    /* rifornimento */
    else if(comando[2]=='f'){
      rifornimento();
    }

    /* aggiungi_ricetta */
    else if(comando[2]=='g'){
      aggiungi_ricetta();
    }

    /* rimuovi_ricetta */
    else if(comando[2]=='m'){
      rimuovi_ricetta();
    }


    ++time;   /* passo all'istante di tempo successivo */


    /* se è arrivato il corriere lo gestisco, altrimenti non faccio nulla */
    if (!(time%period_corriere)){

      /* se ci sono ordini da caricare */
      if(inizio_coda_corriere!=NULL){
          corriere();   /* controllo se è arrivato il corriere (ed eventualmente lo gestisco) */
      }

      /* se invece non ho ordini da caricare */
      else{
        printf("camioncino vuoto\n");    /* PRINTF NECESSARIA PER IL PROGETTO */
      }
    }
  }


  return 0;
}






/* codice sottofunzioni */
coda_rif_corr *mergesort_time(coda_rif_corr *testa){

  int num_ordini=0;
  coda_rif_corr *temp=testa;
  coda_rif_corr *left=testa;
  coda_rif_corr *right=testa;


  /* conto il numero di ordini in attesa della spedizione */
  while(temp!=NULL){
    ++num_ordini;
    temp=temp->next;
  }


  /* caso base */
  if(num_ordini==1){
    return testa;
  }


  /* passo induttivo */
  else{
    num_ordini/=2;    /* divido la coda in 2 (se num_ordini è dispari, right ha un nodo in più rispetto a left) */

    /* scorro fino all'ultimo nodo di left e modifico il next a NULL in modo da separare le due metà definitivamente */
    while(num_ordini>1){
      right=right->next;
      --num_ordini;
    }
    temp=right->next;
    right->next=NULL;
    right=temp;


    /* ordino i due sottoarray left e right ricorsivamente */
    left=mergesort_time(left);
    right=mergesort_time(right);


    /* ordino il primo nodo a parte in modo da poter poi creare i collegamenti nella coda */
    if(left->istante_arrivo < right->istante_arrivo){
      temp=left;
      left=left->next;
    }
    else{
      temp=right;
      right=right->next;
    }
    testa=temp;   /* memorizzo l'indirizzo del primo nodo per la return */


    /* ordino tutti i nodi dal secondo in poi */
    while(left!=NULL && right!=NULL){
      if(left->istante_arrivo < right->istante_arrivo){
        temp->next=left;
        left=left->next;
        temp=temp->next;
      }
      else{
        temp->next=right;
        right=right->next;
        temp=temp->next;
      }
    }

    /* se terminano prima i nodi di right, smaltisco quelli di left (sono già stati ordinati tra loro) */
    while(left!=NULL){
      temp->next=left;
      left=left->next;
      temp=temp->next;
    }

    /* se invece terminano prima i nodi di left, smaltisco quelli di right (sono già stati ordinati tra loro) */
    while(right!=NULL){
      temp->next=right;
      right=right->next;
      temp=temp->next;
    }
  }


  temp->next=NULL;
  fine_coda_corriere=temp;
  return testa;
}



coda_rif_corr *mergesort_peso(coda_rif_corr *testa){

  int num_ordini=0;
  coda_rif_corr *temp=testa;
  coda_rif_corr *left=testa;
  coda_rif_corr *right=testa;


  /* conto il numero di ordini selezionati per la spedizione */
  while(temp!=NULL){
    ++num_ordini;
    temp=temp->next;
  }


  /* caso base */
  if(num_ordini==1){
    return testa;
  }


  /* passo induttivo */
  else{
    num_ordini/=2;    /* divido la coda in 2 (se num_ordini è dispari, right ha un nodo in più rispetto a left) */

    /* scorro fino all'ultimo nodo di left e modifico il next a NULL in modo da separare le due metà definitivamente */
    while(num_ordini>1){
      right=right->next;
      --num_ordini;
    }
    temp=right->next;
    right->next=NULL;
    right=temp;


    /* ordino i due sottoarray left e right ricorsivamente */
    left=mergesort_peso(left);
    right=mergesort_peso(right);


    /* ordino il primo nodo a parte in modo da poter poi creare i collegamenti nella coda */
    if(left->peso >= right->peso){
      temp=left;
      left=left->next;
    }
    else{
      temp=right;
      right=right->next;
    }
    testa=temp;   /* memorizzo l'indirizzo del primo nodo per la return */


    /* ordino tutti i nodi dal secondo in poi */
    while(left!=NULL && right!=NULL){
      if(left->peso >= right->peso){
        temp->next=left;
        left=left->next;
        temp=temp->next;
      }
      else{
        temp->next=right;
        right=right->next;
        temp=temp->next;
      }
    }

    /* se terminano prima i nodi di right, smaltisco quelli di left (sono già stati ordinati tra loro) */
    while(left!=NULL){
      temp->next=left;
      left=left->next;
      temp=temp->next;
    }

    /* se invece terminano prima i nodi di left, smaltisco quelli di right (sono già stati ordinati tra loro) */
    while(right!=NULL){
      temp->next=right;
      right=right->next;
      temp=temp->next;
    }
  }


  temp->next=NULL;
  return testa;
}



void corriere(){

  /* ordino la coda in ordine cronologico di arrivo */
  inizio_coda_corriere=mergesort_time(inizio_coda_corriere);


  /* inizialitto tutti i puntatori necessari (assumo che il corriere carichi sempre almeno un ordine) */
  coda_rif_corr *temp_r_c=inizio_coda_corriere;
  coda_rif_corr *temp_r_c_prec=inizio_coda_corriere;
  coda_rif_corr *inizio_spedizione=inizio_coda_corriere;
  coda_rif_corr *next_inizio_spedizione=inizio_coda_corriere;
  int peso_spedizione=temp_r_c->peso;
  temp_r_c=temp_r_c->next;


  /* scorro la coda degli ordini dal secondo in poi in attesa del corriere finchè ho ancora spazio sul camioncino (il primo ordine della coda è già stato caricato) */
  while(temp_r_c!=NULL){
    peso_spedizione+=temp_r_c->peso;
    if(peso_spedizione <= capienza_corriere){
      temp_r_c_prec=temp_r_c;
      temp_r_c=temp_r_c->next;
    }
    else{
      break;    /* SE SUPERO LA CAPIENZA DEL CAMIONCINO ESCO DAL WHILE */
    }
  }


  /* divido la coda in due: la lista degli ordini da spedire e la lista di quelli che rimangono in attesa del prossimo camioncino */
  inizio_coda_corriere=temp_r_c;
  temp_r_c_prec->next=NULL;


  /* carico sul camioncino in ordine di peso decrescente (se il peso è uguale, sono già in ordine crescente di istante di arrivo) finchè non finiscono gli ordini da caricare */
  inizio_spedizione=mergesort_peso(inizio_spedizione);

  while(inizio_spedizione!=NULL){

    /* carico l'ordine selezionato, decremento il contatore degli ordini in sospeso e lo dealloco dalla lista */
    printf("%d %s %d\n", inizio_spedizione->istante_arrivo, inizio_spedizione->ricetta->nome, inizio_spedizione->pezzi);    /* PRINTF NECESSARIA PER IL PROGETTO */
    --inizio_spedizione->ricetta->num_ordini_sospesi;    /* decremento il contatore nel padre della ricetta che ne impedisce la rimozione dal catalogo */
    next_inizio_spedizione=inizio_spedizione->next;
    free(inizio_spedizione);
    inizio_spedizione=next_inizio_spedizione;
  }
}





int checkmagaz(coda_rif_corr *temp_r_c){

  int min_scorte=0;
  database_catalogo *temp_catal=temp_r_c->ricetta;
  catalogo *temp_c=temp_catal->lista;
  database_magazzino *temp_magaz=NULL;
  magazzino *temp_m=NULL;

  /* resetto variabili responsabili dell'ottimizzazione di checkmagaz */
  temp_catal->last_check=time;
  temp_catal->max_pezzi=999999;


  /* scorro la lista della ricetta: per ogni ingrediente, vado a vedere se nel magazzino ho scorte sufficienti per realizzare l'intero ordine */
  while(temp_c!=NULL){

    temp_magaz=temp_c->ingrediente;   /* memorizzo il padre dell'ingrediente da controllare */
    temp_m=temp_magaz->lista;


    /* rimuovo lotti scaduti */
    while(temp_m!=NULL){
      if(temp_m->scadenza<=time){
        temp_magaz->scorte_ingr-=temp_m->quantita;    /* rimuovo la quantità del lotto scaduto dalla somma nel nodo padre */
        temp_magaz->lista=temp_m->next;    /* scorro in avanti di uno */
        free(temp_m);    /* dealloco il nodo */
        temp_m=temp_magaz->lista;
      }
      else{
        break;    /* SE TROVO UN LOTTO NON SCADUTO ESCO DAL WHILE (HO RIMOSSO TUTTI I LOTTI SCADUTI) */
      }
    }


    /* se è rimasto almeno un lotto non scaduto memorizzo il numero massimo di pezzi che riuscirei a cucinare con questo ingrediente */
    if(temp_m!=NULL){
      min_scorte=temp_magaz->scorte_ingr/temp_c->quantita;
    }


    /* se invece NON è rimasto nemmeno un lotto non scaduto, ritorna falso */
    else{
      temp_catal->max_pezzi=0;
      temp_magaz->lista=NULL;   /* aggiorno il puntatore a lista lotti del padre dell'ingrediente */
      return FALSE;
    }


    /* se trovo un ingrediente le cui scorte permetterebbero di cucinare un numero minore di pezzi rispetto a quanto già controllato, aggiorno il contatore */
    if(min_scorte < temp_catal->max_pezzi){
      temp_catal->max_pezzi=min_scorte;
    }


    /* se anche solo un ingrediente non supera i controlli, ritorna falso */
    if(min_scorte < temp_r_c->pezzi){
      return FALSE;
    }


    temp_c=temp_c->next;    /* passo al prossimo ingrediente della ricetta */
  }


  /* se invece tutti gli ingredienti necessari superano i controlli, decrementa max_pezzi e ritorna vero */
  temp_catal->max_pezzi-=temp_r_c->pezzi;
  return TRUE;
}



void rifwakeup(){

  int totale_ingr=0;
  coda_rif_corr *temp_r_c=inizio_coda_rifornimento;
  coda_rif_corr *temp_r_c_prec=inizio_coda_rifornimento;
  coda_rif_corr *temp_r_c_next=inizio_coda_rifornimento;


  /* gli ordini in attesa di rifornimento vanno smaltiti in ordine cronologico di arrivo */
  while(temp_r_c!=NULL){

    /* se esistono controlli precedenti (stesso istante di tempo) che confermano l'assenza delle scorte necessarie per completare l'ordine, ritorna falso */
    database_catalogo *temp_catal=temp_r_c->ricetta;
    if(temp_r_c->ricetta->last_check==time && temp_r_c->pezzi > temp_r_c->ricetta->max_pezzi){
      temp_r_c_prec=temp_r_c;
      temp_r_c=temp_r_c->next;    /* seleziono il prossimo ordine in attesa di rifornimento */
    }


    /* se invece non esistono, continua con i controlli */
    else{

      /* inizializzo tutti i puntatori che mi serviranno */
      catalogo *temp_c=temp_catal->lista;
      database_magazzino *temp_magaz=NULL;
      magazzino *temp_m=NULL;
      magazzino *temp_m_prec=NULL;


      /* se posso cucinare l'ordine per intero */
      if(checkmagaz(temp_r_c)==TRUE){

        /* scorro la lista della ricetta: per ogni ingrediente, rimuovo dal magazzino la quantità necessaria per prepararlo e aggiorno la variabile peso */
        while(temp_c!=NULL){

          temp_magaz=temp_c->ingrediente;   /* memorizzo il padre dell'ingrediente da controllare */


          /* rimuovo dal magazzino la quantità necessaria per preparare l'ordine */
          temp_m=temp_magaz->lista;
          temp_m_prec=temp_magaz->lista;
          totale_ingr=temp_r_c->pezzi*temp_c->quantita;
          temp_r_c->peso+=totale_ingr;
          temp_magaz->scorte_ingr-=totale_ingr;    /* rimuovo la quantità necessaria per preparare l'ordine dalla somma nel nodo padre */


          /* finchè ancora necessito di questo ingrediente */
          while(totale_ingr>0){

            /* se la quantità che mi serve è maggiore o uguale di quella contenuta nel lotto, consumerò tutto il lotto (quindi dealloco e avanzo al prossimo) */
            if(totale_ingr >= temp_m->quantita){
              totale_ingr-=temp_m->quantita;
              temp_m=temp_m->next;    /* scorro in avanti di uno */
              free(temp_m_prec);    /* dealloco il nodo */
              temp_m_prec=temp_m;    /* scorro in avanti di uno anche in temp_m_prec */
            }

            /* se invece la quantità che mi serve è minore di quella contenuta nel lotto, consumerò parte del lotto (quindi decremento e NON avanzo) */
            else{
              temp_m->quantita-=totale_ingr;
              totale_ingr=0;    /* questo assegnamento segnala che posso uscire dal ciclo */
            }
          }


          temp_magaz->lista=temp_m;   /* aggiorno il puntatore del padre di quell'ingrediente */
          temp_c=temp_c->next;    /* passo al prossimo ingrediente della ricetta */
        }


        /* quando ho terminato di prelevare tutti gli ingredienti dal magazzino, sposto l'ordine dalla coda rifornimento alla coda per il corriere */
        temp_r_c_next=temp_r_c->next;    /* memorizzo in un altra variabile l'indirizzo del prossimo ordine in attesa di rifornimento */


        /* se ho cucinato il primo ordine, aggiorno inizio_coda_rifornimento (sono sicuro che temp_r_c=!NULL) */
        if(temp_r_c==inizio_coda_rifornimento){
          inizio_coda_rifornimento=inizio_coda_rifornimento->next;
          temp_r_c_prec=inizio_coda_rifornimento;
        }

        /* altrimenti, scollego temp_r_c dalla coda per il rifornimento */
        else{
          temp_r_c_prec->next=temp_r_c->next;

          /* se ho cucinato l'ultimo ordine, aggiorno fine_coda_rifornimento */
          if(temp_r_c->next==NULL){
            fine_coda_rifornimento=temp_r_c_prec;
          }
        }


        /* se la coda per il corriere è vuota */
        if(inizio_coda_corriere==NULL){
          inizio_coda_corriere=temp_r_c;
          temp_r_c->next=NULL;
          fine_coda_corriere=temp_r_c;
        }


        /* se invece la coda NON è vuota */
        else{
          fine_coda_corriere->next=temp_r_c;
          temp_r_c->next=NULL;
          fine_coda_corriere=temp_r_c;
        }


        temp_r_c=temp_r_c_next;    /* seleziono il prossimo ordine in attesa di rifornimento */
      }


      /* se invece NON posso cucinare l'ordine per intero, lo lascio nella coda di attesa rifornimento e passo al prossimo */
      else{
        temp_r_c_prec=temp_r_c;
        temp_r_c=temp_r_c->next;    /* seleziono il prossimo ordine in attesa di rifornimento */
      }
    }
  }
}



void rifornimento(){

  unsigned int hash=0, i=13, fattore=1, j=1;   /* i=13 perchè considero già letta la stringa iniziale "rifornimento " */


  /* leggo tutto il comando fino a che non è finito */
  while(comando[i]!='\0'){

    hash=0;
    j=1;

    /* calcolo funzione hash del nome dell'ingrediente */
    while(comando[i]!=' '){
      hash+=((int)comando[i]*j);
      ++i;
      ++j;
    }
    char *nome_ingr=malloc(j);
    --j;
    i-=j;
    j=0;
    while(comando[i]!=' '){
      nome_ingr[j]=comando[i];    /* copio il nome della ricetta nel nodo padre */
      ++i;
      ++j;
    }
    nome_ingr[j]='\0';
    hash%=HASHMAGAZZINO;
    ++i;    /* supero il carattere ' ' */


    /* creo un nodo, lo completo dei dati mancanti e lo inserisco nella tabella hash */
    magazzino *nodo_magaz=malloc(sizeof(magazzino));


    /* scorro fino alla fine del numero che indica la quantità */
    while(comando[i]!=' '){
      ++i;
    }

    /* tornando indietro, leggo la quantità da destra a sinistra e la converto da char a int */
    --i;    /* supero il carattere ' ' */
    nodo_magaz->quantita=0;
    fattore=1;
    while(comando[i]!=' '){
      nodo_magaz->quantita+=(((int)comando[i]-48)*fattore);
      --i;
      fattore*=10;
    }
    ++i;    /* supero il carattere ' ' */

    /* scorro nuovamente fino alla fine della quantità */
    while(comando[i]!=' '){
      ++i;
    }
    ++i;    /* supero il carattere ' ' */


    /* scorro fino alla fine del numero che indica la scadenza */
    while(comando[i]!=' ' && comando[i]!='\n'){
      ++i;
    }

    /* tornando indietro, leggo la scadenza da destra a sinistra e la converto da char a int */
    --i;    /* supero il carattere ' ' oppure '\n' */
    nodo_magaz->scadenza=0;
    fattore=1;
    while(comando[i]!=' '){
      nodo_magaz->scadenza+=(((int)comando[i]-48)*fattore);
      --i;
      fattore*=10;
    }
    ++i;    /* supero il carattere ' ' */

    /* scorro nuovamente fino alla fine della scadenza */
    while(comando[i]!=' ' && comando[i]!='\n'){
      ++i;
    }
    ++i;    /* supero il carattere ' ' oppure '\n' */



    /* inseriscimagaz */
    database_magazzino *temp_magaz=hash_array_magaz[hash];

    /* cerco il nome dell'ingrediente */
    while(temp_magaz!=NULL){

      j=0;
      while(temp_magaz->nome[j]==nome_ingr[j] && temp_magaz->nome[j]!='\0' && nome_ingr[j]!='\0'){
        ++j;
      }

      /* se ho confrontato tutta la stringa e sono identiche fino al carattere terminatore, esco dal while */
      if(temp_magaz->nome[j]=='\0' && nome_ingr[j]=='\0'){
        break;    /* SE TROVO UN NODO CON LO STESSO NOME ESCO DAL WHILE */
      }

      /* altrimenti, passo al prossimo nodo */
      else{
        temp_magaz=temp_magaz->next;
      }
    }


    /* se, una volta usciti dal ciclo, mi trovo in un nodo dove il nome corrisponde, inserisco in base alla scadenza */
    if(temp_magaz!=NULL){

      magazzino *temp=temp_magaz->lista;
      free(nome_ingr);
      temp_magaz->scorte_ingr+=nodo_magaz->quantita;    /* sommo la quantità del nuovo lotto alle altre nel nodo padre */


      /* se la lista non è vuota, inserisco in base alla scadenza */
      if(temp!=NULL){

        /* se il lotto ha scadenza maggiore di quella del primo nodo, scorro in avanti di uno e cerco la posizione corretta dove inserirlo */
        if(nodo_magaz->scadenza > temp->scadenza){

          magazzino *temp_prec=temp_magaz->lista;
          temp=temp->next;

          /* scorro tutti i nodi dal secondo in poi con scadenza minore a quella del lotto da inserire */
          while(temp!=NULL){
            if(nodo_magaz->scadenza > temp->scadenza){
              temp=temp->next;
              temp_prec=temp_prec->next;
            }
            else{
              break;    /* SE TROVO UN NODO CON SCADENZA MAGGIORE O UGUALE ESCO DAL WHILE */
            }
          }


          /* se ci sono lotti con scadenza maggiore o uguale, inserisco il nodo prima di questi */
          if(temp!=NULL){

            /* se hanno scadenze diverse, aggiungo il nodo prima del primo nodo con scadenza maggiore */
            if(nodo_magaz->scadenza != temp->scadenza){
              nodo_magaz->next=temp;
              temp_prec->next=nodo_magaz;
            }

            /* se hanno la stessa scadenza, sommo la quantità da aggiungere al nodo già presente e dealloco il nodo del lotto appena arrivato */
            else{
              temp->quantita+=nodo_magaz->quantita;
              free(nodo_magaz);
            }
          }


          /* se invece tutti i lotti hanno scadenza minore, inserisco il nodo in coda alla lista */
          else{
            temp_prec->next=nodo_magaz;
            nodo_magaz->next=NULL;
          }
        }


        /* se ha scadenza minore di quella del primo nodo, aggiungo in testa */
        else if(nodo_magaz->scadenza < temp->scadenza){
          nodo_magaz->next=temp;
          temp_magaz->lista=nodo_magaz;
        }
        /* se hanno la stessa scadenza, sommo la quantità da aggiungere al nodo già presente e dealloco il nodo del lotto appena arrivato */
        else{
          temp->quantita+=nodo_magaz->quantita;
          free(nodo_magaz);
        }
      }


      /* se invece la lista è vuota, aggiungo in testa */
      else{
        nodo_magaz->next=NULL;
        temp_magaz->lista=nodo_magaz;
      }
    }


    /* se non lo ho trovato, creo un nuovo nodo (padre dell'ingrediente) e poi inserisco in testa */
    else{
      database_magazzino *padre_magaz=malloc(sizeof(database_magazzino));
      padre_magaz->next=hash_array_magaz[hash];
      hash_array_magaz[hash]=padre_magaz;
      padre_magaz->lista=nodo_magaz;
      padre_magaz->nome=nome_ingr;
      padre_magaz->scorte_ingr=nodo_magaz->quantita;    /* inizializzo la somma totale delle quantità dei lotti */
      nodo_magaz->next=NULL;
    }
  }


  /* quando il rifornimento è finito, controllo se posso evadere le ricette in attesa di ingredienti mancanti */
  printf("rifornito\n");    /* PRINTF NECESSARIA PER IL PROGETTO */
  rifwakeup();
}





void inseriscicatal(database_catalogo *padre_catal, int i){

  unsigned int hash=0, fattore=1, j=1;


  /* inserisco il primo ingrediente (assumo non ci siano ricette da zero ingredienti) */
  catalogo *primo_ingr=malloc(sizeof(catalogo));
  padre_catal->lista=primo_ingr;    /* collego la lista degli ingredienti al nodo padre di riferimento */

  /* calcolo gli hash primario e secondario del nome ingrediente */
  while(comando[i]!=' '){
    hash+=((int)comando[i]*j);
    ++i;
    ++j;
  }
  char *nome_ingr=malloc(j);
  --j;
  i-=j;
  j=0;
  while(comando[i]!=' '){
    nome_ingr[j]=comando[i];    /* copio il nome della ricetta nel nodo padre */
    ++i;
    ++j;
  }
  nome_ingr[j]='\0';
  hash%=HASHMAGAZZINO;
  ++i;    /* supero il carattere ' ' */


  /* cerco il nome dell'ingrediente */
  database_magazzino *temp_magaz=hash_array_magaz[hash];

  while(temp_magaz!=NULL){

    j=0;
    while(temp_magaz->nome[j]==nome_ingr[j] && temp_magaz->nome[j]!='\0' && nome_ingr[j]!='\0'){
      ++j;
    }

    /* se ho confrontato tutta la stringa e sono identiche fino al carattere terminatore, esco dal while */
    if(temp_magaz->nome[j]=='\0' && nome_ingr[j]=='\0'){
      break;    /* SE TROVO UN NODO CON LO STESSO NOME ESCO DAL WHILE */
    }

    /* altrimenti, passo al prossimo nodo */
    else{
      temp_magaz=temp_magaz->next;
    }
  }


  /* se, una volta usciti dal ciclo, mi trovo in un nodo dove il nome corrisponde, collego il padre dell'ingrediente alla ricetta */
  if(temp_magaz!=NULL){
    primo_ingr->ingrediente=temp_magaz;    /* collego la ricetta al padre dell'ingrediente */
    free(nome_ingr);
  }


  /* se non lo ho trovato, creo un nuovo nodo (padre dell'ingrediente) e poi inserisco in testa */
  else{
    database_magazzino *padre_magaz=malloc(sizeof(database_magazzino));
    padre_magaz->next=hash_array_magaz[hash];
    hash_array_magaz[hash]=padre_magaz;
    padre_magaz->lista=NULL;
    padre_magaz->nome=nome_ingr;
    padre_magaz->scorte_ingr=0;    /* inizializzo la somma totale delle quantità dei lotti */
    primo_ingr->ingrediente=padre_magaz;    /* collego la ricetta al padre dell'ingrediente */
  }


  /* scorro fino alla fine del numero che indica la quantità */
  while(comando[i]!=' ' && comando[i]!='\n'){
    ++i;
  }

  /* tornando indietro, leggo la quantità da destra a sinistra e la converto da char a int */
  --i;    /* supero il carattere ' ' oppure '\n' */
  primo_ingr->quantita=0;
  while(comando[i]!=' '){
    primo_ingr->quantita+=(((int)comando[i]-48)*fattore);
    --i;
    fattore*=10;
  }
  ++i;    /* supero il carattere ' ' */

  /* scorro nuovamente fino alla fine della quantità */
  while(comando[i]!=' ' && comando[i]!='\n'){
    ++i;
  }
  ++i;    /* supero il carattere ' ' oppure '\n' */


  /* inserisco tutti gli altri ingredienti (incluso il secondo) fino a che non finisce il comando */
  while(comando[i]!='\0'){

    /* ripristino le variabili ausiliarie */
    hash=0;
    j=1;


    /* creo un nuovo nodo e calcolo gli hash primario e secondario del nome ingrediente */
    catalogo *ingr_succ=malloc(sizeof(catalogo));
    while(comando[i]!=' '){
      hash+=((int)comando[i]*j);
      ++i;
      ++j;
    }
    char *nome_ingr=malloc(j);
    --j;
    i-=j;
    j=0;
    while(comando[i]!=' '){
      nome_ingr[j]=comando[i];    /* copio il nome della ricetta nel nodo padre */
      ++i;
      ++j;
    }
    nome_ingr[j]='\0';
    hash%=HASHMAGAZZINO;
    ++i;    /* supero il carattere ' ' */


    /* cerco il nome dell'ingrediente */
    database_magazzino *temp_magaz=hash_array_magaz[hash];

    while(temp_magaz!=NULL){

      j=0;
      while(temp_magaz->nome[j]==nome_ingr[j] && temp_magaz->nome[j]!='\0' && nome_ingr[j]!='\0'){
        ++j;
      }

      /* se ho confrontato tutta la stringa e sono identiche fino al carattere terminatore, esco dal while */
      if(temp_magaz->nome[j]=='\0' && nome_ingr[j]=='\0'){
        break;    /* SE TROVO UN NODO CON LO STESSO NOME ESCO DAL WHILE */
      }

      /* altrimenti, passo al prossimo nodo */
      else{
        temp_magaz=temp_magaz->next;
      }
    }


    /* se, una volta usciti dal ciclo, mi trovo in un nodo dove il nome corrisponde, collego il padre dell'ingrediente alla ricetta */
    if(temp_magaz!=NULL){
      ingr_succ->ingrediente=temp_magaz;    /* collego la ricetta al padre dell'ingrediente */
      free(nome_ingr);
    }


    /* se non lo ho trovato, creo un nuovo nodo (padre dell'ingrediente) e poi inserisco in testa */
    else{
      database_magazzino *padre_magaz=malloc(sizeof(database_magazzino));
      padre_magaz->next=hash_array_magaz[hash];
      hash_array_magaz[hash]=padre_magaz;
      padre_magaz->lista=NULL;
      padre_magaz->nome=nome_ingr;
      padre_magaz->scorte_ingr=0;    /* inizializzo la somma totale delle quantità dei lotti */
      ingr_succ->ingrediente=padre_magaz;    /* collego la ricetta al padre dell'ingrediente */
    }


    /* scorro fino alla fine del numero che indica la quantità */
    while(comando[i]!=' ' && comando[i]!='\n'){
      ++i;
    }

    /* tornando indietro, leggo la quantità da destra a sinistra e la converto da char a int */
    --i;    /* supero il carattere ' ' oppure '\n' */
    ingr_succ->quantita=0;
    fattore=1;
    while(comando[i]!=' '){
      ingr_succ->quantita+=(((int)comando[i]-48)*fattore);
      --i;
      fattore*=10;
    }
    ++i;    /* supero il carattere ' ' */

    /* scorro nuovamente fino alla fine della quantità */
    while(comando[i]!=' ' && comando[i]!='\n'){
      ++i;
    }
    ++i;    /* supero il carattere ' ' oppure '\n' */


    /* completo il nodo (da ora in avanti primo_ingr rappresenta l'ingrediente precedente a ingr_succ) */
    primo_ingr->next=ingr_succ;
    primo_ingr=primo_ingr->next;   /* scorro in avanti di 1 */
  }


  /* chiudo la lista degli ingredienti assegnando NULL come puntatore a next dell'ultimo nodo */
  primo_ingr->next=NULL;
  printf("aggiunta\n");    /* PRINTF NECESSARIA PER IL PROGETTO */
}



void aggiungi_ricetta(){

  unsigned int hash=0, i=17, j=1;   /* i=17 perchè considero già letta la stringa iniziale "aggiungi_ricetta " */
  database_catalogo *padre_catal=malloc(sizeof(database_catalogo));     /* creo il nodo padre della ricetta */


  /* calcolo funzione hash del nome della ricetta e la memorizzo nel nodo padre */
  while(comando[i]!=' '){
    hash+=((int)comando[i]*j);
    ++i;
    ++j;
  }
  padre_catal->nome=malloc(j);
  i=17;
  j=0;
  while(comando[i]!=' '){
    padre_catal->nome[j]=comando[i];    /* copio il nome della ricetta nel nodo padre */
    ++i;
    ++j;
  }
  padre_catal->nome[j]='\0';
  hash%=HASHCATALOGO;
  ++i;    /* supero il carattere ' ' oppure '\n' */


  /* cerco se esistono ricette con lo stesso nome */
  database_catalogo *temp_catal=hash_array_catal[hash];
  while(temp_catal!=NULL){

    j=0;
    while(padre_catal->nome[j]==temp_catal->nome[j] && padre_catal->nome[j]!='\0' && temp_catal->nome[j]!='\0'){
      ++j;
    }

    /* se ho confrontato tutta la stringa e sono identiche fino al carattere terminatore, esco dal while */
    if(padre_catal->nome[j]=='\0' && temp_catal->nome[j]=='\0'){
      break;    /* SE TROVO UN NODO CON LO STESSO NOME ESCO DAL WHILE */
    }

    /* altrimenti, passo al prossimo nodo */
    else{
      temp_catal=temp_catal->next;
    }
  }


  /* se la trovo, ignoro il comando perchè la ricetta è già presente nel catalogo */
  if(temp_catal!=NULL){
    printf("ignorato\n");    /* PRINTF NECESSARIA PER IL PROGETTO */
    free(padre_catal->nome);
    free(padre_catal);
  }

  /* se non la trovo, inserisco in testa alla lista delle ricette con lo stesso hash */
  else{
    padre_catal->next=hash_array_catal[hash];
    hash_array_catal[hash]=padre_catal;
    padre_catal->num_ordini_sospesi=0;
    padre_catal->last_check=0;
    padre_catal->max_pezzi=999999;
    inseriscicatal(padre_catal, i);
  }
}





void rimuovi_ricetta(){

  unsigned int hash=0, i=16, j=0;   /* i=16 perchè considero già letta la stringa iniziale "rimuovi_ricetta " */


  /* calcolo funzione hash nome della ricetta e la memorizzo in una stringa temporanea */
  while(comando[i]!='\n'){
    nome_ricetta_rr[j]=comando[i];    /* copio il nome della ricetta */
    ++j;
    hash+=((int)comando[i]*j);
    ++i;
  }
  nome_ricetta_rr[j]='\0';
  hash%=HASHCATALOGO;


  /* se non esistono ricette con la stessa funzione hash, allora assumo che la ricetta non è presente */
  if(hash_array_catal[hash]==NULL){
    printf("non presente\n");    /* PRINTF NECESSARIA PER IL PROGETTO */
  }


  /* se esistono, controllo il nome della ricetta */
  else{
    database_catalogo *temp_catal=hash_array_catal[hash];
    database_catalogo *temp_catal_prec=hash_array_catal[hash];


    /* controllo il primo nodo fuori dal ciclo in modo da poter tenere traccia di temp_catal_prec */
    j=0;
    while(nome_ricetta_rr[j]==temp_catal->nome[j] && nome_ricetta_rr[j]!='\0' && temp_catal->nome[j]!='\0'){
      ++j;
    }

    /* se NON ho riscontro positivo sul primo nodo, controllo i nodi successivi */
    if(nome_ricetta_rr[j]!='\0' || temp_catal->nome[j]!='\0'){
      temp_catal=temp_catal->next;

      while(temp_catal!=NULL){

        j=0;
        while(nome_ricetta_rr[j]==temp_catal->nome[j] && nome_ricetta_rr[j]!='\0' && temp_catal->nome[j]!='\0'){
          ++j;
        }

        /* se ho confrontato tutta la stringa e sono identiche fino al carattere terminatore, esco dal while */
        if(nome_ricetta_rr[j]=='\0' && temp_catal->nome[j]=='\0'){
          break;    /* SE TROVO UN NODO CON LO STESSO NOME ESCO DAL WHILE */
        }

        /* altrimenti, passo al prossimo nodo */
        else{
          temp_catal=temp_catal->next;
          temp_catal_prec=temp_catal_prec->next;
        }
      }


      /* se lo trovo, assumo la ricetta sia persente */
      if(temp_catal!=NULL){

        /* se ci sono ordini in sospeso di quella ricetta, non rimuovo la ricetta */
        if(temp_catal->num_ordini_sospesi>0){
          printf("ordini in sospeso\n");    /* PRINTF NECESSARIA PER IL PROGETTO */
        }

        /* altrimenti, rimuovo la ricetta */
        else{

          /* taglio fuori il nodo padre dalla lista */
          temp_catal_prec->next=temp_catal->next;

          /* dealloco tutti i nodi degli ingredienti di quella ricetta */
          catalogo *temp=temp_catal->lista;
          catalogo *temp_prec=temp_catal->lista;

          while(temp!=NULL){
            temp=temp->next;
            free(temp_prec);
            temp_prec=temp;
          }

          /* dealloco il nodo padre della ricetta */
          free(temp_catal->nome);
          free(temp_catal);
          printf("rimossa\n");    /* PRINTF NECESSARIA PER IL PROGETTO */
        }
      }


      /* se NON lo trovo, assumo la ricetta NON sia persente */
      else{
        printf("non presente\n");    /* PRINTF NECESSARIA PER IL PROGETTO */
      }
    }


    /* se invece ho riscontro positivo sul primo nodo, valuto se rimuovere la ricetta e aggiornare hash_array_catal[hash] */
    else{

      /* se ci sono ordini in sospeso di quella ricetta, non rimuovo la ricetta */
      if(temp_catal->num_ordini_sospesi>0){
        printf("ordini in sospeso\n");    /* PRINTF NECESSARIA PER IL PROGETTO */
      }


      /* altrimenti, rimuovo la ricetta e aggiorno hash_array_catal[hash] */
      else{

        /* taglio fuori il nodo padre dalla lista */
        hash_array_catal[hash]=temp_catal->next;

        /* dealloco tutti i nodi degli ingredienti di quella ricetta */
        catalogo *temp=temp_catal->lista;
        catalogo *temp_prec=temp_catal->lista;

        while(temp!=NULL){
          temp=temp->next;
          free(temp_prec);
          temp_prec=temp;
        }

        /* dealloco il nodo padre della ricetta */
        free(temp_catal->nome);
        free(temp_catal);
        printf("rimossa\n");    /* PRINTF NECESSARIA PER IL PROGETTO */
      }
    }
  }
}





void lethimcook(coda_rif_corr *temp_r_c){

  int totale_ingr=0;
  database_catalogo *temp_catal=temp_r_c->ricetta;
  catalogo *temp_c=temp_catal->lista;
  database_magazzino *temp_magaz=NULL;
  magazzino *temp_m=NULL;
  magazzino *temp_m_prec=NULL;


  /* se posso cucinare l'ordine per intero */
  if(checkmagaz(temp_r_c)==TRUE){

    /* scorro la lista della ricetta: per ogni ingrediente, rimuovo dal magazzino la quantità necessaria per prepararlo e aggiorno la variabile peso */
    while(temp_c!=NULL){

      temp_magaz=temp_c->ingrediente;   /* memorizzo il padre dell'ingrediente da controllare */


      /* rimuovo dal magazzino la quantità necessaria per preparare l'ordine */
      temp_m=temp_magaz->lista;
      temp_m_prec=temp_magaz->lista;
      totale_ingr=temp_r_c->pezzi*temp_c->quantita;
      temp_r_c->peso+=totale_ingr;
      temp_magaz->scorte_ingr-=totale_ingr;    /* rimuovo la quantità necessaria per preparare l'ordine dalla somma nel nodo padre */


      /* finchè ancora necessito di questo ingrediente */
      while(totale_ingr>0){

        /* se la quantità che mi serve è maggiore o uguale di quella contenuta nel lotto, consumerò tutto il lotto (quindi dealloco e avanzo al prossimo) */
        if(totale_ingr >= temp_m->quantita){
          totale_ingr-=temp_m->quantita;
          temp_m=temp_m->next;    /* scorro in avanti di uno */
          free(temp_m_prec);    /* dealloco il nodo */
          temp_m_prec=temp_m;    /* scorro in avanti di uno anche in temp_m_prec */
        }

        /* se invece la quantità che mi serve è minore di quella contenuta nel lotto, consumerò parte del lotto (quindi decremento e NON avanzo) */
        else{
          temp_m->quantita-=totale_ingr;
          totale_ingr=0;    /* questo assegnamento segnala che posso uscire dal ciclo */
        }
      }


      temp_magaz->lista=temp_m;   /* aggiorno il puntatore del padre di quell'ingrediente */
      temp_c=temp_c->next;    /* passo al prossimo ingrediente della ricetta */
    }


    /* quando ho terminato di prelevare tutti gli ingredienti dal magazzino, aggiungo l'ordine alla coda per il corriere */
    /* se la coda è vuota */
    if(inizio_coda_corriere==NULL){
      inizio_coda_corriere=temp_r_c;
      temp_r_c->next=NULL;
      fine_coda_corriere=temp_r_c;
    }

    /* se invece la coda NON è vuota */
    else{
      fine_coda_corriere->next=temp_r_c;
      temp_r_c->next=NULL;
      fine_coda_corriere=temp_r_c;
    }
  }


  /* se invece NON posso cucinare l'ordine per intero, lo metto nella coda di attesa rifornimento */
  else{

    /* se la coda è vuota */
    if(inizio_coda_rifornimento==NULL){
      inizio_coda_rifornimento=temp_r_c;
      temp_r_c->next=NULL;
      fine_coda_rifornimento=temp_r_c;
    }

    /* se invece la coda NON è vuota */
    else{
      fine_coda_rifornimento->next=temp_r_c;
      temp_r_c->next=NULL;
      fine_coda_rifornimento=temp_r_c;
    }
  }
}



void ordine(){

  unsigned int hash=0, i=7, j=0, fattore=1;   /* i=7 perchè considero già letta la stringa iniziale "ordine " */
  coda_rif_corr *temp_r_c=malloc(sizeof(coda_rif_corr));
  temp_r_c->istante_arrivo=time;
  temp_r_c->peso=0;


  /* calcolo funzione hash nome della ricetta e la memorizzo in una stringa temporanea */
  while(comando[i]!=' '){
    nome_ricetta_o[j]=comando[i];    /* copio il nome della ricetta */
    ++j;
    hash+=((int)comando[i]*j);
    ++i;
  }
  nome_ricetta_o[j]='\0';
  hash%=HASHCATALOGO;


  /* scorro fino alla fine del numero che indica il numero di pezzi ordinati */
  ++i;    /* supero il carattere ' ' */
  while(comando[i]!='\n'){
    ++i;
  }

  /* tornando indietro, leggo il numero di pezzi ordinati da destra a sinistra e lo converto da char a int */
  --i;    /* supero il carattere '\n' */
  temp_r_c->pezzi=0;
  while(comando[i]!=' '){
    temp_r_c->pezzi+=(((int)comando[i]-48)*fattore);
    --i;
    fattore*=10;
  }


  /* se non esistono ricette con lo stesso hash primario, allora assumo che la ricetta non è presente nel catalogo e rifiuto l'ordine */
  if(hash_array_catal[hash]==NULL){
    printf("rifiutato\n");    /* PRINTF NECESSARIA PER IL PROGETTO */
    free(temp_r_c);
  }


  /* se esistono, controllo il nome della ricetta */
  else{
    database_catalogo *temp_catal=hash_array_catal[hash];

    while(temp_catal!=NULL){

      j=0;
      while(nome_ricetta_o[j]==temp_catal->nome[j] && nome_ricetta_o[j]!='\0' && temp_catal->nome[j]!='\0'){
        ++j;
      }

      /* se ho confrontato tutta la stringa e sono identiche fino al carattere terminatore, esco dal while */
      if(nome_ricetta_o[j]=='\0' && temp_catal->nome[j]=='\0'){
        break;    /* SE TROVO UN NODO CON LO STESSO NOME ESCO DAL WHILE */
      }

      /* altrimenti, passo al prossimo nodo */
      else{
        temp_catal=temp_catal->next;
      }
    }


    /* se lo trovo, assumo la ricetta sia persente e accetto l'ordine */
    if(temp_catal!=NULL){

      printf("accettato\n");    /* PRINTF NECESSARIA PER IL PROGETTO */
      temp_r_c->ricetta=temp_catal;
      ++temp_catal->num_ordini_sospesi;    /* incremento il contatore nel padre della ricetta che ne impedisce la rimozione dal catalogo */
      lethimcook(temp_r_c);
    }


    /* se NON lo trovo, assumo la ricetta NON sia persente e rifiuto l'ordine */
    else{
      printf("rifiutato\n");    /* PRINTF NECESSARIA PER IL PROGETTO */
      free(temp_r_c);
    }
  }
}
