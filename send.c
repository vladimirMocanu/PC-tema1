//Mocanu-Prejma Vladimir-Georgian
//325CB

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "link_emulator/lib.h"

#define HOST "127.0.0.1"
#define PORT 10000

// pachetul definit de noi

typedef struct {
    char data[MSGSIZE - 2 * sizeof (int)];
    int index;
    int corrupt;
} package;

// functie luata din laborator

unsigned short
in_cksum(const unsigned short *addr, int len, unsigned short csum) {
    int nleft = len;
    const unsigned short *w = addr;
    unsigned short answer;
    int sum = csum;

    /*
     *  Our algorithm is simple, using a 32 bit accumulator (sum),
     *  we add sequential 16 bit words to it, and at the end, fold
     *  back all the carry bits from the top 16 bits into the lower
     *  16 bits.
     */
    while (nleft > 1) {
        sum += *w++;
        nleft -= 2;
    }

    /* mop up an odd byte, if necessary */
    if (nleft == 1)
        sum += *(unsigned char *) w; /* le16toh() may be unavailable on old systems */

    /*
     * add back carry outs from top 16 bits to low 16 bits
     */
    sum = (sum >> 16) + (sum & 0xffff); /* add hi 16 to low 16 */
    sum += (sum >> 16); /* add carry */
    answer = ~sum; /* truncate to 16 bits */
    return (answer);

}

int main(int argc, char **argv) {
    init(HOST, PORT);
    msg t, p, r;
    package pachet, auxPachet;
    // initializam si citim din argv
    // dupa calculam BDP si fereastra
    int size, cadre, i = 0, j = 0;
    int fisier = open(argv[1], O_RDONLY); //deschidem fisierul pentru citire
    int speed = atoi(argv[2]);
    int delay = atoi(argv[3]);
    int BDP = (speed * 1000) * delay;
    int fereastra = BDP / ((MSGSIZE - 2 * sizeof (int)) * 8);

    //aflam dimensiunea fisierului
    struct stat filestatus;
    fstat(fisier, &filestatus);

    //cadre = pachete
    //calculam sa vedem cate pachete trimitem
    size = filestatus.st_size;
    cadre = (size / (MSGSIZE - 2 * sizeof (int))) + 1;

    //facem un vector de tip int si unul de tip package
    package buffer[cadre];
    int buff[cadre];

    //trimitem primul pachet cu toate datele necesare
    pachet.index = filestatus.st_size;
    pachet.corrupt = delay;
    memcpy(t.payload, &pachet, sizeof (package));
    send_message(&t);

    //trimitem pana cand citim tot din fisier
    while (size > 0) {
        for (; j < fereastra; j++) {
            if (size > 0) {

                //initializam cu 0 variabila pachet si citim din fisier
                //dimensiune MSGSIZE - 2 * sizeof(int)
                memset(&pachet, 0, sizeof (package));
                t.len = read(fisier, pachet.data, MSGSIZE - 2 * sizeof (int));

                // buff[i] inseamna ca pe pozitia i este lungimea pe care am citit-o
                // ex: 1396
                // calculam cksum
                buff[i] = t.len;
                pachet.index = i;
                pachet.corrupt = in_cksum((const short unsigned int *) pachet.data, t.len, 0);
                size -= t.len;

                //copiem pachetul in buffer[i] pentru al folosi mai tarziu, daca avem nevoie
                //initializam cu 0 t.payload si trimitem pachetul spre recv.c
                memcpy(&buffer[i], &pachet, sizeof (package));
                memset(t.payload, 0, MSGSIZE);
                memcpy(t.payload, &pachet, sizeof (package));

                send_message(&t);
                i++;
            } else {
                break;
            }

        }
        //verificam daca am terminat de citit din fisier
        if (size > 0) {
            //daca nu primim raspuns inseamna ca s-a terminat dimensiunea ferestrei
            if (recv_message_timeout(&p, delay) < 0) {
                perror("eroare");
                j = 0;
            } else {
                //initializam cu 0 pachet si copiem datele din r.payload
                memset(&pachet, 0, sizeof (package));
                memcpy(&pachet, p.payload, sizeof (package));

                //daca primim ACk mergem mai departe
                if (strcmp(pachet.data, "ACK") == 0) {

                    //aici este la fel ca mai sus cand trimitem pachete pana umplem fereastra
                    memset(&pachet, 0, sizeof (package));
                    t.len = read(fisier, pachet.data, MSGSIZE - 2 * sizeof (int));

                    buff[i] = t.len;
                    pachet.index = i;
                    pachet.corrupt = in_cksum((const short unsigned int *) pachet.data, t.len, 0);
                    size -= t.len;

                    memcpy(&buffer[i], &pachet, sizeof (package));
                    memset(t.payload, 0, MSGSIZE);
                    memcpy(t.payload, &pachet, sizeof (package));

                    send_message(&t);
                    i++;
                }
            }
        }
    }
    //asteptam sa primim raspuns de la recv.c
    //si trimitem pachete inapoi daca este nevoi
    //pana cand primim mesajul scris
    while (1) {
        if (recv_message(&r) < 0) {
            printf("nu merge\n");
        }
        if (strcmp(r.payload, "Scris") == 0) {
            break;

        } else if (strcmp(r.payload, "Gresit") == 0) {
            //daca primim mesajul cu textul Gresit
            //cautam pachetul pe care trebuie sa il trimite
            //il trimitem si asteptam din nou raspuns
            memcpy(&pachet, &r.payload, sizeof (package));
            memcpy(&auxPachet, &buffer[pachet.index], sizeof (pachet));
            t.len = buff[auxPachet.index];
            memcpy(t.payload, &auxPachet, sizeof (package));
            send_message(&t);
        }
    }
    //inchidem fisierul si programul
    close(fisier);
    return 0;
}
