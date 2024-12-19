# Gestione Pasticceria - Prova Finale di Algoritmi e Strutture Dati

## Descrizione del Progetto
Questo progetto simula la gestione operativa di una pasticceria industriale come parte della prova finale del corso di Algoritmi e Strutture Dati 2023-2024. Il sistema gestisce ingredienti, ricette e ordini, operando in tempo discreto e seguendo rigorosi vincoli di efficienza e correttezza.

## Struttura del Progetto
- **File principali:**
  - `progetto.c`: Codice sorgente del progetto.
  - Documentazione PDF con la specifica dettagliata dei requisiti e regole del progetto.

- **Funzionalità principali:**
  - Gestione degli ingredienti e del magazzino.
  - Creazione e rimozione di ricette.
  - Accettazione, attesa e spedizione di ordini.
  - Simulazione di una pasticceria con logica basata su tempo discreto.

## Tecnologie Utilizzate
- Linguaggio di programmazione: C (standard C11).
- Libreria: Solo librerie standard C.
- Non sono utilizzate librerie esterne, né multithreading.

## Come Eseguire il Progetto
1. **Preparazione:**
   - Compilare il file `progetto.c` con un compilatore C compatibile.
   - Prepara un file di input testuale contenente i comandi richiesti.

2. **Compilazione del programma:**
   Usa un compilatore come GCC per generare l'eseguibile.

3. **Esecuzione:**
   - Fornisci al programma un file di input con i comandi.
   - I risultati saranno prodotti in standard output o rediretti in un file.

Esempio di comando per la compilazione:  
gcc -o pasticceria progetto.c  

Esempio di comando per l'esecuzione:  
./pasticceria < input.txt > output.txt  

## Come Testare il Progetto
- Utilizza i casi di test forniti nella documentazione per verificare il comportamento del programma.
- Confronta l'output generato con quello atteso.
- Se disponibile, usa strumenti automatici di generazione di casi di test per verifiche più approfondite.

## Esempio di Input/Output
### Input:
aggiungi_ricetta torta farina 50 uova 10 zucchero 20  
ordine torta 1  

### Output:
aggiunta  
accettato  
camioncino vuoto  

## Conclusioni
Il progetto richiede una gestione efficiente della memoria e attenzione ai dettagli implementativi. Completare il progetto con successo garantisce una valutazione basata su correttezza ed efficienza.

Per maggiori dettagli, consulta la documentazione fornita.

