//Mocanu-Prejma Vladimir-Georgian
//325CB

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include "link_emulator/lib.h"

#define HOST "127.0.0.1"
#define PORT 10001

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
    msg r, t;
    package pachet, auxPachet;

    init(HOST, PORT);
    //deschidem fisierul pentru scriere
    //intializam cu 0;
    int output = open("recv_fileX", O_WRONLY | O_CREAT, 0644);
    int size, delay, cadre, corrupt, i = 0, j = 0;
    //asteptam mesaj cu datele necesare, daca nu primim iesim din program
    if (recv_message(&r) < 0) {
        perror("Receive message");
        return -1;
    }
    //copiem in pachet ,r.payload
    memcpy(&pachet, r.payload, sizeof (package));
    size = pachet.index;
    cadre = (size / (MSGSIZE - 2 * sizeof (int))) + 1;
    delay = pachet.corrupt;

    //facem un vector de tip int si unul de tip package
    package buffer[cadre];
    int buff[cadre];
    //cat timp primim pachete , nu iesim din while
    while (1) {
        //daca nu mai primim pachete iesim din while
        if (recv_message_timeout(&r, 1.5 * delay) < 0) {
            perror("Receive message");
            break;
        } else {
            //copiem in pachet, r.payload
            //si in buffer[pachet.index] datele primite
            memcpy(&pachet, r.payload, sizeof (package));
            memcpy(&buffer[pachet.index], &pachet, sizeof (package));
            //copiem in buff lungimea pe care am citit-o din fisier
            buff[pachet.index] = r.len;
            corrupt = in_cksum((const short unsigned int *) pachet.data, r.len, 0);

            //verificam coruptia
            //daca este corup mesajul o sa il retrimitem
            if (corrupt != pachet.corrupt) {
                buff[pachet.index] = 0;
            }

            //trimit raspuns spre send.c cu mesajul ACK
            strcpy(auxPachet.data, "ACK");
            auxPachet.index = pachet.index;
            memcpy(t.payload, &auxPachet, sizeof (package));
            send_message(&t);
        }
    }
    //cat timp nu am terminat sa scriem tot fisier. nu iesim
    while (size > 0) {

        // daca buff[i] nu este 0 inseamna ca pachetele nu sunt corupte sau pierdute
        if (buff[i] > 0) {

            //copiem in auxPachet din buffer pentru a putea scrie in fisier
            //auxPachet.data
            //scadeam size si crestem i(index);
            memcpy(&auxPachet, &buffer[i], sizeof (package));
            write(output, auxPachet.data, buff[i]);
            size -= buff[i];
            i++;
        } else {
            //daca buff[i] < 0 inseamna ca trebuie sa retrimitem pachetul
            //trimitem mesaj cu textul Gresit
            strcpy(auxPachet.data, "Gresit");
            auxPachet.index = i;
            memcpy(t.payload, &auxPachet, sizeof (package));
            send_message(&t);

            //trimitem si asteptam pana cat primim mesajul bun
            if (recv_message_timeout(&r, 1.5 * delay) < 0) {
                perror("Receive message");
            } else {
                //daca am primit mesaj verificam daca este corupt
                //daca totul este ok scriem in fisier
                //scadem size si crestem i(index);
                memcpy(&pachet, r.payload, sizeof (package));
                corrupt = in_cksum((const short unsigned int *) pachet.data, r.len, 0);

                if (corrupt == pachet.corrupt) {
                    write(output, pachet.data, r.len);
                    size -= r.len;
                    i++;
                }
            }
        }
    }
    //trimitem mesajul cu textul Scris
    //astfel, am terminat de scris in fisier
    strcpy(auxPachet.data, "Scris");
    memcpy(t.payload, &auxPachet, sizeof (package));
    send_message(&t);
    //inchidem fisierul si iesim din program
    close(output);

    return 0;
}