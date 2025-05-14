/*
 * Mini blockchain Simulator în C
 * Tema: Algoritmi probabilistici și genetici: analiză, implementare, exemple de utilizare
 * Autor: Nicu & ChatGPT
 */

// === Simularea unui Blockchain simplu în C ===
// Fiecare bloc este salvat într-un folder individual
// Fiecare bloc conține: data.txt, prev.txt, hash.txt
// Hash-ul este generat prin SHA-256 și simulează Proof-of-Work

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <openssl/sha.h> // OpenSSL pentru SHA-256

#define MAX_BLOCKS 100
#define DIFFICULTY 3 // Numărul de zerouri cerute la începutul hash-ului (Proof of Work)

// ======= FUNCȚII UTILITARE ========

// Generează hash-ul SHA-256 pentru un string dat (folosit pentru a securiza datele blocului)
void sha256(const char *str, char outputBuffer[65]) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, str, strlen(str));
    SHA256_Final(hash, &sha256);

    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        sprintf(outputBuffer + (i * 2), "%02x", hash[i]);
    }
    outputBuffer[64] = 0;
}

// Verifică dacă hash-ul generat începe cu un anumit număr de zerouri
// (folosit pentru simularea unui mecanism de Proof-of-Work simplificat)
int starts_with_zeros(char *hash, int zeros) {
    for (int i = 0; i < zeros; i++) {
        if (hash[i] != '0') return 0;
    }
    return 1;
}

// ======= CREARE FOLDERE ȘI FIȘIERE =======

// Creează directoarele pentru stocarea unui bloc (nod)
// Format: bc_data/nodes/nodeX, unde X este numărul blocului
void create_block_folder(int index) {
    struct stat st;
    if (stat("bc_data", &st) == -1) {
        mkdir("bc_data", 0755);
    }
    if (stat("bc_data/nodes", &st) == -1) {
        mkdir("bc_data/nodes", 0755);
    }

    char dirname[64];
    sprintf(dirname, "bc_data/nodes/node%d", index);
    if (stat(dirname, &st) == -1) {
        mkdir(dirname, 0755);
    }
}

// Scrie un conținut text într-un fișier specificat
void write_file(const char *filename, const char *content) {
    FILE *f = fopen(filename, "w");
    if (f) {
        fprintf(f, "%s", content);
        fclose(f);
    }
}

// Citește o linie dintr-un fișier și o returnează
// Este folosit pentru a obține hash-ul anterior și datele stocate
char *read_file(const char *filename) {
    static char buffer[2048];
    FILE *f = fopen(filename, "r");
    if (!f) {
        buffer[0] = '\0';
        return buffer;
    }
    if (fgets(buffer, sizeof(buffer), f)) {
        buffer[strcspn(buffer, "\n")] = 0;
    } else {
        buffer[0] = '\0';
    }
    fclose(f);
    return buffer;
}

// ======= CREARE BLOC =======

// Creează un bloc nou în blockchain:
// - scrie datele în fișiere
// - combină datele, hash-ul anterior și un nonce pentru a genera un nou hash valid
// - salvează hash-ul într-un fișier
void create_block(int index, const char *data, const char *prev_hash) {
    char dirname[64];
    sprintf(dirname, "bc_data/nodes/node%d", index);
    create_block_folder(index);

    char data_path[128], prev_path[128], hash_path[128];
    sprintf(data_path, "%s/data.txt", dirname);
    sprintf(prev_path, "%s/prev.txt", dirname);
    sprintf(hash_path, "%s/hash.txt", dirname);

    write_file(data_path, data);
    write_file(prev_path, prev_hash);

    char combined[4096];
    char hash[65];
    int nonce = 0;

    clock_t start = clock();
    do {
        sprintf(combined, "%s%s%d", data, prev_hash, nonce);
        sha256(combined, hash);
        nonce++;
    } while (!starts_with_zeros(hash, DIFFICULTY));
    clock_t end = clock();
    double time_spent = (double)(end - start) / CLOCKS_PER_SEC;

    write_file(hash_path, hash);
    printf("[INFO] Block %d creat în %.2f secunde. Hash: %s\n", index, time_spent, hash);
}

// ======= VERIFICARE INTEGRITATE =======

// Verifică dacă lanțul de blocuri este valid:
// compară hash-ul anterior din fiecare bloc cu hash-ul generat al blocului precedent
void check_chain(int last_index) {
    char prev_hash[65] = "";
    char current_path[128];
    for (int i = 1; i <= last_index; i++) {
        sprintf(current_path, "bc_data/nodes/node%d/prev.txt", i);
        char *read_prev = read_file(current_path);
        if (strcmp(read_prev, prev_hash) != 0) {
            printf("[EROARE] Lanț corupt la block %d!\n", i);
            return;
        }
        sprintf(current_path, "bc_data/nodes/node%d/hash.txt", i);
        char *read_hash = read_file(current_path);
        if (!read_hash || strlen(read_hash) == 0) {
            printf("[EROARE] Hash lipsă sau invalid la block %d.\n", i);
            return;
        }
        strcpy(prev_hash, read_hash);
    }
    printf("[OK] blockchain valid până la block %d.\n", last_index);
}

// ======= INTERFAȚĂ =======
int main() {
    int option, block_index = 1;
    char data[256];
    char prev_hash[65] = "";

    // Dacă există un fișier numit 'bc_data' care nu este director, îl șterge pentru a evita conflictele
    struct stat st;
    if (stat("bc_data", &st) == 0 && !S_ISDIR(st.st_mode)) {
        remove("bc_data");
    }

    while (1) {
        // Meniu principal pentru interacțiunea cu utilizatorul
        // Oferă opțiuni de adăugare blocuri și verificare a integrității
        printf("\n=== MINI BLOCKCHAIN C ===\n");
        printf("1. Adaugă block nou\n");
        printf("2. Verifică integritatea\n");
        printf("0. Ieșire\n> ");
        scanf("%d", &option);
        getchar(); // curăță newline

        switch(option) {
            case 1:
                printf("Date block %d: ", block_index);
                fgets(data, sizeof(data), stdin);
                data[strcspn(data, "\n")] = 0;
                create_block(block_index, data, prev_hash);
                // actualizează prev_hash
                char path[128];
                sprintf(path, "bc_data/nodes/node%d/hash.txt", block_index);
                char *read_new = read_file(path);
                if (!read_new || strlen(read_new) == 0) {
                    printf("[EROARE] Nu s-a putut citi hash-ul pentru block %d.\n", block_index);
                    break;
                }
                strcpy(prev_hash, read_new);
                block_index++;
                break;
            case 2:
                check_chain(block_index - 1);
                break;
            case 0:
                exit(0);
            default:
                printf("[!] Opțiune invalidă.\n");
        }
    }
    return 0;

}


// teza de an 2025 SDA