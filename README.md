# Gestione Pasticceria - Prova Finale di Algoritmi e Strutture Dati 

## Descrizione del Progetto
Questo progetto simula la gestione operativa di una pasticceria come parte della prova finale del corso di Algoritmi e Strutture Dati 2023-2024. Il sistema gestisce ingredienti, ricette e ordini, operando in tempo discreto e seguendo rigorosi vincoli di efficienza e correttezza.

## Struttura del Progetto
- **Risorse principali:**
  - [Codice sorgente](progetto.c)
  - [Presentazione del progetto](docs/PFAPI2023-2024.pdf)
  - [Documentazione e specifiche](docs/Prova%20finale%20di%20algoritmi%20e%20strutture%20dati.pdf)
  - [Testing](docs/strumenti_progetto_api.pdf)
  - [Esempi di input e output per verificare correttezza e prestazioni del programma](test_cases_pubblici)

- **Funzionalità principali:**
  - Gestione degli ingredienti e del magazzino
  - Creazione e rimozione di ricette
  - Accettazione, gestione e spedizione di ordini
  - Simulazione di una pasticceria con logica basata su tempo discreto

## Tecnologie Utilizzate
- Linguaggio di programmazione: C (standard C11)
- Non sono utilizzate librerie esterne, né multithreading

## Come Eseguire il Progetto
1. **Compilazione del programma:**
   Usa un compilatore come GCC e le opzioni del Makefile per generare l'eseguibile:
   ```bash
   make progetto

3. **Esecuzione:**
   Fornisci al programma un file di input con i comandi e redirigi l'output in un file di testo:
   ```bash  
   ./progetto < test_cases_pubblici/input.txt > output.txt  

5. **Confronto Output:**
   Confronta il file `output.txt` generato con il file `test_cases_pubblici/output.txt` usando strumenti come `diff` o `meld`:
   ```bash
   diff output.txt test_cases_pubblici/output.txt
   ```
   ```bash
   meld output.txt test_cases_pubblici/output.txt
   
## Come Testare il Progetto
- **Cartella `test_cases_pubblici`:** Include esempi di file di input e output per verificare il corretto funzionamento del programma. 
- Verifica il comportamento del programma confrontando l'output generato con i file di output forniti.
- Usa strumenti come `diff` o `meld` per identificare eventuali discrepanze.
- Usa strumenti come `callgrind` o `massif` per verificarne le prestazioni. 

## Conclusioni
Il progetto richiede una gestione efficiente della memoria e del tempo di esecuzione, oltre che una discreta attenzione ai dettagli implementativi.

Per maggiori dettagli, consulta la documentazione fornita nella sezione `risorse principali`.
